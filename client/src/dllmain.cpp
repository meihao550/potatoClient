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

static DWORD WINAPI mainThread(LPVOID) {
    Logger::init();
    LOG("Injected! Minecraft.Windows.exe base = %p", GetModuleHandleW(nullptr));

    if (Hooks::init()) {
        ModuleManager::init();   // module hooks (Xray, ...)
        PlayerTick::init();      // normalTick hooks = our way onto the client/server threads
        Enchant::init();         // EnchantUtils::applyEnchant for the enchant command
        CommandSender::init();   // CommandRequestPacket: send /commands to a remote server
        CommandManager::init();  // commands for the command bar (up, help)
        Input::init();           // raw input filter
        Renderer::init();        // Present hook -> ImGui menu
    }
    LOG("ready. Insert = menu, Home = command, End = unload");

    while (!Input::unloadRequested) {
        Input::hookGameInput();   // needs a mouse/keyboard reading to exist, so keep trying
        Sleep(100);
    }

    LOG("unloading...");
    ModuleManager::shutdown();   // turn modules off (restores rendering)
    Input::uninstall();
    Hooks::shutdown();
    Sleep(200);                  // let in-flight hooked calls return
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
