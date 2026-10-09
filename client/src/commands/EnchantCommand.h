#pragma once
#include "Command.h"

// "enchant <name> [level] [friend]" / "enchant all [friend]" / "enchant list"
class EnchantCommand : public Command {
public:
    EnchantCommand() : Command("enchant", "手に持っている武器・防具にエンチャント (末尾に friend で近くのフレンドの手持ちに)",
                               "enchant <名前> [レベル] [friend] / enchant all [friend] / enchant list") {}
    void execute(const std::vector<std::string>& args) override;
};
