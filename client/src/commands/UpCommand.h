#pragma once
#include "Command.h"

// "up": teleport onto the highest block above the player
class UpCommand : public Command {
public:
    UpCommand() : Command("up", "真上の一番高いブロックの上へテレポート", "up") {}
    void execute(const std::vector<std::string>& args) override;
};
