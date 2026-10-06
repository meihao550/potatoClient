#include "MoveInput.h"
#include "gui/Menu.h"
#include <Windows.h>
#include <cmath>

namespace {
    bool held(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
}

bool MoveInput::gameHasFocus() {
    if (Menu::capturesInput()) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

bool MoveInput::direction(float yawDegrees, float& x, float& z) {
    if (!gameHasFocus()) return false;
    const float forward = (held('W') ? 1.0f : 0.0f) - (held('S') ? 1.0f : 0.0f);
    const float left = (held('A') ? 1.0f : 0.0f) - (held('D') ? 1.0f : 0.0f);
    if (forward == 0 && left == 0) return false;

    // yaw 0 = facing +Z, yaw -90 = facing +X.  forward = (-sin, cos), left = (cos, sin)
    const float yaw = yawDegrees * 3.14159265f / 180.0f;
    const float s = std::sin(yaw), c = std::cos(yaw);
    x = forward * -s + left * c;
    z = forward * c + left * s;
    const float length = std::sqrt(x * x + z * z);
    x /= length;
    z /= length;
    return true;
}

bool MoveInput::up()   { return gameHasFocus() && held(VK_SPACE); }
bool MoveInput::down() { return gameHasFocus() && held(VK_SHIFT); }
