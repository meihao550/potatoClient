#pragma once

namespace Renderer {
    bool init();       // finds DXGI/D3D12 vtables and hooks Present / ResizeBuffers / ExecuteCommandLists
    void shutdown();   // releases ImGui + GPU resources (call after hooks are removed)
}
