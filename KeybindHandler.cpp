#include "pch.h"
#include "KeybindHandler.h"
#include "Util/Globals.h"

namespace KeybindHandler {
    std::thread keybind_thread;
    std::atomic<bool> running{ false };
    static const int keys_to_check[] = { VK_INSERT };

    std::vector<int> get_pressed_keys() {
        std::vector<int> pressed_keys;
        for (int key : keys_to_check) {
            if (GetAsyncKeyState(key) & 0x8000) {
                pressed_keys.push_back(key);
            }
        }
        return pressed_keys;
    }

    void keybind_loop() {
        auto last_insert_press = std::chrono::steady_clock::now();
        while (running) {
            auto keys_down = get_pressed_keys();
            if (!keys_down.empty()) {
                auto now = std::chrono::steady_clock::now();
                for (int key : keys_down) {
                    if (key == VK_INSERT) {
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - last_insert_press
                        ).count();
                        if (elapsed > 200) {
                            Globals::menu_open = !Globals::menu_open;
                            last_insert_press = now;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    bool start_keybinds() {
        if (running) {
            return false;
        }
        running = true;
        keybind_thread = std::thread(keybind_loop);
        keybind_thread.detach();
        return true;
    }

    void stop_keybinds() {
        running = false;
        if (keybind_thread.joinable()) {
            keybind_thread.join();
        }
    }
}
