#pragma once
#include "Command.h"

class Actor;
struct Vec3;

// After a client-side move (remote server), checks a second later whether the server
// pulled us back and says so. Does nothing for server = true (single-player /tp).
void reportIfPulledBack(Actor& player, bool server, const Vec3& feet);

// "vclip 10" / "vclip -5": move straight up / down through blocks
class VClipCommand : public Command {
public:
    VClipCommand() : Command("vclip", "上下に n ブロック移動 (vclip 10 / vclip -5)") {}
    void execute(const std::vector<std::string>& args) override;
};

// "hclip 5": move n blocks in the direction you are looking (horizontal only)
class HClipCommand : public Command {
public:
    HClipCommand() : Command("hclip", "向いている方向に n ブロック移動 (hclip 5)") {}
    void execute(const std::vector<std::string>& args) override;
};

// "tp 100 64 -20" / "tp ~ ~10 ~": move the feet to x y z ('~' = relative, like /tp)
class TpCommand : public Command {
public:
    TpCommand() : Command("tp", "座標へ移動 (tp x y z / ~ で相対)") {}
    void execute(const std::vector<std::string>& args) override;
};
