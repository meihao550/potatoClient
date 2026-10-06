#pragma once
#include "Actor.h"

// Where a player's items live. Game thread only (onTick / PlayerTick tasks).
namespace PlayerItems {
    constexpr int hotbarSize = 9;
    constexpr int inventorySize = 36;   // 0..8 hotbar, 9..35 main inventory

    // The 36-slot inventory, nullptr if not a player / not readable
    Container* inventory(Actor& player);
    // Selected hotbar slot (0..8)
    int selectedSlot(Actor& player);
    // The two hands (slot 0 = main hand, 1 = off hand), nullptr if not found
    Container* hands(Actor& player);
    // The off hand stack (an empty stack when nothing is held), nullptr if not found
    ItemStack* offhand(Actor& player);

    bool isTotem(ItemStack* stack);
    // First slot of the inventory that holds a totem / nothing, -1 if none
    int findTotem(Container& inventory);
    int firstEmpty(Container& inventory);
}
