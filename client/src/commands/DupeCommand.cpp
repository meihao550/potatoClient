#include "DupeCommand.h"
#include "Args.h"
#include "CommandManager.h"
#include "Require.h"
#include "sdk/Actor.h"
#include "sdk/PlayerTick.h"
#include <cstdio>
#include <memory>

/*
 * DUPE
 * ----
 * An item stack in the inventory is just a struct with a count byte (ItemStack::mCount).
 * Actor::getCarriedItem() returns a reference to the selected hotbar slot, so we
 * write the new count straight into it - no new item objects are created.
 *
 * It has to happen on both sides with the same value:
 *   - server (ServerPlayer): the real inventory. This is what gets saved and used
 *     when you place/drop/craft.
 *   - client (LocalPlayer): what the hotbar shows. The server does not resend a
 *     slot it thinks nobody changed, so without this the number on screen stays old.
 *
 * Items that don't stack (tools, armor: max 1) can't be multiplied this way; that would
 * need a real copy of the ItemStack (it owns enchantment NBT etc.), i.e. calling the
 * game's own copy constructor.
 */

namespace {
    // Returns the new count, or 0 (with a message) if the held item can't be changed
    int fill(Actor& player, int wanted, bool report) {
        ItemStack* stack = player.getCarriedItem();
        if (!stack || !stack->item()) {
            if (report) CommandManager::print("アイテムを手に持ってから使ってください");
            return 0;
        }
        const int max = stack->maxStackSize();
        if (max < 1 || max > 64) {   // sanity check: wrong offsets would read garbage here
            if (report) CommandManager::print("アイテムの情報が読めません (Offsets.h を確認)");
            return 0;
        }
        if (max == 1) {
            if (report) CommandManager::print("スタックできないアイテム (道具・防具など) は増やせません");
            return 0;
        }
        const int count = wanted < 1 || wanted > max ? max : wanted;
        stack->count() = static_cast<uint8_t>(count);
        if (report) {
            char text[160];
            snprintf(text, sizeof(text), "%s を %d 個にしました", stack->name(), count);
            CommandManager::print(text);
        }
        return count;
    }
}

void DupeCommand::execute(const std::vector<std::string>& args) {
    if (!Require::inWorld() || !Require::ownWorld()) return;

    int wanted = 0;   // 0 = max stack size
    if (args.size() > 1) {
        const auto parsed = Args::parseInt(args[1]);
        if (!parsed || *parsed < 1) { CommandManager::printUsage(*this); return; }
        wanted = *parsed;   // more than the max stack size is capped by fill()
    }
    // Server first (the real inventory, reports the result), then the client's copy to match it
    auto count = std::make_shared<int>(0);
    PlayerTick::runOnServerThenClient(
        [wanted, count](Actor& server) { *count = fill(server, wanted, true); return *count > 0; },
        [count](Actor& local) { fill(local, *count, false); });
}
