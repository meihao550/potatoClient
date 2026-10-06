#pragma once
#include <string>
#include <vector>

// A command typed into the command bar (Home key), e.g. "up".
class Command {
public:
    Command(std::string name, std::string description)
        : m_name(std::move(name)), m_description(std::move(description)) {}
    virtual ~Command() = default;

    // args[0] is the command name itself. Runs on the render thread:
    // use PlayerTick::run() to touch the game. Report with CommandManager::print().
    virtual void execute(const std::vector<std::string>& args) = 0;

    const std::string& name() const { return m_name; }          // lower case
    const std::string& description() const { return m_description; }

private:
    std::string m_name;
    std::string m_description;
};
