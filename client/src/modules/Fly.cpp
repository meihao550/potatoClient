#include "Fly.h"
#include "MoveInput.h"
#include "sdk/Actor.h"
#include <imgui.h>

/*
 * Fly
 * ---
 * StateVectorComponent::posDelta is the player's velocity (blocks per tick). Each tick the
 * game moves the player by it (with collisions), then applies gravity and drag to it.
 * We overwrite it right after the tick, so the next tick moves us exactly by our value:
 * no gravity, no drag. On a remote server the client reports the resulting position.
 */

void Fly::onTick(Actor& player) {
    StateVectorComponent* state = player.stateVector();
    ActorRotationComponent* rotation = player.rotation();
    if (!state || !rotation) return;

    float x = 0, z = 0;
    MoveInput::direction(rotation->yaw, x, z);
    state->posDelta.x = x * m_speed;
    state->posDelta.z = z * m_speed;

    if (MoveInput::up()) state->posDelta.y = m_verticalSpeed;
    else if (MoveInput::down()) state->posDelta.y = -m_verticalSpeed;
    else state->posDelta.y = m_antiKick ? -0.04f : 0.0f;
}

void Fly::renderSettings() {
    ImGui::SliderFloat("横の速さ", &m_speed, 0.1f, 5.0f, "%.2f ブロック/tick");
    ImGui::SliderFloat("縦の速さ", &m_verticalSpeed, 0.1f, 3.0f, "%.2f ブロック/tick");
    ImGui::Checkbox("アンチキック (止まっている間ゆっくり下がる)", &m_antiKick);
    ImGui::TextDisabled("1 ブロック/tick = 20 ブロック/秒。普通のダッシュは約 0.28");
}
