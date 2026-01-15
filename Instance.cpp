#include "pch.h"
#include "Instance.h"
#include "Handlers/LuaHandler.h"
#include "Memory/Memory.h"
#include "Util/Offsets.h"
#include "Util/Globals.h"

using namespace Util;

namespace Instances {

    BaseInstance base;

    std::thread game_checker;
    std::thread game_updater;

    void print_instance_tree(const Instance& inst, int depth = 0) {
        std::string indent(depth * 2, ' ');
        std::cout << indent << inst.name << " [" << inst.class_name << "]" << std::endl;
        for (const auto& child : inst.children) {
            print_instance_tree(child, depth + 1);
        }
    }

    uint64_t get_game_addr() {
        uint64_t render_job_string = mem.FindSignature(
            skCrypt("65 6E 64 65 72 4A 6F 62 28 45 61 72 6C 79 52 65 6E 64 65 72 69 6E 67 3B 55 67 63 29").decrypt(),
            0x1000,
            0xffffffff
        );
        uint64_t maybe_render_job2 = mem.FindSignature(
            skCrypt("65 6E 64 65 72 4A 6F 62 28 45 61 72 6C 79 52 65 6E 64 65 72 69 6E 67 3B 55 67 63 29").decrypt(),
            render_job_string + 0x200,
            0xffffffff
        );
        if (maybe_render_job2) {
            render_job_string = maybe_render_job2;
        }

        uint64_t fake_data_model = mem.Read<uint64_t>(render_job_string + Offsets::fake_data_model);
        uint64_t real_data_model = mem.Read<uint64_t>(fake_data_model + Offsets::real_data_model);

        if (Globals::debug) {
            LOGI << std::hex << render_job_string;
            LOGI << std::hex << fake_data_model;
            LOGI << std::hex << real_data_model;
        }

        return real_data_model;
    }

    bool get_game() {
        base.data_model.addr = get_game_addr();
        if (!base.data_model.addr) return false;
        base.data_model.get_name();
        base.data_model.get_class_name();
        base.data_model.get_children();
        base.data_model_value = mem.Read<uint64_t>(base.data_model.addr);
        return true;
    }

    bool setup() {
        bool got_game = get_game();
        if (!got_game) return false;
        Globals::game_loaded = true;
        return true;
    }

    Instance retrieve_data_model(bool refetch) {
        if (refetch) {
            base.data_model = Instance{};
            setup();
        }
        return base.data_model;
    }

    bool start_data_model_updater() {
        std::thread updater_thread([]() {
            while (true) {
                if (!Globals::game_loaded || !base.data_model.addr) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    LOGI << "Sleeping";
                    continue;
                }
                LOGI << "Updating tree";
                std::unordered_set<uint64_t> visited;
                base.data_model.update_tree(visited);
                print_instance_tree(base.data_model, 5);

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            });

        updater_thread.detach();
        game_updater = std::move(updater_thread);
        return true;
    }

    bool start_game_checker() {
        std::thread checker_thread([]() {
            while (true) {
                if (mem.Read<uint64_t>(base.data_model.addr) != base.data_model_value) {
                    LOGI << skCrypt("[TECHNO] Data model change detected, searching for new!").decrypt();
                    Globals::game_loaded = false;
                    while (!Globals::game_loaded) {
                        setup();
                        std::this_thread::sleep_for(std::chrono::milliseconds(25));
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            });

        checker_thread.detach();
        game_checker = std::move(checker_thread);
        return true;
    }

    std::vector<Instance> get_children(uint64_t addr) {
        std::vector<Instance> container;
        if (!addr) {
            LOGE << skCrypt("[TECHNO] Address for get_children is null").decrypt();
            return container;
        }

        uint64_t child_list = mem.Read<uint64_t>(addr + Offsets::children);
        if (!child_list) return container;

        uint64_t child_end = mem.Read<uint64_t>(child_list + Offsets::string);
        if (!child_end) return container;

        uint64_t current = mem.Read<uint64_t>(child_list);

        size_t count = (child_end - current) / 0x10;
        std::vector<uint64_t> foundchildren;
        foundchildren.reserve(count);

        HANDLE scatter_handle = mem.CreateScatterHandle();

        while (current != child_end) {
            foundchildren.emplace_back(0ull);
            mem.AddScatterReadRequest(scatter_handle, current, &foundchildren.back(), sizeof(uint64_t));
            current += 0x10;
        }

        mem.ExecuteReadScatter(scatter_handle);
        mem.CloseScatterHandle(scatter_handle);

        for (auto childptr : foundchildren) {
            if (childptr) {
                Instance child;
                child.addr = childptr;
                child.update_instance();
                container.push_back(child);
            }
        }

        return container;
    }

    std::vector<Instance> get_children(Instance instance) {
        return get_children(instance.addr);
    }

    std::vector<Instance> get_parent(uint64_t addr) {
        std::vector<Instance> container;
        uint64_t parent_addr = mem.Read<uint64_t>(addr + Offsets::parent);

        if (parent_addr) {
            Instance parent_inst;
            parent_inst.addr = parent_addr;
            parent_inst.update_instance();
            container.emplace_back(std::move(parent_inst));
        }

        return container;
    }

    std::vector<Instance> get_parent(Instance instance) {
        return get_parent(instance.addr);
    }

    Instance return_parent(Instance instance) {
        if (instance.parent.empty()) {
            if (!instance.get_parent()) {
                return {};
            }
        }
        if (instance.class_name == "Datamodel") {
            return{};
        }
        return instance.parent.at(0);
    }

    std::vector<Instance> get_descendants(uint64_t addr) {
        std::vector<Instance> descendants;
        if (!addr) return descendants;

        static thread_local std::unordered_set<uint64_t> visited;
        if (!visited.insert(addr).second) return descendants;

        for (auto& child : get_children(addr)) {
            descendants.emplace_back(child);
            auto child_descendants = get_descendants(child.addr);
            descendants.insert(
                descendants.end(),
                std::make_move_iterator(child_descendants.begin()),
                std::make_move_iterator(child_descendants.end())
            );
        }

        if (visited.size() == 1) visited.clear();

        return descendants;
    }

    std::vector<Instance> get_descendants(Instance instance) {
        return get_descendants(instance.addr);
    }

    std::string get_class_name(uint64_t addr) {
        if (!addr) return {};

        uint64_t desc_ptr = mem.Read<uint64_t>(addr + Offsets::class_descriptor);
        uint64_t name_ptr = mem.Read<uint64_t>(desc_ptr + Offsets::string);
        int name_len = mem.Read<int>(name_ptr + Offsets::string_length);

        if (name_len >= 16) {
            uint64_t nested_ptr = mem.Read<uint64_t>(name_ptr);
            return mem.ReadString(nested_ptr);
        }

        return mem.ReadString(name_ptr);
    }

    std::string get_class_name(Instance instance) {
        return get_class_name(instance.addr);
    }

    std::string get_name(uint64_t addr) {
        if (!addr) return {};
        uint64_t name_ptr = mem.Read<uint64_t>(addr + Offsets::name);
        uint64_t name_len = mem.Read<uint64_t>(name_ptr + Offsets::string_length);

        if (name_len >= 16) {
            uint64_t nested_ptr = mem.Read<uint64_t>(name_ptr);
            return mem.ReadString(nested_ptr);
        }

        return mem.ReadString(name_ptr);
    }

    std::string get_name(Instance instance) {
        return get_name(instance.addr);
    }

    Instance find_first_child(uint64_t addr, const std::string& name) {
        for (auto& child : get_children(addr)) {
            if (child.name == name) return child;
        }
        return {};
    }

    Instance find_first_child(Instance instance, const std::string& name) {
        return find_first_child(instance.addr, name);
    }

    Instance return_child(Instance instance, const std::string& name) {
        if (instance.children.empty()) return {};
        for (auto& child : instance.children) {
            if (child.name == name) return child;
        }
        return {};
    }

    Util::Vector3 get_size(uint64_t addr) {
        uint64_t primitive_pointer = mem.Read<uint64_t>(addr + Offsets::primitive);

        if (!primitive_pointer) {
            LOGE << skCrypt("Failed to read the size of ").decrypt() << std::hex << addr;
            return {};
        }
        Vector3 pos;
        pos.x = mem.Read<float>(primitive_pointer + Offsets::sizex);
        pos.y = mem.Read<float>(primitive_pointer + Offsets::sizey);
        pos.z = mem.Read<float>(primitive_pointer + Offsets::sizez);
        return pos;
    }

    Util::Vector3 get_size(Instance instance) {
        return get_size(instance.addr);
    }

    void set_size(uint64_t addr, Util::Vector3 size) {
        uint64_t primitive_pointer = mem.Read<uint64_t>(addr + Offsets::primitive);

        if (!primitive_pointer) {
            LOGE << skCrypt("Failed to set the size of ").decrypt() << std::hex << addr;
        }
        HANDLE scatter_handle = mem.CreateScatterHandle();

        mem.AddScatterWriteRequest(scatter_handle, primitive_pointer + Offsets::sizex, &size.x, sizeof(float));
        mem.AddScatterWriteRequest(scatter_handle, primitive_pointer + Offsets::sizey, &size.y, sizeof(float));
        mem.AddScatterWriteRequest(scatter_handle, primitive_pointer + Offsets::sizez, &size.z, sizeof(float));

        mem.ExecuteWriteScatter(scatter_handle);
        mem.CloseScatterHandle(scatter_handle);
    }

    void set_size(Instance instance, Util::Vector3 size) {
        set_size(instance.addr, size);
    }

    Instance get_local_player() {
        Instance players = return_child(base.data_model, "Players");
        if (players.addr) {
            Instance local_player;
            local_player.addr = mem.Read<uint64_t>(players.addr + Offsets::local_player);
            local_player.update_instance();
            return local_player;
        }
        return {};
    }

    Instance get_character(Instance player) {
        if (player.class_name.empty()) {
            player.update_instance();
        }
        if (player.class_name != "Player") {
            LOGE << "Tried to access the character of a non player!";
            return {};
        }
        Instance character;
        character.addr = mem.Read<uint64_t>(player.addr + Offsets::character);
        character.update_instance();
        return character;
    }

    bool Instance::update_tree(std::unordered_set<uint64_t>& visited) {
        if (!update_instance()) return false;
        if (!visited.insert(addr).second) return false;

        children.clear();
        auto direct = Instances::get_children(addr);
        if (!direct.empty()) {
            children = std::move(direct);
        }

        for (auto& c : children) {
            c.update_tree(visited);
        }

        return true;
    }

    bool Instance::update_instance() {
        if (!addr) {
            LOGD << skCrypt("[TECHNO] Instance address for update_instance is null").decrypt();
            return false;
        }
        if (!get_class_name()) {
            LOGE << skCrypt("[TECHNO] Failed to get class name").decrypt();
            return false;
        }
        if (!get_name()) {
            LOGE << skCrypt("[TECHNO] Failed to get instance name").decrypt();
            return false;
        }
        if (!get_parent()) {
            LOGE << skCrypt("[TECHNO] Failed to get instance parent").decrypt();
            return false;
        }
        return true;
    }

    bool Instance::get_parent() {
        if (!addr) return false;
        parent = Instances::get_parent(addr);
        return true;
    }

    bool Instance::get_class_name() {
        if (!addr) return false;
        class_name = Instances::get_class_name(addr);
        return true;
    }

    bool Instance::get_name() {
        if (!addr) return false;
        name = Instances::get_name(addr);
        return true;
    }

    bool Instance::get_children() {
        if (!addr) return false;
        auto children_vec = Instances::get_children(addr);
        if (children_vec.empty()) return false;
        children = std::move(children_vec);
        return true;
    }

    bool Instance::get_descendants() {
        if (!addr) return false;
        auto descendants_vec = Instances::get_descendants(addr);
        if (descendants_vec.empty()) return false;
        descendants = std::move(descendants_vec);
        return true;
    }
}
