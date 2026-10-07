#include "Require.h"
#include "CommandManager.h"
#include "sdk/PlayerTick.h"

bool Require::inWorld() {
    if (PlayerTick::ticking(PlayerTick::Side::Client)) return true;
    CommandManager::print("ワールドに入ってから使ってください");
    return false;
}

bool Require::ownWorld() {
    if (PlayerTick::ticking(PlayerTick::Side::Server)) return true;
    CommandManager::print("自分のワールド (シングルプレイ) でのみ使えます");
    return false;
}

std::optional<ActorRefs> Require::playerRefs(Actor& player) {
    auto refs = player.refs();
    if (!refs) CommandManager::print("プレイヤーの情報が取れません (Offsets.h を確認)");
    return refs;
}

ItemStack* Require::heldItem(Actor& player, const char* messageIfEmpty) {
    ItemStack* stack = player.getCarriedItem();
    if (stack && stack->item()) return stack;
    CommandManager::print(messageIfEmpty);
    return nullptr;
}
