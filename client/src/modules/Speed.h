#pragma once
#include "Module.h"

// Walk/run faster: while WASD is held, the horizontal velocity is set to the chosen speed
class Speed : public Module {
public:
    Speed() : Module("Speed", "移動を速くする (WASD を押している間)", Category::Movement, 0) {
        addSettings({ &m_speed });
    }
    void onTick(Actor& player) override;

private:
    FloatSetting m_speed{ "speed", "速さ", 0.5f, 0.1f, 3.0f, "%.2f ブロック/tick",
                          "普通の歩き 約 0.22 / ダッシュ 約 0.28" };
};
