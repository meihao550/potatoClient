#include "InventoryView.h"
#include "gui/Menu.h"
#include "sdk/PlayerItems.h"
#include <imgui.h>
#include <cstdio>
#include <cstring>

/*
 * Inventory
 * ---------
 * Reads the LocalPlayer's own inventory (the client copy), so it also works on
 * remote servers. Layout like the game: main inventory (9..35) on top, hotbar
 * (0..8) below, the selected hotbar slot highlighted.
 */

namespace {
    void readSlot(ItemStack* stack, std::string& name, int& count) {
        if (!stack || stack->isEmpty()) { name.clear(); count = 0; return; }
        const char* full = stack->name();
        if (strncmp(full, "minecraft:", 10) == 0) full += 10;
        name = full;
        count = stack->count();
    }

    // "diamond_sword" -> "diamond sword"
    std::string displayName(const std::string& name) {
        std::string out = name;
        for (char& c : out) if (c == '_') c = ' ';
        return out;
    }
}

void InventoryView::onTick(Actor& player) {
    Container* inventory = PlayerItems::inventory(player);
    if (!inventory) return;

    Snapshot snap;
    snap.time = GetTickCount64();
    snap.selected = PlayerItems::selectedSlot(player);
    for (int i = 0; i < PlayerItems::inventorySize; i++)
        readSlot(inventory->getItem(i), snap.slots[i].name, snap.slots[i].count);
    readSlot(PlayerItems::offhand(player), snap.offhand.name, snap.offhand.count);

    std::lock_guard lock(m_mutex);
    m_snap = std::move(snap);
}

void InventoryView::onRender() {
    Snapshot snap;
    {
        std::lock_guard lock(m_mutex);
        snap = m_snap;
    }
    if (GetTickCount64() - snap.time > 1000) return;   // paused / left the world

    // Only clickable/movable while the menu is open, otherwise mouse goes to the game
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings;
    if (!Menu::open) flags |= ImGuiWindowFlags_NoInputs;

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screen.x - 20, 20), ImGuiCond_FirstUseEver, ImVec2(1, 0));
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("インベントリ", nullptr, flags);

    const ImVec2 cell(m_cellWidth, m_cellWidth * 0.6f);
    auto drawSlot = [&](const Slot& slot, bool selected, int id) {
        ImGui::PushID(id);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.7f, 0.1f, 0.8f));
        else if (slot.name.empty()) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
        else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.35f, 0.5f, 0.8f));

        char label[96] = "";
        if (!slot.name.empty())
            snprintf(label, sizeof(label), "%s\n%d", displayName(slot.name).c_str(), slot.count);
        ImGui::Button(label, cell);
        if (!slot.name.empty() && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s x%d", slot.name.c_str(), slot.count);

        ImGui::PopStyleColor();
        ImGui::PopID();
    };

    ImGui::SetWindowFontScale(0.8f);
    for (int row = 1; row <= 3; row++) {   // main inventory 9..35
        for (int col = 0; col < PlayerItems::hotbarSize; col++) {
            if (col) ImGui::SameLine();
            const int i = row * PlayerItems::hotbarSize + col;
            drawSlot(snap.slots[i], false, i);
        }
    }
    ImGui::Separator();
    for (int col = 0; col < PlayerItems::hotbarSize; col++) {   // hotbar 0..8
        if (col) ImGui::SameLine();
        drawSlot(snap.slots[col], col == snap.selected, col);
    }
    if (m_showOffhand) {
        ImGui::Separator();
        ImGui::TextUnformatted("オフハンド");
        ImGui::SameLine();
        drawSlot(snap.offhand, false, 100);
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void InventoryView::onDisable() {
    std::lock_guard lock(m_mutex);
    m_snap = {};
}

void InventoryView::renderSettings() {
    ImGui::SliderFloat("マスの大きさ", &m_cellWidth, 40.0f, 120.0f, "%.0f px");
    ImGui::Checkbox("オフハンドも表示", &m_showOffhand);
    ImGui::TextDisabled("メニューを開いている間はウィンドウを動かせる / マスにカーソルで名前");
}
