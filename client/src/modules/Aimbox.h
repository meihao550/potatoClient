#pragma once
#include "Module.h"
#include "sdk/Actor.h"
#include <mutex>
#include <vector>

// Draws the hitbox of nearby mobs / players. Positions are collected on the game
// thread (onTick) and drawn on the render thread (onRender) from a copy.
class Aimbox : public Module {
public:
    Aimbox() : Module("Aimbox", "モブ・プレイヤーの当たり判定を箱で表示", 'B') {}
    void onTick(Actor& player) override;
    void onRender() override;
    void onDisable() override;
    void renderSettings() override;

private:
    struct Snapshot {
        unsigned long long time = 0;   // GetTickCount64 when collected
        Vec3 eye{};
        float yaw = 0, pitch = 0;
        std::vector<AABB> boxes;
    };
    std::mutex m_mutex;
    Snapshot m_snap;

    float m_fov = 70.0f;        // match the game's FOV setting
    float m_range = 64.0f;      // blocks
    bool m_skipSmall = true;    // items, XP orbs
    float m_color[4] = { 1.0f, 0.25f, 0.25f, 1.0f };
};