#include "Xray.h"
#include "core/Logger.h"
#include "sdk/BlockRegistry.h"
#include <imgui.h>
#include <cstring>

/*
 * Xray by data patching
 * ---------------------
 * Every block kind ("minecraft:stone", ...) is one BlockType object in the
 * BlockTypeRegistry. The chunk mesher reads these fields when it builds geometry.
 * minecraft:barrier is already an invisible block, so we make every hidden
 * block look like a barrier:
 *   mRenderLayer       -> Barrier (hidden unless you hold a barrier item)
 *   mIsOpaqueFullBlock -> false   (neighbours stop culling the faces touching it,
 *                                  so ore faces buried in stone get drawn)
 *   mTranslucency      -> 1.0, mLightBlock -> 0 (optional: light passes through)
 * Each Block (one state of a BlockType) also caches mIsOpaqueFullBlock, mLight and
 * its occlusion shapes, which is what face culling really reads - those get the
 * barrier's values (0) too.
 * mSolid is left alone so collision doesn't change.
 * Existing chunk meshes only pick this up when they are rebuilt: toggling Smooth Lighting
 * rebuilds every chunk (changing render distance only rebuilds chunks entering range).
 *
 * The real values of each block are captured exactly once (m_originals), so
 * toggling or editing the list never mistakes an already-patched value for the original.
 */

namespace {
    namespace Flags2 = Offsets::BlockType::Flags2;
    constexpr const char* kRebuildHint = "設定→ビデオの「スムーズライティング」を一度切り替えると全チャンクに反映されます";
}

Xray::Xray()
    : Module("Xray", "選んだ鉱石以外のブロックを透明にする", 'X'),
      m_groups{
          { "ダイヤモンド",     { "diamond_ore" },                    true },
          { "エメラルド",       { "emerald_ore" },                    true },
          { "金",               { "gold_ore" },                       true },   // incl. nether_gold_ore
          { "鉄",               { "iron_ore" },                       true },
          { "銅",               { "copper_ore" },                     false },
          { "石炭",             { "coal_ore" },                       false },
          { "レッドストーン",   { "redstone_ore" },                   true },   // incl. lit_ variants
          { "ラピスラズリ",     { "lapis_ore" },                      true },
          { "ネザークォーツ",   { "quartz_ore" },                     false },
          { "古代の残骸",       { "ancient_debris" },                 true },
          { "スポナー",         { "mob_spawner", "trial_spawner" },   true },
          { "チェスト",         { "chest", "barrel" },                true },
          { "溶岩",             { "lava" },                           true },
          { "水",               { "water" },                          false },
      } {}

Xray::Original Xray::capture(BlockType* b, const std::string& name) {
    Original o{ name, b->translucency(), b->renderLayer(), b->flags2(), b->lightBlock(), {} };
    for (Block* state : b->permutations()) {
        OriginalState s{ state, state->isOpaqueFullBlock(), state->light(), {} };
        memcpy(s.occlusionShapes, state->occlusionShapes(), sizeof(s.occlusionShapes));
        o.states.push_back(s);
    }
    return o;
}

void Xray::restore(BlockType* b, const Original& o) {
    b->renderLayer() = o.renderLayer;
    b->flags2() = o.flags2;
    b->translucency() = o.translucency;
    b->lightBlock() = o.lightBlock;
    for (const OriginalState& s : o.states) {
        s.block->isOpaqueFullBlock() = s.isOpaqueFullBlock;
        s.block->light() = s.light;
        memcpy(s.block->occlusionShapes(), s.occlusionShapes, sizeof(s.occlusionShapes));
    }
}

void Xray::hide(BlockType* b, const Original& o) const {
    // BlockType: drawn on the barrier layer, no longer a full opaque cube
    b->renderLayer() = Offsets::RenderLayer::Barrier;
    b->flags2() = static_cast<uint8_t>((o.flags2 & ~Flags2::isOpaqueFullBlock) | Flags2::ignoreForInsideCube);
    b->translucency() = 1.0f;
    b->lightBlock() = m_letLightThrough ? 0 : o.lightBlock;
    // Each Block permutation caches the culling data; make it match minecraft:barrier
    // (occlusion shape 0 = empty, so faces of neighbouring ores are not culled)
    for (const OriginalState& s : o.states) {
        s.block->isOpaqueFullBlock() = 0;
        s.block->light() = m_letLightThrough ? 0 : s.light;
        memset(s.block->occlusionShapes(), 0, sizeof(s.occlusionShapes));
    }
}

bool Xray::isVisible(const std::string& name) const {
    auto matches = [&](const std::string& p) { return !p.empty() && name.find(p) != std::string::npos; };
    for (const auto& g : m_groups)
        if (g.visible)
            for (const auto& p : g.patterns)
                if (matches(p)) return true;
    for (const auto& p : m_custom)
        if (matches(p)) return true;
    return false;
}

bool Xray::apply() {
    const auto blocks = BlockRegistry::all();
    if (blocks.empty()) {
        m_status = "ブロック一覧が見つかりません。ワールドに入ってから有効にしてください";
        LOG("xray: %s", m_status.c_str());
        return false;
    }

    size_t hidden = 0;
    for (BlockType* b : blocks) {
        const std::string name = b->name();
        if (name == "minecraft:air" || name == "minecraft:barrier") continue;

        auto it = m_originals.find(b);
        if (it != m_originals.end() && it->second.name != name) {   // pointer reused after a world change
            m_originals.erase(it);
            it = m_originals.end();
        }

        if (isVisible(name)) {
            if (it != m_originals.end()) restore(b, it->second);   // was hidden before -> put it back
            continue;
        }

        if (it == m_originals.end()) it = m_originals.emplace(b, capture(b, name)).first;
        hide(b, it->second);
        ++hidden;
    }
    m_status = std::to_string(hidden) + " 種類を透明化。" + kRebuildHint;
    LOG("xray: hid %zu of %zu block types", hidden, blocks.size());
    return true;
}

void Xray::restoreAll() {
    // Only touch BlockTypes that are still registered under the same name:
    // after a world change the old objects are freed and writing to them would corrupt the heap.
    size_t restored = 0;
    for (BlockType* b : BlockRegistry::all()) {
        auto it = m_originals.find(b);
        if (it == m_originals.end() || b->name() != it->second.name) continue;
        restore(b, it->second);
        ++restored;
    }
    m_status = "元に戻しました。" + std::string(kRebuildHint);
    LOG("xray: restored %zu block types", restored);
}

bool Xray::onEnable() {
    std::lock_guard lock(m_mutex);
    return apply();
}

void Xray::onDisable() {
    std::lock_guard lock(m_mutex);
    restoreAll();
}

void Xray::renderSettings() {
    // The settings below are read by apply(), which can run on another thread (key bind -> onEnable)
    std::lock_guard lock(m_mutex);
    bool changed = false;
    if (!m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
    changed |= ImGui::Checkbox("光を通す (鉱石が暗くならない)", &m_letLightThrough);

    ImGui::SeparatorText("表示する鉱石");
    for (size_t i = 0; i < m_groups.size(); ++i) {
        if (i % 3) ImGui::SameLine(140.0f * (i % 3));
        changed |= ImGui::Checkbox(m_groups[i].label, &m_groups[i].visible);
    }
    if (ImGui::SmallButton("全部ON")) { for (auto& g : m_groups) g.visible = true; changed = true; }
    ImGui::SameLine();
    if (ImGui::SmallButton("全部OFF")) { for (auto& g : m_groups) g.visible = false; changed = true; }

    ImGui::SeparatorText("追加ブロック (名前の一部で一致)");
    int removeIndex = -1;
    for (int i = 0; i < static_cast<int>(m_custom.size()); ++i) {
        ImGui::PushID(i);
        if (ImGui::SmallButton("x")) removeIndex = i;
        ImGui::SameLine();
        ImGui::TextUnformatted(m_custom[i].c_str());
        ImGui::PopID();
    }
    if (removeIndex >= 0) { m_custom.erase(m_custom.begin() + removeIndex); changed = true; }

    static char input[64] = "";
    ImGui::SetNextItemWidth(200);
    const bool submitted = ImGui::InputText("##add", input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((ImGui::Button("追加") || submitted) && input[0]) {
        m_custom.emplace_back(input);
        input[0] = 0;
        changed = true;
    }

    if (changed && isEnabled()) apply();
}
