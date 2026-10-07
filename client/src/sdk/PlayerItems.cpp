#include "PlayerItems.h"
#include "core/Logger.h"
#include <Windows.h>
#include <atomic>
#include <cstring>
#include <mutex>

/*
 * Inventory: Actor+0x5B8 -> PlayerInventory -> +0xB8 -> Inventory (a Container).
 * Same path Actor::getCarriedItem (vtable 77) takes for the selected slot.
 *
 * Off hand: it is not in the inventory but in a separate 2-slot "hand container"
 * that ActorEquipment::getHandContainer(EntityContext&) returns. That function has
 * no signature of its own, but Actor::getEquippedTotem (vtable 79) starts with
 *     mov rsi, rcx / add rcx, 8 / call ActorEquipment::getHandContainer
 * (Actor+8 is the EntityContext), so we read the call target from there.
 */

namespace {
    using GetHandContainer_t = Container* (*)(void* entityContext);
    std::atomic<GetHandContainer_t> g_getHandContainer = nullptr;
    std::mutex g_searchMutex;
    ULONGLONG g_nextSearch = 0;   // guarded by g_searchMutex

    GetHandContainer_t findGetHandContainer(Actor& player) {
        const auto& pattern = Offsets::Sig::getHandContainerCall;
        void** vtable = *reinterpret_cast<void***>(&player);
        const auto* fn = static_cast<const uint8_t*>(vtable[Offsets::Actor::VIndex::getEquippedTotem]);

        uint8_t code[Offsets::Sig::getHandContainerSearchSize];
        if (!Memory::safeRead(fn, code, sizeof(code))) return nullptr;
        for (size_t i = 0; i + sizeof(pattern) + 4 <= sizeof(code); i++) {
            if (memcmp(code + i, pattern, sizeof(pattern)) != 0) continue;
            const uintptr_t call = reinterpret_cast<uintptr_t>(fn) + i + sizeof(pattern) - 1;   // the E8 call
            const uintptr_t target = Memory::resolveRel32(call, 1, 5);
            LOG("ActorEquipment::getHandContainer at exe+%#llx", Memory::rva(target));
            return reinterpret_cast<GetHandContainer_t>(target);
        }
        LOG("getHandContainer not found in Actor::getEquippedTotem (game updated?)");
        return nullptr;
    }

    // Found once and then cached. Both game threads can ask at the same time, so the search
    // is serialized; a failed search is retried after a while instead of on every tick.
    GetHandContainer_t handContainerFunction(Actor& player) {
        if (GetHandContainer_t fn = g_getHandContainer) return fn;
        std::lock_guard lock(g_searchMutex);
        if (GetHandContainer_t fn = g_getHandContainer) return fn;   // found while we waited
        const ULONGLONG now = GetTickCount64();
        if (now < g_nextSearch) return nullptr;
        g_nextSearch = now + 10000;
        GetHandContainer_t fn = findGetHandContainer(player);
        g_getHandContainer = fn;
        return fn;
    }

    bool isValidInventory(Container* c) {
        return c && c->size() == PlayerItems::inventorySize;
    }
}

Container* PlayerItems::inventory(Actor& player) {
    auto* playerInventory = player.at<uint8_t*>(Offsets::Actor::playerInventory);
    Container* container = nullptr;
    // safeRead: on a non-player actor (or after a game update) these pointers can be garbage
    if (!playerInventory ||
        !Memory::safeRead(playerInventory + Offsets::PlayerInventory::container, &container, sizeof(container)))
        return nullptr;
    return isValidInventory(container) ? container : nullptr;
}

int PlayerItems::selectedSlot(Actor& player) {
    auto* playerInventory = player.at<uint8_t*>(Offsets::Actor::playerInventory);
    int slot = 0;
    if (playerInventory) Memory::safeRead(playerInventory + Offsets::PlayerInventory::selected, &slot, sizeof(slot));
    return slot >= 0 && slot < hotbarSize ? slot : 0;
}

Container* PlayerItems::hands(Actor& player) {
    GetHandContainer_t fn = handContainerFunction(player);
    if (!fn) return nullptr;
    return fn(reinterpret_cast<uint8_t*>(&player) + Offsets::Actor::entityContext);
}

ItemStack* PlayerItems::offhand(Actor& player) {
    Container* c = hands(player);
    return c ? c->getItem(Offsets::Container::offhandSlot) : nullptr;
}

bool PlayerItems::isTotem(ItemStack* stack) {
    return stack && !stack->isEmpty() && strcmp(stack->name(), "minecraft:totem_of_undying") == 0;
}

int PlayerItems::findTotem(Container& inventory) {
    for (int slot = 0; slot < inventorySize; slot++)
        if (isTotem(inventory.getItem(slot))) return slot;
    return -1;
}

int PlayerItems::firstEmpty(Container& inventory) {
    for (int slot = 0; slot < inventorySize; slot++) {
        ItemStack* stack = inventory.getItem(slot);
        if (stack && stack->isEmpty()) return slot;
    }
    return -1;
}
