#include "PlayerItems.h"
#include "core/Logger.h"
#include <atomic>
#include <cstring>

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
    std::atomic<bool> g_searched = false;

    GetHandContainer_t findGetHandContainer(Actor& player) {
        static const uint8_t pattern[] = { 0x48, 0x89, 0xCE, 0x48, 0x83, 0xC1, 0x08, 0xE8 };
        void** vtable = *reinterpret_cast<void***>(&player);
        const auto* fn = static_cast<const uint8_t*>(vtable[Offsets::Actor::VIndex::getEquippedTotem]);

        uint8_t code[0x40];
        if (!Memory::safeRead(fn, code, sizeof(code))) return nullptr;
        for (size_t i = 0; i + sizeof(pattern) + 4 <= sizeof(code); i++) {
            if (memcmp(code + i, pattern, sizeof(pattern)) != 0) continue;
            const uintptr_t call = reinterpret_cast<uintptr_t>(fn) + i + 7;   // the E8 call
            const uintptr_t target = Memory::resolveRel32(call, 1, 5);
            LOG("ActorEquipment::getHandContainer at exe+%#llx",
                static_cast<unsigned long long>(target - Memory::moduleBase()));
            return reinterpret_cast<GetHandContainer_t>(target);
        }
        LOG("getHandContainer not found in Actor::getEquippedTotem (game updated?)");
        return nullptr;
    }

    bool isValidInventory(Container* c) {
        return c && c->size() == PlayerItems::inventorySize;
    }
}

Container* PlayerItems::inventory(Actor& player) {
    auto* playerInventory = player.at<uint8_t*>(Offsets::Actor::playerInventory);
    if (!playerInventory) return nullptr;
    auto* container = *reinterpret_cast<Container**>(playerInventory + Offsets::PlayerInventory::container);
    return isValidInventory(container) ? container : nullptr;
}

int PlayerItems::selectedSlot(Actor& player) {
    auto* playerInventory = player.at<uint8_t*>(Offsets::Actor::playerInventory);
    if (!playerInventory) return 0;
    return *reinterpret_cast<int*>(playerInventory + Offsets::PlayerInventory::selected);
}

Container* PlayerItems::hands(Actor& player) {
    if (!g_searched.exchange(true)) g_getHandContainer = findGetHandContainer(player);
    GetHandContainer_t fn = g_getHandContainer;
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
