#pragma once
#include "Module.h"
#include "sdk/Actor.h"
#include <mutex>
#include <vector>

// Draws the hitbox of nearby mobs / players. Positions are collected on the game
// thread (onTick) and drawn on the render thread (onRender) from a copy.
class Aimbox : public Module {
public:
    Aimbox() : Module("Aimbox", "モブ・プレイヤーの当たり判定を箱で表示", Category::Render, 'B') {
        addSettings({ &m_fov, &m_range, &m_skipSmall, &m_color });
    }
    void onTick(Actor& player) override;
    void onRender() override;
    void onDisable() override;

private:
    struct Snapshot {
        unsigned long long time = 0;   // GetTickCount64 when collected
        Vec3 eye{};
        float yaw = 0, pitch = 0;
        std::vector<AABB> boxes;
    };
    std::mutex m_mutex;
    Snapshot m_snap;

    FloatSetting m_fov{ "fov", "FOV", 70.0f, 30.0f, 110.0f, "%.0f" };   // match the game's FOV setting
    FloatSetting m_range{ "range", "距離", 64.0f, 8.0f, 128.0f, "%.0f ブロック" };
    BoolSetting m_skipSmall{ "skipSmall", "小さいもの (アイテム・経験値) を除く", true };   // items, XP orbs
    ColorSetting m_color{ "color", "色", 1.0f, 0.25f, 0.25f, 1.0f, "箱がずれるときは FOV をゲームの設定に合わせる" };
};
