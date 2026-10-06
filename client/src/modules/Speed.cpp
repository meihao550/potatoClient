#include "Speed.h"
#include "MoveInput.h"
#include "sdk/Actor.h"
#include <imgui.h>

// Same trick as Fly (overwrite posDelta after the tick), but only x/z:
// gravity, jumping and falling stay as the game computes them.
void Speed::onTick(Actor& player) {
    StateVectorComponent* state = player.stateVector();
    ActorRotationComponent* rotation = player.rotation();
    if (!state || !rotation) return;

    float x = 0, z = 0;
    if (!MoveInput::direction(rotation->yaw, x, z)) return;   // no keys: let the game slow us down
    state->posDelta.x = x * m_speed;
    state->posDelta.z = z * m_speed;
}

void Speed::renderSettings() {
    ImGui::SliderFloat("速さ", &m_speed, 0.1f, 3.0f, "%.2f ブロック/tick");
    ImGui::TextDisabled("普通の歩き 約 0.22 / ダッシュ 約 0.28");
}
