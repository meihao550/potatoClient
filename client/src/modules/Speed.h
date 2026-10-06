#pragma once
#include "Module.h"

// Walk/run faster: while WASD is held, the horizontal velocity is set to the chosen speed
class Speed : public Module {
public:
    Speed() : Module("Speed", "移動を速くする (WASD を押している間)", 0) {}
    void onTick(Actor& player) override;
    void renderSettings() override;

private:
    float m_speed = 0.5f;   // blocks per tick
};
