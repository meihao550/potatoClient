#include "UpCommand.h"
#include "CommandManager.h"
#include "MoveCommands.h"
#include "sdk/Actor.h"
#include "sdk/PlayerTick.h"
#include <cmath>
#include <cstdio>

/*
 * UP
 * --
 * 1-2. PlayerTick::runOnSelf: in single-player, find our ServerPlayer on the built-in
 *      server (the one standing where our LocalPlayer is). On a remote server there is
 *      no ServerPlayer in this process, so we use the LocalPlayer itself.
 * 3. Ask that side's world for the top of that column:
 *    BlockSource::getAboveTopSolidBlock(x, z, includeWater, includeLeaves)
 *    scans down from the build limit and returns the Y just above the first
 *    solid block, i.e. where our feet should go.
 * 4. Actor::moveFeetTo:
 *    - single-player: ServerPlayer::teleportTo - the same thing /tp does. The server moves
 *      us and tells the client. (Moving only the LocalPlayer gets undone by the server.)
 *    - remote server: write the LocalPlayer's position. The client reports its position
 *      every tick, so a server without movement checks simply accepts it.
 *
 * In the Nether the bedrock ceiling is the top, so you end up on the Nether roof.
 */

namespace {
    using PlayerTick::Side;

    // Runs on the thread of whichever side owns our position (see PlayerTick::runOnSelf)
    void teleportUp(Actor& player, bool server) {
        StateVectorComponent* state = player.stateVector();
        AABBShapeComponent* shape = player.aabbShape();
        BlockSource* region = player.blockSource();
        if (!state || !shape || !region) {
            CommandManager::print("プレイヤーの情報が取れません (Offsets.h を確認)");
            return;
        }

        const float feetY = shape->aabb.min.y;
        const int x = static_cast<int>(std::floor(state->pos.x));
        const int z = static_cast<int>(std::floor(state->pos.z));
        const short top = region->getAboveTopSolidBlock(x, z, true, true);   // water and leaves count as ground

        if (top <= region->getMinHeight()) {
            CommandManager::print("この場所の上にはブロックがありません");
            return;
        }
        if (top <= feetY + 0.01f) {
            CommandManager::print("すでに一番上にいます");
            return;
        }

        const Vec3 to{ state->pos.x, static_cast<float>(top), state->pos.z };
        player.moveFeetTo(to, server);
        reportIfPulledBack(player, server, to);

        char text[96];
        snprintf(text, sizeof(text), "Y %.0f → %d へ移動しました", std::floor(feetY), top);
        CommandManager::print(text);
    }
}

void UpCommand::execute(const std::vector<std::string>&) {
    if (!PlayerTick::ticking(Side::Client)) {
        CommandManager::print("ワールドに入ってから使ってください");
        return;
    }

    PlayerTick::runOnSelf(teleportUp);
}
