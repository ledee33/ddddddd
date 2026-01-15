#pragma once

namespace KeybindHandler {
    extern std::thread keybind_thread;
    extern std::atomic<bool> running;

    std::vector<int> get_pressed_keys();
    void keybind_loop();
    bool start_keybinds();
    void stop_keybinds();
}