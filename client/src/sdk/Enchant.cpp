#include "Enchant.h"
#include "core/Logger.h"
#include "core/Util.h"

/*
 * Enchanting with the game's own code
 * -----------------------------------
 * Enchants live in the item's NBT (ItemStack::mUserData -> "ench" list). Building
 * CompoundTags by hand would mean allocating game objects ourselves, so instead we
 * call the function /enchant uses:
 *
 *   bool EnchantUtils::applyEnchant(ItemStackBase& item, EnchantmentInstance const& e, bool allowNonVanilla)
 *
 * EnchantmentInstance is 8 bytes: { uint8 type; int level; }. The function checks
 * that the item can take the enchant (sword vs. boots, Sharpness vs. Smite, ...)
 * and writes it into the NBT, so we get the same rules as the vanilla command.
 */

namespace {
    struct EnchantmentInstance {
        uint8_t type;
        int level;
    };
    using ApplyEnchant_t = bool (*)(ItemStack* item, const EnchantmentInstance* enchant, bool allowNonVanilla);
    ApplyEnchant_t g_applyEnchant = nullptr;

    const std::vector<Enchant::Info> g_enchants = {
        { "protection",            "ダメージ軽減",         0,  4 },
        { "fire_protection",       "火炎耐性",             1,  4 },
        { "feather_falling",       "落下耐性",             2,  4 },
        { "blast_protection",      "爆発耐性",             3,  4 },
        { "projectile_protection", "飛び道具耐性",         4,  4 },
        { "thorns",                "棘の鎧",               5,  3 },
        { "respiration",           "水中呼吸",             6,  3 },
        { "depth_strider",         "水中歩行",             7,  3 },
        { "aqua_affinity",         "水中採掘",             8,  1 },
        { "sharpness",             "ダメージ増加",         9,  5 },
        { "smite",                 "アンデッド特効",       10, 5 },
        { "bane_of_arthropods",    "虫特効",               11, 5 },
        { "knockback",             "ノックバック",         12, 2 },
        { "fire_aspect",           "火属性",               13, 2 },
        { "looting",               "ドロップ増加",         14, 3 },
        { "efficiency",            "効率強化",             15, 5 },
        { "silk_touch",            "シルクタッチ",         16, 1 },
        { "unbreaking",            "耐久力",               17, 3 },
        { "fortune",               "幸運",                 18, 3 },
        { "power",                 "射撃ダメージ増加",     19, 5 },
        { "punch",                 "パンチ",               20, 2 },
        { "flame",                 "フレイム",             21, 1 },
        { "infinity",              "無限",                 22, 1 },
        { "luck_of_the_sea",       "宝釣り",               23, 3 },
        { "lure",                  "入れ食い",             24, 3 },
        { "frost_walker",          "氷渡り",               25, 2 },
        { "mending",               "修繕",                 26, 1 },
        { "binding",               "束縛の呪い",           27, 1 },
        { "vanishing",             "消滅の呪い",           28, 1 },
        { "impaling",              "水生特効",             29, 5 },
        { "riptide",               "激流",                 30, 3 },
        { "loyalty",               "忠誠",                 31, 3 },
        { "channeling",            "召雷",                 32, 1 },
        { "multishot",             "拡散",                 33, 1 },
        { "piercing",              "貫通",                 34, 4 },
        { "quick_charge",          "高速装填",             35, 3 },
        { "soul_speed",            "ソウルスピード",       36, 3 },
        { "swift_sneak",           "スニーク速度上昇",     37, 3 },
        { "wind_burst",            "風爆",                 38, 3 },
        { "density",               "密度",                 39, 5 },
        { "breach",                "防具貫通",             40, 4 },
        { "lunge",                 "突進",                 41, 3 },
    };

    // Order matters: when two enchants exclude each other the first one wins
    // (Mending over Infinity, Fortune over Silk Touch, Loyalty over Riptide, ...).
    const std::vector<uint8_t> g_bestSet = {
        26, 17,                      // mending, unbreaking (everything)
        9, 14, 13, 12, 40, 39, 38,   // sharpness, looting, fire aspect, knockback, breach, density, wind burst
        15, 18,                      // efficiency, fortune
        0, 5, 6, 8, 2, 7, 36, 37,    // protection, thorns, respiration, aqua affinity, feather falling, depth strider, soul speed, swift sneak
        19, 20, 21,                  // power, punch, flame
        23, 24,                      // luck of the sea, lure
        29, 31, 32,                  // impaling, loyalty, channeling
        33, 35,                      // multishot, quick charge
        41,                          // lunge
    };
}

bool Enchant::init() {
    g_applyEnchant = reinterpret_cast<ApplyEnchant_t>(Memory::findSig(Offsets::Sig::applyEnchant));
    LOG("EnchantUtils::applyEnchant at exe+%#llx", g_applyEnchant
        ? static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(g_applyEnchant) - Memory::moduleBase()) : 0ull);
    return g_applyEnchant != nullptr;
}

const std::vector<Enchant::Info>& Enchant::all() { return g_enchants; }
const std::vector<uint8_t>& Enchant::bestSet() { return g_bestSet; }

const Enchant::Info* Enchant::find(const std::string& name) {
    const std::string key = Util::toLower(name);
    for (const Info& e : g_enchants)
        if (key == e.name || name == e.japanese) return &e;
    return nullptr;
}

bool Enchant::apply(ItemStack& stack, uint8_t id, int level) {
    if (!g_applyEnchant || !stack.item()) return false;
    const EnchantmentInstance enchant{ id, level };
    return g_applyEnchant(&stack, &enchant, false);
}
