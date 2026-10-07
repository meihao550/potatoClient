#include "Backends.h"
#include "core/InputFocus.h"
#include "core/Logger.h"
#include "gui/Menu.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

// Fallback for when the game runs on D3D11 (much simpler than D3D12).

namespace {
    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
}

bool Dx11Backend::init(IDXGISwapChain* swapChain, ID3D11Device* device) {
    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    g_device = device;
    g_device->AddRef();
    g_device->GetImmediateContext(&g_context);

    ImGui::CreateContext();
    Menu::loadFonts();
    ImGui_ImplWin32_Init(desc.OutputWindow);
    const bool ok = ImGui_ImplDX11_Init(g_device, g_context);
    LOG("dx11: imgui init %s", ok ? "ok" : "failed");
    return ok;
}

void Dx11Backend::render(IDXGISwapChain* swapChain) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().MouseDrawCursor = InputFocus::menuOpen;
    if (InputFocus::menuOpen) ClipCursor(nullptr);
    Menu::render();
    ImGui::Render();

    // Create a render target view of the current back buffer for this frame only,
    // so ResizeBuffers never fails because we hold a reference.
    ID3D11Texture2D* backBuffer = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
        g_device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
        backBuffer->Release();
    }
    if (!rtv) return;
    g_context->OMSetRenderTargets(1, &rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    rtv->Release();
}

void Dx11Backend::shutdown() {
    if (!g_device) return;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_context->Release();
    g_device->Release();
    g_device = nullptr;
}
