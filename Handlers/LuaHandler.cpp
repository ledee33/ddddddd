#include "pch.h"
#include "LuaHandler.h"
#include "KeybindHandler.h"
#include "Util/Util.h"
#include "Util/Instance.h"
#include "Memory/Memory.h"
#include "Util/Globals.h"

using namespace Instances;
using namespace Util;

namespace LuaHandler {

    sol::state lua;
    std::vector<std::thread> lua_threads;

    void write_process_memory(uint64_t addr, uint64_t offset, int value) { mem.Write(addr + offset, value); }
    void write_process_memory(uint64_t addr, uint64_t offset, float value) { mem.Write(addr + offset, value); }
    void write_process_memory(uint64_t addr, uint64_t offset, double value) { mem.Write(addr + offset, value); }
    void write_process_memory(uint64_t addr, uint64_t offset, uint64_t value) { mem.Write(addr + offset, value); }
    void write_process_memory(uint64_t addr, uint64_t offset, bool value) { mem.Write(addr + offset, value); }
    void write_process_memory(uint64_t addr, uint64_t offset, const std::string& value) { mem.WriteString(addr + offset, value); }

    sol::object read_process_memory(sol::this_state ts, uint64_t addr, uint64_t offset, const std::string& type) {
        sol::state_view view{ ts };
        if (type == "int")    return sol::make_object(view, mem.Read<int>(addr + offset));
        if (type == "uint64")    return sol::make_object(view, mem.Read<uint64_t>(addr + offset));
        if (type == "float")    return sol::make_object(view, mem.Read<float>(addr + offset));
        if (type == "double")    return sol::make_object(view, mem.Read<double>(addr + offset));
        if (type == "bool")    return sol::make_object(view, mem.Read<bool>(addr + offset));
        if (type == "string")    return sol::make_object(view, mem.ReadString(addr + offset));
        return sol::make_object(view, sol::nil);
    }

    sol::object read_process_memory(sol::this_state ts, uint64_t addr, const std::string& type) {
        return read_process_memory(ts, addr, 0, type);
    }

    void execute(const std::string& code) {
        try {
            lua.script(code);
        }
        catch (const sol::error& e) {
            LOGE << skCrypt("[TECHNO] Lua error: ").decrypt() << e.what();
        }
    }

    bool setup_lua_vm() {
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::io, sol::lib::os);

        lua.new_usertype<Instance>("Instance",
            sol::meta_function::index,
            [&](sol::this_state ts, Instance& self, const std::string& key) -> sol::object {
                sol::state_view lua(ts);

                sol::optional<sol::object> builtin = lua["Instance"][key];
                if (builtin && builtin->valid()) {
                    return *builtin;
                }

                if (key == "parent") {
                    Instance p = return_parent(self);
                    return sol::make_object(lua, p);
                }

                Instance child = Instances::return_child(self, key);

                if (child.addr != 0) {
                    return sol::make_object(lua, child);
                }

                return sol::nil;
            },

            "address", &Instance::addr,
            "name", &Instance::name,
            "classname", &Instance::class_name,
            "children", &Instance::children,
            "getparent", &Instance::get_parent,
            "getchildren", &Instance::get_children,
            "getdescendants", &Instance::get_descendants,
            "getname", &Instance::get_name,
            "getclassname", &Instance::get_class_name,
            "updateinstance", &Instance::update_instance
        );

        lua.new_usertype<Vector2>("Vector2",
            "x", &Vector2::x,
            "y", &Vector2::y
        );
        lua.set_function("getvector2", Util::newvector2);
        lua.new_usertype<Vector3>("Vector3",
            "x", &Vector3::x,
            "y", &Vector3::y,
            "z", &Vector3::z
        );
        lua.set_function("getvector3", Util::newvector3);

        lua["game"] = base.data_model;

        lua.set_function("readprocessmemory", sol::overload(
            [](sol::this_state ts, uint64_t addr, uint64_t offset, const std::string& t) { return read_process_memory(ts, addr, offset, t); },
            [](sol::this_state ts, uint64_t addr, const std::string& t) { return read_process_memory(ts, addr, t); }
        ));

        lua.set_function("writeprocessmemory", sol::overload(
            [](uint64_t addr, int v) { write_process_memory(addr, 0, v); },
            [](uint64_t addr, uint64_t offset, int v) { write_process_memory(addr, offset, v); },
            [](uint64_t addr, float v) { write_process_memory(addr, 0, v); },
            [](uint64_t addr, uint64_t offset, float v) { write_process_memory(addr, offset, v); },
            [](uint64_t addr, double v) { write_process_memory(addr, 0, v); },
            [](uint64_t addr, uint64_t offset, double v) { write_process_memory(addr, offset, v); },
            [](uint64_t addr, uint64_t v) { write_process_memory(addr, 0, v); },
            [](uint64_t addr, uint64_t offset, uint64_t v) { write_process_memory(addr, offset, v); },
            [](uint64_t addr, bool v) { write_process_memory(addr, 0, v); },
            [](uint64_t addr, uint64_t offset, bool v) { write_process_memory(addr, offset, v); },
            [](uint64_t addr, const std::string& s) { write_process_memory(addr, 0, s); },
            [](uint64_t addr, uint64_t offset, const std::string& s) { write_process_memory(addr, offset, s); }
        ));

        lua.set_function("findfirstchild", sol::overload(
            static_cast<Instance(*)(uint64_t, const std::string&)>(&Instances::find_first_child),
            static_cast<Instance(*)(Instance, const std::string&)>(&Instances::find_first_child)
        ));

        lua.set_function("getchildren", sol::overload(
            static_cast<std::vector<Instance>(*)(uint64_t)>(&Instances::get_children),
            static_cast<std::vector<Instance>(*)(Instance)>(&Instances::get_children)
        ));

        lua.set_function("getdescendants", sol::overload(
            static_cast<std::vector<Instance>(*)(uint64_t)>(&Instances::get_descendants),
            static_cast<std::vector<Instance>(*)(Instance)>(&Instances::get_descendants)
        ));

        lua.set_function("getlocalplayer", Instances::get_local_player);
        lua.set_function("getlocalplayer", Instances::get_local_player);
        lua.set_function("getcharacter", Instances::get_character);
        lua.set_function("getcharacter", Instances::get_character);

        lua.set_function("getpressedkeys", KeybindHandler::get_pressed_keys);

        lua.set_function("getsize", sol::overload(
            static_cast<Vector3(*)(uint64_t)>(&Instances::get_size),
            static_cast<Vector3(*)(Instance)>(&Instances::get_size)
        ));
        lua.set_function("setsize", sol::overload(
            static_cast<void(*)(uint64_t, Vector3)>(&Instances::set_size),
            static_cast<void(*)(Instance, Vector3)>(&Instances::set_size)
        ));

        //LOGI << skCrypt("[TECHNO] Successfully created Lua VM").decrypt();
        return true;
    }
}
