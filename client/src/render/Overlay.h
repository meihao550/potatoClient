#pragma once
#include <Windows.h>

// The API-independent half of drawing the overlay. Each backend (Dx11Backend, Dx12Backend)
// only adds the D3D-specific setup and the draw call around these. Render thread only.
namespace Overlay {
    // ImGui context + fonts + the Win32 platform backend for the game window
    void init(HWND window);
    // Builds this frame's UI (menu, command bar, module overlays). Call after the
    // D3D backend's NewFrame and before rendering ImGui::GetDrawData().
    void buildFrame();
    // Win32 platform backend + context. Call after the D3D backend's own shutdown.
    void shutdown();
}
