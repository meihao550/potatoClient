#include "AutoTotem.h"
#include "core/Logger.h"
#include "sdk/PlayerItems.h"
#include "sdk/PlayerTick.h"
#include <Windows.h>
#include <imgui.h>

/*
 * AutoTotem
 * ---------
 * Every client tick we look at our own (client) copy of the items. If the off hand
 * has no totem but the inventory has one, a task moves it on the server thread:
 *
 *   1. off hand holds something else -> copy it into an empty inventory slot (setItem)
 *   2. copy the totem into the off hand (Actor::setOffhandSlot)
 *   3. remove it from its inventory slot (removeItem)
 *
 * Only the game's own functions are used, so the server sends the changes to the
 * client like it does for any other inventory change. The move is done on the
 * ServerPlayer because it owns the real inventory (same reason as dupe / enchant).
 */

namespace {
    // Returns true if a totem was moved into the off hand
    bool equipTotem(Actor& player, bool swapOffhand) {
        Container* inventory = PlayerItems::inventory(player);
        ItemStack* offhand = PlayerItems::offhand(player);
        if (!inventory || !offhand || PlayerItems::isTotem(offhand)) return false;

        const int totemSlot = PlayerItems::findTotem(*inventory);
        if (totemSlot < 0) return false;

        if (!offhand->isEmpty()) {
            if (!swapOffhand) return false;
            const int freeSlot = PlayerItems::firstEmpty(*inventory);
            if (freeSlot < 0) return false;   // nowhere to put the off hand item
            inventory->setItem(freeSlot, *offhand);
        }

        ItemStack* totem = inventory->getItem(totemSlot);
        const int count = totem->count();
        player.setOffhandSlot(*totem);
        inventory->removeItem(totemSlot, count);
        return true;
    }
}

void AutoTotem::onTick(Actor& player) {
    using PlayerTick::Side;
    if (GetTickCount64() < m_busyUntil || !PlayerTick::ticking(Side::Server)) return;

    // Cheap check on the client copy first, so we only bother the server when needed
    Container* inventory = PlayerItems::inventory(player);
    ItemStack* offhand = PlayerItems::offhand(player);
    if (!inventory || !offhand || PlayerItems::isTotem(offhand)) return;
    if (PlayerItems::findTotem(*inventory) < 0) return;
    if (!offhand->isEmpty() && (!m_swapOffhand || PlayerItems::firstEmpty(*inventory) < 0)) return;

    m_busyUntil = GetTickCount64() + 2500;
    PlayerTick::runOnOwnServerPlayer([this](Actor& server) {
        if (equipTotem(server, m_swapOffhand)) {
            m_equipped++;
            LOG("AutoTotem: totem moved to the off hand");
        }
        // Give the server's update a moment to reach our client copy before checking again
        m_busyUntil = GetTickCount64() + 200;
    });
}

void AutoTotem::renderSettings() {
    bool swap = m_swapOffhand;
    if (ImGui::Checkbox("オフハンドに別のものがあっても入れ替える", &swap)) m_swapOffhand = swap;
    ImGui::TextDisabled("入れ替えたものはインベントリの空きマスへ (空きがないときは何もしない)");
    ImGui::Text("装備した回数: %d", m_equipped.load());
    if (!PlayerTick::ticking(PlayerTick::Side::Server))
        ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "シングルプレイのワールドでのみ動きます");
}
