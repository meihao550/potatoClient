#include "EnchantCommand.h"
#include "Args.h"
#include "CommandManager.h"
#include "Require.h"
#include "core/Util.h"
#include "sdk/CommandSender.h"
#include "sdk/Enchant.h"
#include "sdk/PlayerItems.h"
#include "sdk/PlayerTick.h"
#include <cstdio>
#include <memory>
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
 * /enchant stops at the vanilla max level, so for more a guest asks the host: "enchant ... friend"
 * makes the host do the single-player steps on the guest's ServerPlayer instead.
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

    // Server thread: enchant a ServerPlayer's held item. Returns what was applied (empty = nothing).
    Wanted enchantHeld(Actor& server, const Wanted& wanted, const char* messageIfEmpty = kHoldSomething) {
        Wanted applied;
        ItemStack* stack = Require::heldItem(server, messageIfEmpty);
        if (!stack) return applied;

        for (const auto& [id, level] : wanted)
            if (Enchant::apply(*stack, id, level)) applied.push_back({ id, level });
        if (applied.empty()) {
            CommandManager::print("このアイテムには付けられません (種類が合わない・他のエンチャントと両立しない)");
            return applied;
        }

        std::string text = std::string(stack->name()) + " に付けました:";
        for (const auto& [id, level] : applied) {
            char part[64];
            snprintf(part, sizeof(part), " %s %d", byId(id)->japanese, level);
            text += part;
        }
        CommandManager::print(text);
        return applied;
    }

    // Client thread: the same enchants on the client's copy of the held item, so the hotbar shows them
    void applyToCopy(Actor& local, const Wanted& applied) {
        ItemStack* copy = local.getCarriedItem();
        if (copy && copy->item())
            for (const auto& [id, level] : applied) Enchant::apply(*copy, id, level);
    }

    // Server thread: applyEnchant changed the guest's stack in place, and the server only tells a
    // client about slots changed through the container. Copy it to an empty slot and back, so both
    // slot changes go out to the guest's client. false = no empty slot to do it with.
    bool resendHeld(Actor& player) {
        Container* inventory = PlayerItems::inventory(player);
        if (!inventory) return false;
        const int slot = PlayerItems::selectedSlot(player);
        const int spare = PlayerItems::firstEmpty(*inventory);
        if (spare < 0) return false;
        inventory->setItem(spare, *inventory->getItem(slot));
        ItemStack* copy = inventory->getItem(spare);
        inventory->setItem(slot, *copy);
        inventory->removeItem(spare, copy->count());
        return true;
    }

    // Hosting: enchant the held item of the guest standing closest to us, any level up to 255
    void enchantFriend(const Wanted& wanted) {
        PlayerTick::runOnNearestGuest([wanted](Actor* guest) {
            if (!guest) {
                CommandManager::print("近く (16 ブロック以内) にフレンドがいません");
                return;
            }
            CommandManager::print("フレンドのアイテム:");
            if (enchantHeld(*guest, wanted, "フレンドがアイテムを手に持っていません").empty()) return;
            if (!resendHeld(*guest))
                CommandManager::print("インベントリに空きがないので、フレンドの画面にはまだ出ません (アイテムを一度捨てて拾うか、入り直すと出ます)");
        });
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
        // Only a note: we can't see the server's answer here, it comes back as a chat message
        CommandManager::print(text + "  → 送信しました。結果はチャットに出ます (失敗したらそこにエラーが出ます)");
    }
}

void EnchantCommand::execute(const std::vector<std::string>& arguments) {
    using PlayerTick::Side;
    std::vector<std::string> args = arguments;
    const bool forFriend = args.size() > 2 && Util::toLower(args.back()) == "friend";
    if (forFriend) args.pop_back();
    if (args.size() < 2 || Util::toLower(args[1]) == "list") {
        printList();
        return;
    }
    if (!Require::inWorld()) return;
    // Both paths need applyEnchant: directly in single-player, to test which enchants fit before sending otherwise
    if (!Enchant::available()) {
        CommandManager::print("エンチャントの関数が見つかりません (シグネチャ未検出: client.log を確認)");
        return;
    }

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

    if (forFriend) {
        if (!Require::ownWorld()) return;   // only the host can touch someone else's inventory
        enchantFriend(wanted);
        return;
    }
    if (PlayerTick::ticking(Side::Server)) {   // the world runs on this PC
        auto applied = std::make_shared<Wanted>();
        PlayerTick::runOnServerThenClient(
            [wanted, applied](Actor& server) { *applied = enchantHeld(server, wanted); return !applied->empty(); },
            [applied](Actor& local) { applyToCopy(local, *applied); });
        return;
    }
    if (!CommandSender::available()) {
        CommandManager::print("サーバーへのコマンド送信が使えません (シグネチャ未検出: client.log を確認)");
        return;
    }
    if (wanted.size() == 1 && wanted[0].second > byId(wanted[0].first)->maxLevel)
        CommandManager::print("サーバーの /enchant は最大レベルまでなので、最大レベルで送ります (それ以上はホストに「enchant <名前> <レベル> friend」を頼んでください)");
    PlayerTick::run(Side::Client, [wanted](Actor& local) { enchantViaServer(local, wanted); return true; });
}
