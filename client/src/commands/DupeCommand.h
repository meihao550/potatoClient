#pragma once
#include "Command.h"

// "dupe [count]": fill the stack in your hand (default: up to the max stack size)
class DupeCommand : public Command {
public:
    DupeCommand() : Command("dupe", "手に持っているアイテムを増やす (dupe [個数])") {}
    void execute(const std::vector<std::string>& args) override;
};
