#pragma once
#include <string>

class Actor;

class Module {
public:
    Module(std::string name, std::string description, int key)
        : m_name(std::move(name)), m_description(std::move(description)), m_key(key) {}
    virtual ~Module() = default;

    virtual bool onEnable() { return true; }   // return false to refuse enabling
    virtual void onDisable() {}
    virtual void renderSettings() {}       // extra ImGui widgets shown in the menu
    virtual bool isAvailable() const { return true; }  // false = signatures missing
    // Every client tick while enabled, on the game thread, with our LocalPlayer
    virtual void onTick(Actor& /*player*/) {}

    void setEnabled(bool on) {
        if (on == m_enabled || (on && !isAvailable())) return;
        if (on) m_enabled = onEnable();
        else { m_enabled = false; onDisable(); }
    }
    void toggle() { setEnabled(!m_enabled); }

    bool isEnabled() const { return m_enabled; }
    const std::string& name() const { return m_name; }
    const std::string& description() const { return m_description; }
    int key() const { return m_key; }
    void setKey(int vk) { m_key = vk; }

private:
    std::string m_name;
    std::string m_description;
    int m_key = 0;          // virtual-key code, 0 = unbound
    bool m_enabled = false;
};
