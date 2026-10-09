#pragma once
#include <nlohmann/json_fwd.hpp>
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

/*
 * Module settings
 * ---------------
 * A setting is a value plus what the menu needs to show it: a stable English id (used as the
 * key in the config file - never change it once released), a label for the menu, and limits.
 * Modules keep them as members and register them with Module::addSettings, so the menu draws
 * them and the config saves them without per-module code.
 *
 * Values are atomic: the menu writes them on the render thread while onTick reads them on
 * the game thread.
 */

class Setting {
public:
    Setting(std::string id, std::string label, std::string hint)
        : m_id(std::move(id)), m_label(std::move(label)), m_hint(std::move(hint)) {}
    virtual ~Setting() = default;
    Setting(const Setting&) = delete;
    Setting& operator=(const Setting&) = delete;

    const std::string& id() const { return m_id; }
    const std::string& label() const { return m_label; }

    // Draws the widget (+ the hint under it). Render thread. true = the value changed.
    bool render();
    virtual void reset() = 0;   // back to the default value
    // Config file (modules/Config.cpp). load ignores a value of the wrong type.
    virtual void save(nlohmann::json& out) const = 0;
    virtual void load(const nlohmann::json& in) = 0;

protected:
    virtual bool renderWidget() = 0;

private:
    std::string m_id;
    std::string m_label;
    std::string m_hint;
};

class BoolSetting : public Setting {
public:
    BoolSetting(std::string id, std::string label, bool value, std::string hint = "")
        : Setting(std::move(id), std::move(label), std::move(hint)), m_default(value), m_value(value) {}

    bool get() const { return m_value; }
    operator bool() const { return m_value; }
    void set(bool value) { m_value = value; }
    void reset() override { m_value = m_default; }
    void save(nlohmann::json& out) const override;
    void load(const nlohmann::json& in) override;

protected:
    bool renderWidget() override;

private:
    const bool m_default;
    std::atomic<bool> m_value;
};

class FloatSetting : public Setting {
public:
    // format: printf-style for the slider, e.g. "%.2f ブロック/tick"
    FloatSetting(std::string id, std::string label, float value, float min, float max,
                 const char* format = "%.2f", std::string hint = "")
        : Setting(std::move(id), std::move(label), std::move(hint)),
          m_default(value), m_min(min), m_max(max), m_format(format), m_value(value) {}

    float get() const { return m_value; }
    operator float() const { return m_value; }
    void set(float value) { m_value = value < m_min ? m_min : value > m_max ? m_max : value; }
    void reset() override { m_value = m_default; }
    void save(nlohmann::json& out) const override;
    void load(const nlohmann::json& in) override;

protected:
    bool renderWidget() override;

private:
    const float m_default, m_min, m_max;
    const char* m_format;
    std::atomic<float> m_value;
};

// RGBA, each channel 0..1. Stored packed so it can be one atomic.
class ColorSetting : public Setting {
public:
    ColorSetting(std::string id, std::string label, float r, float g, float b, float a, std::string hint = "")
        : Setting(std::move(id), std::move(label), std::move(hint)),
          m_default(pack(r, g, b, a)), m_value(m_default) {}

    uint32_t packed() const { return m_value; }   // 0xAABBGGRR, the same layout as ImU32
    void setPacked(uint32_t rgba) { m_value = rgba; }
    void reset() override { m_value = m_default; }
    void save(nlohmann::json& out) const override;
    void load(const nlohmann::json& in) override;

protected:
    bool renderWidget() override;

private:
    static uint32_t pack(float r, float g, float b, float a);

    const uint32_t m_default;
    std::atomic<uint32_t> m_value;
};
