#pragma once
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <atomic>

// Each backend draws ImGui on top of the game's back buffer.
// All functions run on the game's render thread (inside Present).
namespace Dx12Backend {
    // Captured from ExecuteCommandLists (any game thread), used on the render thread
    inline std::atomic<ID3D12CommandQueue*> commandQueue = nullptr;
    bool init(IDXGISwapChain3* swapChain, ID3D12Device* device);
    void render(IDXGISwapChain3* swapChain);
    void releaseBuffers();   // before ResizeBuffers
    void shutdown();
}

namespace Dx11Backend {
    bool init(IDXGISwapChain* swapChain, ID3D11Device* device);
    void render(IDXGISwapChain* swapChain);
    void shutdown();
}
