#pragma once
#include "Module.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Block;
class BlockType;

class Xray : public Module {
public:
    Xray();
    bool onEnable() override;
    void onDisable() override;
    void renderSettings() override;

private:
    struct Group {                     // one row in the menu, e.g. "ダイヤ" -> {"diamond_ore"}
        const char* label;
        std::vector<std::string> patterns;   // substrings of the block name
        bool visible;
    };
    struct OriginalState {             // cached copy inside each Block permutation
        Block* block;
        uint8_t isOpaqueFullBlock, light;
        uint16_t occlusionShapes[7];
    };
    struct Original {                  // a block's real values, captured before we ever touch it
        std::string name;
        float translucency;
        uint8_t renderLayer, flags2, lightBlock;
        std::vector<OriginalState> states;
    };

    static Original capture(BlockType* b, const std::string& name);
    static void restore(BlockType* b, const Original& o);
    void hide(BlockType* b, const Original& o) const;

    bool isVisible(const std::string& name) const;
    bool apply();                      // patch/unpatch every block according to the groups
    void restoreAll();

    std::vector<Group> m_groups;
    std::vector<std::string> m_custom;
    std::unordered_map<BlockType*, Original> m_originals;
    bool m_letLightThrough = true;
    std::string m_status;
    std::mutex m_mutex;
};
