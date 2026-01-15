#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <sol/sol.hpp>

namespace LuaHandler {
    void write_process_memory(uint64_t addr, uint64_t offset, int value);
    void write_process_memory(uint64_t addr, uint64_t offset, float value);
    void write_process_memory(uint64_t addr, uint64_t offset, double value);
    void write_process_memory(uint64_t addr, uint64_t offset, uint64_t value);
    void write_process_memory(uint64_t addr, uint64_t offset, bool value);
    void write_process_memory(uint64_t addr, uint64_t offset, const std::string& value);

    sol::object read_process_memory(sol::this_state ts,
        uint64_t addr,
        uint64_t offset,
        const std::string& type);
    sol::object read_process_memory(sol::this_state ts,
        uint64_t addr,
        const std::string& type);

    bool setup_lua_vm();
    void execute(const std::string& code);

    extern sol::state lua;
    extern std::vector<std::thread> lua_threads;

}