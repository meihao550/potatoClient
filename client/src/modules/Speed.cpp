#include "Speed.h"
#include "MoveInput.h"
#include "sdk/Actor.h"
#include <imgui.h>

// Same trick as Fly (overwrite posDelta after the tick), but only x/z:
// gravity, jumping and falling stay as the game computes them.
void Speed::onTick(Actor& player) {
    const auto refs = player.refs();
    if (!refs) return;

    float x = 0, z = 0;
    if (!MoveInput::direction(refs->rotation.yaw, x, z)) return;   // no keys: let the game slow us down
    refs->state.posDelta.x = x * m_speed;
    refs->state.posDelta.z = z * m_speed;
}

void Speed::renderSettings() {
    ImGui::SliderFloat("速さ", &m_speed, 0.1f, 3.0f, "%.2f ブロック/tick");
    ImGui::TextDisabled("普通の歩き 約 0.22 / ダッシュ 約 0.28");
}
