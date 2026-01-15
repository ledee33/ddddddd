#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include "Util.h"

namespace Instances {
    // Instance classes

    class Instance {
    public:
        int64_t addr = 0;
        std::string class_name = "";
        std::string name = "";
        std::vector<Instance> children;
        std::vector<Instance> descendants;
        std::vector<Instance> parent;

        bool get_name();
        bool get_class_name();
        bool get_parent();
        bool get_children();
        bool get_descendants();
        bool update_instance();
        bool update_tree(std::unordered_set<uint64_t>& visited);
    };

    class BaseInstance {
    public:
        Instance data_model;
        uint64_t data_model_value;
        std::string game_name;
    };

    extern BaseInstance base;
    extern std::thread game_checker;
    extern std::thread game_updater;

    // Main functions

    bool setup();
    bool get_game();
    bool start_game_checker();
    bool start_data_model_updater();

    uint64_t get_game_addr();

    Instance retrieve_data_model(bool refetch);

    std::string get_name(uint64_t addr);
    std::string get_name(Instance instance);

    std::string get_class_name(uint64_t addr);
    std::string get_class_name(Instance instance);

    std::vector<Instance> get_parent(uint64_t addr);
    std::vector<Instance> get_parent(Instance instance);
    Instance return_parent(Instance instance);

    std::vector<Instance> get_children(uint64_t addr);
    std::vector<Instance> get_children(Instance instance);

    std::vector<Instance> get_descendants(uint64_t addr);
    std::vector<Instance> get_descendants(Instance instance);

    Instance find_first_child(uint64_t addr, const std::string& name);
    Instance find_first_child(Instance instance, const std::string& name);
    Instance return_child(Instance instance, const std::string& name);

    // Other functions

    Util::Vector3 get_size(uint64_t addr);
    Util::Vector3 get_size(Instance instance);
    void set_size(uint64_t addr, Util::Vector3 value);
    void set_size(Instance instance, Util::Vector3 value);

    Util::Vector3 get_position(uint64_t addr);
    Util::Vector3 get_position(Instance instance);
    void set_position(uint64_t addr, Util::Vector3 value);
    void set_position(Instance instance, Util::Vector3 value);

    Instance get_local_player();
    Instance get_character(Instance player);
}
