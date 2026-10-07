#pragma once
#include <string>
#include <utility>
#include <vector>

// A command typed into the command bar (Home key), e.g. "up".
class Command {
public:
    // usage: how to type it, shown by "help <name>" and when the arguments are wrong
    Command(std::string name, std::string description, std::string usage)
        : m_name(std::move(name)), m_description(std::move(description)), m_usage(std::move(usage)) {}
    virtual ~Command() = default;

    // args[0] is the command name itself. Runs on the render thread:
    // use PlayerTick::run() to touch the game. Report with CommandManager::print().
    virtual void execute(const std::vector<std::string>& args) = 0;

    const std::string& name() const { return m_name; }          // lower case
    const std::string& description() const { return m_description; }
    const std::string& usage() const { return m_usage; }

private:
    std::string m_name;
    std::string m_description;
    std::string m_usage;
};
