#pragma once
#include "Module.h"

// Creative-like flight: WASD moves, Space goes up, Shift goes down, no keys = hover
class Fly : public Module {
public:
    Fly() : Module("Fly", "飛行 (WASD: 移動 / Space: 上昇 / Shift: 下降 / 何も押さない: その場で停止)", 'F') {
        addSettings({ &m_speed, &m_verticalSpeed, &m_antiKick });
    }
    void onTick(Actor& player) override;

private:
    // blocks per tick (x20 = blocks per second)
    FloatSetting m_speed{ "speed", "横の速さ", 1.0f, 0.1f, 5.0f, "%.2f ブロック/tick" };
    FloatSetting m_verticalSpeed{ "verticalSpeed", "縦の速さ", 0.5f, 0.1f, 3.0f, "%.2f ブロック/tick",
                                  "1 ブロック/tick = 20 ブロック/秒。普通のダッシュは約 0.28" };
    BoolSetting m_antiKick{ "antiKick", "アンチキック (止まっている間ゆっくり下がる)", false };
};
