#include "Backends.h"
#include "core/Logger.h"
#include "gui/Menu.h"
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <vector>

/*
 * D3D12 is explicit: we own a command allocator per back buffer, a command list,
 * an RTV heap (render target views of the game's back buffers) and an SRV heap
 * (for ImGui's font texture). A fence tells us when the GPU finished a frame so
 * we don't reset an allocator that is still in use.
 */

namespace {
    struct Frame {
        ID3D12CommandAllocator* allocator = nullptr;
        ID3D12Resource* backBuffer = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
        UINT64 fenceValue = 0;
    };

    ID3D12Device* g_device = nullptr;
    ID3D12DescriptorHeap* g_rtvHeap = nullptr;
    ID3D12DescriptorHeap* g_srvHeap = nullptr;
    ID3D12GraphicsCommandList* g_cmdList = nullptr;
    ID3D12Fence* g_fence = nullptr;
    HANDLE g_fenceEvent = nullptr;
    UINT64 g_fenceCounter = 0;
    std::vector<Frame> g_frames;
    bool g_buffersReady = false;

    void waitFor(UINT64 value) {
        if (g_fence->GetCompletedValue() < value) {
            g_fence->SetEventOnCompletion(value, g_fenceEvent);
            WaitForSingleObject(g_fenceEvent, 1000);
        }
    }

    void createBuffers(IDXGISwapChain3* swapChain) {
        DXGI_SWAP_CHAIN_DESC desc{};
        swapChain->GetDesc(&desc);
        if (desc.BufferCount != g_frames.size()) {
            // Our RTV heap / allocators are sized for the original count; skip drawing rather than crash
            LOG("dx12: buffer count changed (%u -> %u), overlay paused", (UINT)g_frames.size(), desc.BufferCount);
            return;
        }
        const UINT rtvSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < g_frames.size(); ++i) {
            swapChain->GetBuffer(i, IID_PPV_ARGS(&g_frames[i].backBuffer));
            g_frames[i].rtv = handle;
            g_device->CreateRenderTargetView(g_frames[i].backBuffer, nullptr, handle);
            handle.ptr += rtvSize;
        }
        g_buffersReady = true;
    }
}

bool Dx12Backend::init(IDXGISwapChain3* swapChain, ID3D12Device* device) {
    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    g_device = device;
    g_device->AddRef();
    g_frames.resize(desc.BufferCount);

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, desc.BufferCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    if (FAILED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_rtvHeap))) ||
        FAILED(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvHeap)))) {
        LOG("dx12: descriptor heap creation failed");
        return false;
    }
    for (auto& f : g_frames)
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&f.allocator));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frames[0].allocator, nullptr, IID_PPV_ARGS(&g_cmdList));
    g_cmdList->Close();
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
    g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    createBuffers(swapChain);

    ImGui::CreateContext();
    Menu::loadFonts();
    ImGui_ImplWin32_Init(desc.OutputWindow);

    ImGui_ImplDX12_InitInfo info;
    info.Device = device;
    info.CommandQueue = commandQueue;
    info.NumFramesInFlight = static_cast<int>(desc.BufferCount);
    info.RTVFormat = desc.BufferDesc.Format;
    info.SrvDescriptorHeap = g_srvHeap;
    info.LegacySingleSrvCpuDescriptor = g_srvHeap->GetCPUDescriptorHandleForHeapStart();
    info.LegacySingleSrvGpuDescriptor = g_srvHeap->GetGPUDescriptorHandleForHeapStart();
    const bool ok = ImGui_ImplDX12_Init(&info);
    LOG("dx12: %u back buffers, format %d, imgui init %s", desc.BufferCount, desc.BufferDesc.Format, ok ? "ok" : "failed");
    return ok;
}

void Dx12Backend::render(IDXGISwapChain3* swapChain) {
    if (!g_buffersReady) createBuffers(swapChain);
    if (!g_buffersReady) return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().MouseDrawCursor = Menu::open;
    if (Menu::open) ClipCursor(nullptr);   // free the mouse while the menu is open
    Menu::render();
    ImGui::Render();

    Frame& frame = g_frames[swapChain->GetCurrentBackBufferIndex()];
    waitFor(frame.fenceValue);
    frame.allocator->Reset();
    g_cmdList->Reset(frame.allocator, nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frame.backBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    g_cmdList->ResourceBarrier(1, &barrier);

    g_cmdList->OMSetRenderTargets(1, &frame.rtv, FALSE, nullptr);
    g_cmdList->SetDescriptorHeaps(1, &g_srvHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_cmdList);

    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    g_cmdList->ResourceBarrier(1, &barrier);
    g_cmdList->Close();

    ID3D12CommandList* lists[] = { g_cmdList };
    commandQueue->ExecuteCommandLists(1, lists);
    frame.fenceValue = ++g_fenceCounter;
    commandQueue->Signal(g_fence, frame.fenceValue);
}

void Dx12Backend::releaseBuffers() {
    waitFor(g_fenceCounter);
    for (auto& f : g_frames)
        if (f.backBuffer) { f.backBuffer->Release(); f.backBuffer = nullptr; }
    g_buffersReady = false;
}

void Dx12Backend::shutdown() {
    if (!g_device) return;
    releaseBuffers();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    for (auto& f : g_frames)
        if (f.allocator) f.allocator->Release();
    g_frames.clear();
    if (g_cmdList) g_cmdList->Release();
    if (g_fence) g_fence->Release();
    if (g_fenceEvent) CloseHandle(g_fenceEvent);
    if (g_rtvHeap) g_rtvHeap->Release();
    if (g_srvHeap) g_srvHeap->Release();
    g_device->Release();
    g_device = nullptr;
}
