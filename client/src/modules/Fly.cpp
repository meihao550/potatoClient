#include "Fly.h"
#include "MoveInput.h"
#include "sdk/Actor.h"

/*
 * Fly
 * ---
 * StateVectorComponent::posDelta is the player's velocity (blocks per tick). Each tick the
 * game moves the player by it (with collisions), then applies gravity and drag to it.
 * We overwrite it right after the tick, so the next tick moves us exactly by our value:
 * no gravity, no drag. On a remote server the client reports the resulting position.
 */

void Fly::onTick(Actor& player) {
    const auto refs = player.refs();
    if (!refs) return;
    Vec3& velocity = refs->state.posDelta;

    float x = 0, z = 0;
    MoveInput::direction(refs->rotation.yaw, x, z);
    velocity.x = x * m_speed;
    velocity.z = z * m_speed;

    if (MoveInput::up()) velocity.y = m_verticalSpeed;
    else if (MoveInput::down()) velocity.y = -m_verticalSpeed;
    else velocity.y = m_antiKick ? -0.04f : 0.0f;
}
