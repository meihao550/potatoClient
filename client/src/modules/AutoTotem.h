#pragma once
#include "Module.h"
#include <atomic>

// Keeps a Totem of Undying in the off hand: when the off hand has none and the
// inventory does, the totem is moved there. Single-player worlds only (the real
// inventory is on the built-in server).
class AutoTotem : public Module {
public:
    AutoTotem() : Module("AutoTotem", "不死のトーテムを自動でオフハンドに持つ (シングルプレイ専用)", 0) {
        addSettings({ &m_swapOffhand });
    }
    void onTick(Actor& player) override;
    void renderSettings() override;

private:
    // A server task is queued until then (GetTickCount64). A time, not a bool, because
    // PlayerTick drops tasks that find no player within 2 s and we'd never hear back.
    std::atomic<unsigned long long> m_busyUntil = 0;
    // off hand holds something else: move it into the inventory
    BoolSetting m_swapOffhand{ "swapOffhand", "オフハンドに別のものがあっても入れ替える", true,
                               "入れ替えたものはインベントリの空きマスへ (空きがないときは何もしない)" };
    std::atomic<int> m_equipped = 0;         // how many times we equipped one
};
