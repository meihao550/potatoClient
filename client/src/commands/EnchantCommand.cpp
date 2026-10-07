#include "EnchantCommand.h"
#include "Args.h"
#include "CommandManager.h"
#include "Require.h"
#include "core/Util.h"
#include "sdk/CommandSender.h"
#include "sdk/Enchant.h"
#include "sdk/PlayerTick.h"
#include <cstdio>
#include <utility>

/*
 * ENCHANT
 * -------
 * Same pattern as dupe: the item in hand is changed on the server (the real
 * inventory, saved with the world) and then the same change is made to the
 * client's copy so the hotbar / tooltip shows it right away.
 * The actual work is EnchantUtils::applyEnchant (see sdk/Enchant.cpp).
 *
 * On a remote server the real inventory is out of reach, so we ask the server instead:
 * "/enchant @s <name> <level>" for each enchant, sent like chat (sdk/CommandSender.cpp).
 * That needs operator permission. We first try each enchant on the client's copy of the
 * item with the same game rules, and only send the ones that fit - so "enchant all" doesn't
 * flood the chat with errors. The server then sends us the real item.
 */

namespace {
    using Wanted = std::vector<std::pair<uint8_t, int>>;   // (enchant id, level)
    constexpr const char* kHoldSomething = "エンチャントしたい武器・防具を手に持ってください";

    const Enchant::Info* byId(uint8_t id) {
        for (const auto& e : Enchant::all())
            if (e.id == id) return &e;
        return nullptr;
    }

    void printList() {
        std::string text = "エンチャント一覧 (名前 / 最大レベル):\n";
        int column = 0;
        for (const auto& e : Enchant::all()) {
            char line[96];
            snprintf(line, sizeof(line), "%s (%s) 最大%d", e.name, e.japanese, e.maxLevel);
            text += line;
            text += ++column % 3 ? "    " : "\n";
        }
        CommandManager::print(text);
    }

    // Server thread: enchant our ServerPlayer's held item, then mirror it on the client
    void enchantHeld(Actor& server, const Wanted& wanted) {
        ItemStack* stack = Require::heldItem(server, kHoldSomething);
        if (!stack) return;

        Wanted applied;
        for (const auto& [id, level] : wanted)
            if (Enchant::apply(*stack, id, level)) applied.push_back({ id, level });
        if (applied.empty()) {
            CommandManager::print("このアイテムには付けられません (種類が合わない・他のエンチャントと両立しない)");
            return;
        }

        PlayerTick::run(PlayerTick::Side::Client, [applied](Actor& local) {
            ItemStack* copy = local.getCarriedItem();
            if (copy && copy->item())
                for (const auto& [id, level] : applied) Enchant::apply(*copy, id, level);
            return true;
        });

        std::string text = std::string(stack->name()) + " に付けました:";
        for (const auto& [id, level] : applied) {
            char part[64];
            snprintf(part, sizeof(part), " %s %d", byId(id)->japanese, level);
            text += part;
        }
        CommandManager::print(text);
    }

    // Client thread, remote server: let the server run the vanilla /enchant for us
    void enchantViaServer(Actor& local, const Wanted& wanted) {
        ItemStack* stack = Require::heldItem(local, kHoldSomething);
        if (!stack) return;

        std::string text = std::string(stack->name()) + " に送信:";
        int sent = 0;
        for (const auto& [id, wantedLevel] : wanted) {
            const Enchant::Info* info = byId(id);
            const int level = wantedLevel > info->maxLevel ? info->maxLevel : wantedLevel;   // /enchant refuses more
            if (!Enchant::apply(*stack, id, level)) continue;   // doesn't fit this item
            if (!CommandSender::send(local, "/enchant @s " + std::string(info->name) + " " + std::to_string(level))) {
                CommandManager::print("コマンドを送れませんでした (client.log を確認)");
                return;
            }
            char part[64];
            snprintf(part, sizeof(part), " %s %d", info->japanese, level);
            text += part;
            ++sent;
        }
        if (!sent) {
            CommandManager::print("このアイテムには付けられません (種類が合わない・他のエンチャントと両立しない)");
            return;
        }
        CommandManager::print(text + "  (結果はチャットに出ます。OP 権限が必要)");
    }
}

void EnchantCommand::execute(const std::vector<std::string>& args) {
    using PlayerTick::Side;
    if (args.size() < 2 || Util::toLower(args[1]) == "list") {
        printList();
        return;
    }
    if (!Require::inWorld()) return;

    Wanted wanted;
    if (Util::toLower(args[1]) == "all") {
        for (uint8_t id : Enchant::bestSet()) wanted.push_back({ id, byId(id)->maxLevel });
    } else {
        const Enchant::Info* info = Enchant::find(args[1]);
        if (!info) {
            CommandManager::print("不明なエンチャント: " + args[1] + "  (enchant list で一覧)");
            return;
        }
        int level = info->maxLevel;
        if (args.size() > 2) {
            const auto parsed = Args::parseInt(args[2]);
            if (!parsed) { CommandManager::printUsage(*this); return; }
            level = *parsed;
        }
        if (level < 1) level = 1;
        if (level > 255) level = 255;
        wanted.push_back({ info->id, level });
    }

    if (PlayerTick::ticking(Side::Server)) {   // the world runs on this PC
        PlayerTick::runOnOwnServerPlayer([wanted](Actor& server) { enchantHeld(server, wanted); });
        return;
    }
    if (!CommandSender::available()) {
        CommandManager::print("サーバーへのコマンド送信が使えません (シグネチャ未検出: client.log を確認)");
        return;
    }
    if (wanted.size() == 1 && wanted[0].second > byId(wanted[0].first)->maxLevel)
        CommandManager::print("サーバーの /enchant は最大レベルまでなので、最大レベルで送ります");
    PlayerTick::run(Side::Client, [wanted](Actor& local) { enchantViaServer(local, wanted); return true; });
}
