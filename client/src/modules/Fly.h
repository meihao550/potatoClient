#pragma once
#include "Module.h"

// Creative-like flight: WASD moves, Space goes up, Shift goes down, no keys = hover
class Fly : public Module {
public:
    Fly() : Module("Fly", "飛行 (WASD: 移動 / Space: 上昇 / Shift: 下降 / 何も押さない: その場で停止)", 'F') {}
    void onTick(Actor& player) override;
    void renderSettings() override;

private:
    float m_speed = 1.0f;          // blocks per tick (x20 = blocks per second)
    float m_verticalSpeed = 0.5f;
    bool m_antiKick = false;       // sink slowly while hovering
};
