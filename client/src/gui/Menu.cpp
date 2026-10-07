#include "Menu.h"
#include "commands/CommandManager.h"
#include "core/InputFocus.h"
#include "core/Logger.h"
#include "modules/Config.h"
#include "modules/ModuleManager.h"
#include <Windows.h>
#include <imgui.h>
#include <cstdio>
#include <string>

namespace {
    // Set on the render thread (button), consumed on the window thread (next key press)
    std::atomic<Module*> g_binding = nullptr;   // module waiting for a new key

    char g_command[128] = "";   // render thread only
    // Set on the window thread (Home / Esc), handled on the render thread
    std::atomic<bool> g_focusCommand = false;
    std::atomic<bool> g_closeCommandRequested = false;

    bool keyHeld(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

    void renderCommandBar() {
        // The game reads keys through GameInput, not window messages. If we closed while
        // Enter/Esc is still down, the game would see it (Esc = pause menu), so wait for the release.
        if (g_closeCommandRequested && !keyHeld(VK_RETURN) && !keyHeld(VK_ESCAPE)) {
            LOG("command bar: closed");
            InputFocus::commandBarOpen = false;
            g_closeCommandRequested = false;
        }
        if (!InputFocus::commandBarOpen) return;

        ImGui::SetNextWindowPos(ImVec2(20, 20));
        ImGui::SetNextWindowSize(ImVec2(420, 0));
        ImGui::Begin("##command", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextDisabled("コマンド (Enter: 実行 / Esc: 閉じる / help: 一覧)");
        if (g_focusCommand.exchange(false)) {
            g_command[0] = 0;   // a fresh, empty bar each time it opens
            // Forget keys ImGui may still think are held from before (e.g. the Enter that
            // closed the bar last time) - a "held" Enter would submit the empty box at once.
            ImGui::GetIO().ClearInputKeys();
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##input", g_command, sizeof(g_command), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (g_command[0]) {
                CommandManager::execute(g_command);
                Menu::closeCommandBar("Enter");
            } else {
                ImGui::SetKeyboardFocusHere(-1);   // empty: stay open and keep typing
            }
        }
        ImGui::End();
    }

    void renderLastMessage() {
        unsigned long long age = 0;
        const std::string message = CommandManager::lastMessage(&age);
        if (message.empty() || age > 5000) return;
        ImGui::SetNextWindowPos(ImVec2(20, InputFocus::commandBarOpen ? 90.0f : 20.0f));
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("##message", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::TextUnformatted(message.c_str());
        ImGui::End();
    }

    void keyName(int vk, char* out, int size) {
        if (!vk) { snprintf(out, size, "なし"); return; }
        UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        // Extended keys (Insert, Delete, arrows, ...) need bit 24 set for GetKeyNameText
        switch (vk) {
        case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
            scan |= 0x100; break;
        }
        if (!GetKeyNameTextA(static_cast<LONG>(scan << 16), out, size))
            snprintf(out, size, "VK 0x%02X", vk);
    }
}

void Menu::loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // don't write imgui.ini into the game folder
    const char* font = "C:\\Windows\\Fonts\\meiryo.ttc";
    if (GetFileAttributesA(font) != INVALID_FILE_ATTRIBUTES)
        io.Fonts->AddFontFromFileTTF(font, 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    else
        io.Fonts->AddFontDefault();
}

bool Menu::onKeyForBinding(int vk) {
    Module* module = g_binding.exchange(nullptr);
    if (!module) return false;
    module->setKey(vk == VK_ESCAPE ? 0 : vk);   // Esc clears the bind
    return true;
}

void Menu::openCommandBar() {
    if (InputFocus::commandBarOpen) return;   // Home only opens; Esc / Enter close
    LOG("command bar: opened");
    g_closeCommandRequested = false;
    g_focusCommand = true;     // the render thread clears the text and focuses the box
    InputFocus::commandBarOpen = true;
}

void Menu::closeCommandBar(const char* reason) {
    if (!InputFocus::commandBarOpen || g_closeCommandRequested) return;
    LOG("command bar: close requested (%s)", reason);
    g_closeCommandRequested = true;
}

void Menu::render() {
    ModuleManager::render();
    renderCommandBar();
    renderLastMessage();

    // Save when the menu closes, so changes survive even if the game crashes before End
    static bool wasOpen = false;
    const bool isOpen = InputFocus::menuOpen;
    if (wasOpen && !isOpen) Config::save();
    wasOpen = isOpen;
    if (!isOpen) return;
    ImGui::SetNextWindowSize(ImVec2(420, 360), ImGuiCond_FirstUseEver);
    bool keepOpen = true;   // the window's close button
    ImGui::Begin("PotatoClient  (Insert: 閉じる / End: アンロード)", &keepOpen);

    for (auto& m : ModuleManager::modules()) {
        ImGui::PushID(m.get());
        bool enabled = m->isEnabled();
        ImGui::BeginDisabled(!m->isAvailable());
        if (ImGui::Checkbox(m->name().c_str(), &enabled)) m->setEnabled(enabled);
        ImGui::EndDisabled();

        ImGui::SameLine(200);
        char label[64];
        if (g_binding == m.get()) snprintf(label, sizeof(label), "キーを押して… (Escで解除)");
        else { char k[32]; keyName(m->key(), k, sizeof(k)); snprintf(label, sizeof(label), "キー: %s", k); }
        if (ImGui::Button(label)) g_binding = m.get();

        ImGui::TextDisabled("%s", m->description().c_str());
        if (!m->isAvailable())
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "シグネチャ未検出のため無効 (client.log を確認)");
        if (ImGui::TreeNode("設定")) { m->renderSettings(); ImGui::TreePop(); }
        ImGui::Separator();
        ImGui::PopID();
    }
    ImGui::End();
    if (!keepOpen) InputFocus::menuOpen = false;
}
