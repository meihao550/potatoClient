#include "CommandManager.h"
#include "DupeCommand.h"
#include "EnchantCommand.h"
#include "MoveCommands.h"
#include "UpCommand.h"
#include "core/Logger.h"
#include "core/Util.h"
#include <Windows.h>
#include <mutex>
#include <sstream>

namespace {
    std::vector<std::unique_ptr<Command>> g_commands;

    std::mutex g_messageMutex;
    std::string g_message;
    ULONGLONG g_messageTime = 0;

    Command* find(const std::string& name) {
        const std::string key = Util::toLower(name);
        for (const auto& c : g_commands)
            if (c->name() == key) return c.get();
        return nullptr;
    }

    // "help": one line per command. "help tp": how to use that command.
    class HelpCommand : public Command {
    public:
        HelpCommand() : Command("help", "コマンド一覧 / 使い方を表示", "help [コマンド名]") {}
        void execute(const std::vector<std::string>& args) override {
            if (args.size() >= 2) {
                const Command* c = find(args[1]);
                if (!c) { CommandManager::print("不明なコマンド: " + args[1]); return; }
                CommandManager::print(c->name() + ": " + c->description() + "\n使い方: " + c->usage());
                return;
            }
            std::string text = "コマンド一覧 (help <名前> で使い方):";
            for (const auto& c : g_commands) text += "\n  " + c->name() + " - " + c->description();
            CommandManager::print(text);
        }
    };
}

void CommandManager::init() {
    g_commands.push_back(std::make_unique<UpCommand>());
    g_commands.push_back(std::make_unique<VClipCommand>());
    g_commands.push_back(std::make_unique<HClipCommand>());
    g_commands.push_back(std::make_unique<TpCommand>());
    g_commands.push_back(std::make_unique<DupeCommand>());
    g_commands.push_back(std::make_unique<EnchantCommand>());
    g_commands.push_back(std::make_unique<HelpCommand>());
}

void CommandManager::execute(const std::string& line) {
    const size_t start = line.find_first_not_of(" .");
    if (start == std::string::npos) return;

    std::istringstream in(line.substr(start));
    std::vector<std::string> args;
    for (std::string word; in >> word;) args.push_back(word);
    if (args.empty()) return;

    Command* command = find(args[0]);
    if (!command) {
        print("不明なコマンド: " + args[0] + "  (help で一覧)");
        return;
    }
    LOG("command: %s", line.c_str());
    command->execute(args);
}

void CommandManager::printUsage(const Command& command) {
    print("使い方: " + command.usage());
}

void CommandManager::print(const std::string& message) {
    LOG("%s", message.c_str());
    std::lock_guard lock(g_messageMutex);
    g_message = message;
    g_messageTime = GetTickCount64();
}

std::string CommandManager::lastMessage(unsigned long long* ageMs) {
    std::lock_guard lock(g_messageMutex);
    if (ageMs) *ageMs = GetTickCount64() - g_messageTime;
    return g_message;
}
