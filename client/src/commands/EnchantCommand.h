#pragma once
#include "Command.h"

// "enchant <name> [level]" / "enchant all" / "enchant list"
class EnchantCommand : public Command {
public:
    EnchantCommand() : Command("enchant", "手に持っている武器・防具にエンチャント (enchant list で一覧)") {}
    void execute(const std::vector<std::string>& args) override;
};
