#include "MoveCommands.h"
#include "CommandManager.h"
#include "core/Util.h"
#include "sdk/Actor.h"
#include "sdk/PlayerTick.h"
#include <Windows.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>

/*
 * vclip / hclip / tp
 * ------------------
 * All three just compute a new feet position and call Actor::moveFeetTo on the side
 * that owns our position (PlayerTick::runOnSelf, same as up):
 *   single-player -> ServerPlayer::teleportTo, remote server -> write the LocalPlayer's position.
 * Nothing checks for blocks: you can end up inside a wall (that is the point of "clip").
 * On a remote server with server-side movement correction (BDS: correct-player-movement=true)
 * the server simulates our movement itself and sends us back - reportIfPulledBack tells the user.
 */

namespace {
    std::optional<float> parseNumber(const std::string& text) {
        char* end = nullptr;
        const float value = std::strtof(text.c_str(), &end);
        if (text.empty() || *end || !std::isfinite(value)) return std::nullopt;
        return value;
    }

    // "12.5" -> 12.5, "~" -> current, "~3" -> current + 3
    std::optional<float> parseCoordinate(const std::string& text, float current) {
        if (text.empty() || text[0] != '~') return parseNumber(text);
        if (text.size() == 1) return current;
        const auto offset = parseNumber(text.substr(1));
        return offset ? std::optional<float>(current + *offset) : std::nullopt;
    }

    bool inWorld() {
        if (PlayerTick::ticking(PlayerTick::Side::Client)) return true;
        CommandManager::print("ワールドに入ってから使ってください");
        return false;
    }

    void report(const Vec3& to) {
        char text[96];
        snprintf(text, sizeof(text), "%.1f %.1f %.1f へ移動しました", to.x, to.y, to.z);
        CommandManager::print(text);
    }

    // The player's components, or nothing (and a message) when they can't be read
    std::optional<ActorRefs> playerRefs(Actor& player) {
        auto refs = player.refs();
        if (!refs) CommandManager::print("プレイヤーの情報が取れません (Offsets.h を確認)");
        return refs;
    }
}

void reportIfPulledBack(Actor& player, bool server, const Vec3& feet) {
    if (server) return;
    const ULONGLONG checkAt = GetTickCount64() + 1000;
    PlayerTick::run(PlayerTick::Side::Client, [checkAt, feet](Actor& p) {
        if (GetTickCount64() < checkAt) return false;   // not yet: ask again next tick
        const auto refs = p.refs();
        if (!refs) return true;
        const Vec3 now = refs->feet();
        const float dx = now.x - feet.x, dy = now.y - feet.y, dz = now.z - feet.z;
        // falling after a vclip is fine; being moved sideways or back up/down a lot is not
        if (dx * dx + dz * dz > 1.0f || dy > 1.0f || dy < -30.0f)
            CommandManager::print("サーバーに位置を戻されました (このサーバーは移動を検証しています)");
        return true;
    });
}

void VClipCommand::execute(const std::vector<std::string>& args) {
    const auto blocks = args.size() >= 2 ? parseNumber(args[1]) : std::nullopt;
    if (!blocks) { CommandManager::print("使い方: vclip <ブロック数>  (例: vclip 10 / vclip -5)"); return; }
    if (!inWorld()) return;

    PlayerTick::runOnSelf([dy = *blocks](Actor& player, bool server) {
        const auto refs = playerRefs(player);
        if (!refs) return;
        Vec3 to = refs->feet();
        to.y += dy;
        player.moveFeetTo(to, server);
        report(to);
        reportIfPulledBack(player, server, to);
    });
}

void HClipCommand::execute(const std::vector<std::string>& args) {
    const auto blocks = args.size() >= 2 ? parseNumber(args[1]) : std::nullopt;
    if (!blocks) { CommandManager::print("使い方: hclip <ブロック数>  (例: hclip 5)"); return; }
    if (!inWorld()) return;

    PlayerTick::runOnSelf([distance = *blocks](Actor& player, bool server) {
        const auto refs = playerRefs(player);
        if (!refs) return;
        const float yaw = refs->rotation.yaw * Util::kDegToRad;
        Vec3 to = refs->feet();
        to.x += -std::sin(yaw) * distance;   // yaw 0 = +Z, yaw -90 = +X
        to.z += std::cos(yaw) * distance;
        player.moveFeetTo(to, server);
        report(to);
        reportIfPulledBack(player, server, to);
    });
}

void TpCommand::execute(const std::vector<std::string>& args) {
    if (args.size() != 4) { CommandManager::print("使い方: tp <x> <y> <z>  (~ で相対。例: tp ~ ~20 ~)"); return; }
    if (!inWorld()) return;

    PlayerTick::runOnSelf([args](Actor& player, bool server) {
        const auto refs = playerRefs(player);
        if (!refs) return;
        const Vec3 from = refs->feet();
        const auto x = parseCoordinate(args[1], from.x);
        const auto y = parseCoordinate(args[2], from.y);
        const auto z = parseCoordinate(args[3], from.z);
        if (!x || !y || !z) { CommandManager::print("座標が読めません: 数字か ~ を使ってください"); return; }
        const Vec3 to{ *x, *y, *z };
        player.moveFeetTo(to, server);
        report(to);
        reportIfPulledBack(player, server, to);
    });
}
