#include "Setting.h"
#include <imgui.h>

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
