#include <Windows.h>
#include "commands/CommandManager.h"
#include "core/Hooks.h"
#include "core/Logger.h"
#include "gui/Input.h"
#include "modules/ModuleManager.h"
#include "render/Renderer.h"
#include "sdk/CommandSender.h"
#include "sdk/Enchant.h"
#include "sdk/PlayerTick.h"

HMODULE g_module = nullptr;

namespace {
    // Hooks everything. false = nothing usable was installed (no menu, so End could never unload us).
    bool initAll() {
        if (!Hooks::init()) return false;
        // Each step logs its own details; a missing signature only disables the features that need it
        if (!PlayerTick::init())     // normalTick hooks = our way onto the client/server threads
            LOG("warning: player tick hooks incomplete - modules and commands that need the game thread won't run");
        ModuleManager::init();       // after PlayerTick: availability and the saved on/off state depend on it
        if (!Enchant::init())        // EnchantUtils::applyEnchant for the enchant command
            LOG("warning: enchant is unavailable");
        if (!CommandSender::init())  // CommandRequestPacket: send /commands to a remote server
            LOG("warning: commands can't be sent to remote servers");
        CommandManager::init();      // commands for the command bar (up, help)
        if (!Input::init())          // raw input filter
            LOG("warning: raw input hook failed - the camera may move while the menu is open");
        if (!Renderer::init()) {     // Present hook -> ImGui menu (and the WndProc that reads End)
            LOG("renderer hooks failed - without a menu there is no way to unload, so unloading now");
            return false;
        }
        return true;
    }
}

static DWORD WINAPI mainThread(LPVOID) {
    Logger::init();
    LOG("Injected! Minecraft.Windows.exe base = %p", GetModuleHandleW(nullptr));

    if (initAll()) {
        LOG("ready. Insert = menu, Home = command, End = unload");
        while (!Input::unloadRequested) {
            Input::hookGameInput();   // needs a mouse/keyboard reading to exist, so keep trying
            Sleep(100);
        }
    }

    LOG("unloading...");
    ModuleManager::shutdown();   // turn modules off (restores rendering)
    const bool wndProcRemoved = Input::uninstall();
    const bool hooksRemoved = Hooks::shutdown();   // waits for game threads still inside our detours
    if (!wndProcRemoved || !hooksRemoved) {
        // Freeing the DLL now would crash the game; stay loaded (doing nothing) instead
        LOG("could not unload cleanly - the client stays in memory but does nothing. Restart the game to remove it.");
        Logger::shutdown();
        ExitThread(0);
    }
    Renderer::shutdown();
    LOG("bye");
    Logger::shutdown();
    FreeLibraryAndExitThread(g_module, 0);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        // Never do real work inside DllMain (loader lock) - start a thread instead
        if (HANDLE t = CreateThread(nullptr, 0, mainThread, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
