#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <utility>

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
    // Every frame while enabled, on the render thread (draw with ImGui)
    virtual void onRender() {}

    // Called from the window thread (key binds), the render thread (menu) and the unload
    // thread, so switching is serialized: onEnable / onDisable never run twice at once.
    void setEnabled(bool on) {
        std::lock_guard lock(m_switchMutex);
        switchTo(on);
    }
    void toggle() {
        std::lock_guard lock(m_switchMutex);
        switchTo(!m_enabled);
    }

    bool isEnabled() const { return m_enabled; }
    const std::string& name() const { return m_name; }
    const std::string& description() const { return m_description; }
    int key() const { return m_key; }
    void setKey(int vk) { m_key = vk; }

private:
    void switchTo(bool on) {
        if (on == m_enabled || (on && !isAvailable())) return;
        if (on) m_enabled = onEnable();
        else { m_enabled = false; onDisable(); }
    }

    std::string m_name;
    std::string m_description;
    std::atomic<int> m_key = 0;          // virtual-key code, 0 = unbound
    std::atomic<bool> m_enabled = false;   // read every tick / frame without the lock
    std::mutex m_switchMutex;
};
