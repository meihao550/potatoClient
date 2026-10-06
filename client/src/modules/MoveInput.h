#pragma once

// Movement keys for Fly / Speed, read straight from the keyboard (GetAsyncKeyState).
// Uses the default WASD / Space / Shift, not the game's own key settings.
namespace MoveInput {
    // False while another window, our menu or the command bar has the keyboard
    bool gameHasFocus();
    // Unit vector on the ground (x, z) for the held WASD keys relative to yaw (degrees).
    // Returns false when no direction is held (or W+S / A+D cancel out).
    bool direction(float yawDegrees, float& x, float& z);
    bool up();     // Space
    bool down();   // Shift
}
