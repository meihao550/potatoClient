#include "Setting.h"
#include <imgui.h>
#include <nlohmann/json.hpp>

bool Setting::render() {
    const bool changed = renderWidget();
    if (!m_hint.empty()) ImGui::TextDisabled("%s", m_hint.c_str());
    return changed;
}

bool BoolSetting::renderWidget() {
    bool value = m_value;
    if (!ImGui::Checkbox(label().c_str(), &value)) return false;
    m_value = value;
    return true;
}

bool FloatSetting::renderWidget() {
    float value = m_value;
    if (!ImGui::SliderFloat(label().c_str(), &value, m_min, m_max, m_format)) return false;
    set(value);
    return true;
}

uint32_t ColorSetting::pack(float r, float g, float b, float a) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a));
}

bool ColorSetting::renderWidget() {
    const ImVec4 c = ImGui::ColorConvertU32ToFloat4(m_value);
    float rgba[4] = { c.x, c.y, c.z, c.w };
    if (!ImGui::ColorEdit4(label().c_str(), rgba)) return false;
    m_value = pack(rgba[0], rgba[1], rgba[2], rgba[3]);
    return true;
}

// ---- config file ----

void BoolSetting::save(nlohmann::json& out) const { out = get(); }
void BoolSetting::load(const nlohmann::json& in) {
    if (in.is_boolean()) set(in.get<bool>());
}

void FloatSetting::save(nlohmann::json& out) const { out = get(); }
void FloatSetting::load(const nlohmann::json& in) {
    if (in.is_number()) set(in.get<float>());   // set() clamps to the slider's range
}

// [r, g, b, a], each 0..1
void ColorSetting::save(nlohmann::json& out) const {
    const ImVec4 c = ImGui::ColorConvertU32ToFloat4(m_value);
    out = nlohmann::json::array({ c.x, c.y, c.z, c.w });
}
void ColorSetting::load(const nlohmann::json& in) {
    if (!in.is_array() || in.size() != 4) return;
    float c[4];
    for (int i = 0; i < 4; i++) {
        if (!in[i].is_number()) return;
        const float v = in[i].get<float>();
        c[i] = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
    }
    m_value = pack(c[0], c[1], c[2], c[3]);
}
