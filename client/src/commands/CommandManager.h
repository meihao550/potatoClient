#pragma once
#include "Command.h"
#include <memory>
#include <string>
#include <vector>

namespace CommandManager {
    void init();
    // "up", ".up", "UP" ... (a leading '.' and case are ignored)
    void execute(const std::string& line);
    // Shows a message under the command bar and writes it to the log. Safe from any thread.
    void print(const std::string& message);
    // "使い方: <usage>" for a command whose arguments were wrong
    void printUsage(const Command& command);
    // Latest message and how many milliseconds ago it was printed (empty if none)
    std::string lastMessage(unsigned long long* ageMs);
}
