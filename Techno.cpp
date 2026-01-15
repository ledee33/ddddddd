#include "pch.h"

#include "Util/Util.h"
#include "Util/Instance.h"
#include "Handlers/LuaHandler.h"
#include "Handlers/ImguiHandler.h"
#include "Handlers/KeybindHandler.h"
#include "Memory/Memory.h"
#include "Util/Globals.h"

#include <skStr.h>

using namespace Instances;

static void keepopen() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int main() {
    plog::init(plog::verbose, "c:\\techno\\logs\\log.txt", 1000000, 5);
    plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::get()->addAppender(&consoleAppender);

    LOGI << skCrypt("[TECHNO] Authenticating user...").decrypt();
    //authenticate_user();
    LOGI << skCrypt("[TECHNO] Authentication complete.").decrypt();

    int result = MessageBoxA(nullptr, skCrypt("Use kernel mode?").decrypt(), skCrypt("Memory Mode").decrypt(), MB_YESNO | MB_ICONQUESTION);
    Globals::kernel_mode = (result == IDYES);

    if (Globals::kernel_mode) {
        LOGI << skCrypt("[TECHNO] Kernel mode enabled.").decrypt();
        LOGE << skCrypt("[TECHNO] Kernel mode not yet implemented").decrypt();
        return true;
    }

    if (!mem.Init("RobloxPlayerBeta.exe", true, false)) {
        LOGE << skCrypt("[TECHNO] Failed to init Roblox.").decrypt();
        return false;
    }

    LOGI << skCrypt("[TECHNO] Successfully initialized DMA.").decrypt();

    if (!Instances::setup()) {
        LOGE << skCrypt("[TECHNO] Failed to set up instances.").decrypt();
        return false;
    }

    if (!LuaHandler::setup_lua_vm()) {
        LOGE << skCrypt("[TECHNO] Failed to set up Lua VM.").decrypt();
        return false;
    }

    if (!KeybindHandler::start_keybinds()) {
        LOGE << skCrypt("[TECHNO] Failed to start keybind handler.").decrypt();
        return false;
    }

    if (!ImguiHandler::start_window()) {
        LOGE << skCrypt("[TECHNO] Failed to start GUI.").decrypt();
        return false;
    }

    if (!Instances::start_game_checker()) {
        LOGE << skCrypt("[TECHNO] Failed to start up leave listener...").decrypt();
        return false;
    }

    if (!Instances::start_data_model_updater()) {
        LOGE << skCrypt("[TECHNO] Failed to start up workspace updater...").decrypt();
        return false;
    }

    LOGI << skCrypt("[TECHNO] Ready!.").decrypt();
    keepopen();
    return 0;
}