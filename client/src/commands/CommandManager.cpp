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

    class HelpCommand : public Command {
    public:
        HelpCommand() : Command("help", "コマンド一覧を表示") {}
        void execute(const std::vector<std::string>&) override {
            std::string text;
            for (const auto& c : g_commands) text += (text.empty() ? "" : " / ") + c->name() + ": " + c->description();
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

    const std::string name = Util::toLower(args[0]);
    for (const auto& c : g_commands) {
        if (c->name() == name) {
            LOG("command: %s", line.c_str());
            c->execute(args);
            return;
        }
    }
    print("不明なコマンド: " + args[0] + "  (help で一覧)");
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
