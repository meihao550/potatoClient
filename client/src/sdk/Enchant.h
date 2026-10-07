#pragma once
#include "Actor.h"
#include <string>
#include <vector>

namespace Enchant {
    struct Info {
        const char* name;       // Bedrock id, as in /enchant ("sharpness")
        const char* japanese;   // in-game Japanese name
        uint8_t id;             // Enchant::Type
        int maxLevel;           // vanilla max level
    };

    bool init();        // finds EnchantUtils::applyEnchant (signature)
    bool available();   // false = the signature was not found, apply() can't work
    const std::vector<Info>& all();
    // English id (case-insensitive) or Japanese name. nullptr if unknown
    const Info* find(const std::string& name);
    // Ids used by "enchant all": the usual best set, skipping enchants that exclude each other
    const std::vector<uint8_t>& bestSet();

    // Adds the enchant to the stack (game code: same checks as /enchant).
    // false = this item can't take it (wrong item type, conflicts with an existing enchant, ...)
    bool apply(ItemStack& stack, uint8_t id, int level);
}
