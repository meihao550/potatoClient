#include "Renderer.h"
#include "Backends.h"
#include "core/Hooks.h"
#include "core/InFlight.h"
#include "core/Logger.h"
#include "gui/Input.h"
#include <Windows.h>
#include <mutex>

/*
 * How the overlay works
 * ---------------------
 * 1. Create a throw-away window + D3D12 device/queue/swapchain of our own.
 * 2. Read function pointers out of their COM vtables. DXGI/D3D12 objects of the
 *    same class share one vtable, so these are the same functions the game calls.
 *      IDXGISwapChain::Present        = vtable[8]
 *      IDXGISwapChain::ResizeBuffers  = vtable[13]
 *      ID3D12CommandQueue::ExecuteCommandLists = vtable[10]
 * 3. Detour them with MinHook. Every frame the game presents, we draw ImGui first.
 */

namespace {
    enum class Api { Unknown, Dx12, Dx11, Failed };
    Api g_api = Api::Unknown;
    std::mutex g_renderMutex;

    using Present_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    using ExecuteCommandLists_t = void(__stdcall*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
    Present_t oPresent = nullptr;
    ResizeBuffers_t oResizeBuffers = nullptr;
    ExecuteCommandLists_t oExecuteCommandLists = nullptr;

    void __stdcall hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT count, ID3D12CommandList* const* lists) {
        InFlight::Guard guard;   // see core/InFlight.h
        // Remember the game's DIRECT queue; ImGui must submit on the queue that owns the swapchain.
        if (!Dx12Backend::commandQueue && queue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            ID3D12CommandQueue* expected = nullptr;
            if (Dx12Backend::commandQueue.compare_exchange_strong(expected, queue)) {   // first thread wins
                queue->AddRef();
                LOG("captured D3D12 command queue %p", queue);
            }
        }
        oExecuteCommandLists(queue, count, lists);
    }

    void initApi(IDXGISwapChain* swapChain) {
        DXGI_SWAP_CHAIN_DESC desc{};
        swapChain->GetDesc(&desc);

        ID3D12Device* d12 = nullptr;
        ID3D11Device* d11 = nullptr;
        if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&d12)))) {
            if (!Dx12Backend::commandQueue) { d12->Release(); return; }   // wait for the queue
            IDXGISwapChain3* sc3 = nullptr;
            swapChain->QueryInterface(IID_PPV_ARGS(&sc3));
            const bool ok = sc3 && Dx12Backend::init(sc3, d12);
            if (sc3) sc3->Release();
            d12->Release();
            g_api = ok ? Api::Dx12 : Api::Failed;
        } else if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&d11)))) {
            const bool ok = Dx11Backend::init(swapChain, d11);
            d11->Release();
            g_api = ok ? Api::Dx11 : Api::Failed;
        } else {
            g_api = Api::Failed;
        }
        LOG("renderer api = %s", g_api == Api::Dx12 ? "D3D12" : g_api == Api::Dx11 ? "D3D11" : "FAILED");
        if (g_api != Api::Failed) Input::install(desc.OutputWindow);
    }

    HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT sync, UINT flags) {
        InFlight::Guard guard;   // see core/InFlight.h
        {
            std::lock_guard lock(g_renderMutex);
            if (g_api == Api::Unknown) initApi(swapChain);
            if (g_api == Api::Dx12) {
                IDXGISwapChain3* sc3 = nullptr;
                if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&sc3)))) {
                    Dx12Backend::render(sc3);
                    sc3->Release();
                }
            } else if (g_api == Api::Dx11) {
                Dx11Backend::render(swapChain);
            }
        }
        return oPresent(swapChain, sync, flags);
    }

    HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
        InFlight::Guard guard;   // see core/InFlight.h
        {
            // The swapchain can't resize while we hold references to its back buffers
            std::lock_guard lock(g_renderMutex);
            if (g_api == Api::Dx12) Dx12Backend::releaseBuffers();
        }
        return oResizeBuffers(sc, count, w, h, fmt, flags);
    }

    bool getVtables(void** present, void** resize, void** execute) {
        WNDCLASSEXW wc{ sizeof(wc), CS_HREDRAW | CS_VREDRAW, DefWindowProcW, 0, 0, GetModuleHandleW(nullptr),
                        nullptr, nullptr, nullptr, nullptr, L"PotatoClientDummy", nullptr };
        RegisterClassExW(&wc);
        HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

        bool ok = false;
        IDXGIFactory4* factory = nullptr;
        ID3D12Device* device = nullptr;
        ID3D12CommandQueue* queue = nullptr;
        IDXGISwapChain1* swapChain = nullptr;
        D3D12_COMMAND_QUEUE_DESC qd{ D3D12_COMMAND_LIST_TYPE_DIRECT };
        DXGI_SWAP_CHAIN_DESC1 sd{};
        sd.Width = 100; sd.Height = 100;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 2;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) &&
            SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))) &&
            SUCCEEDED(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue))) &&
            SUCCEEDED(factory->CreateSwapChainForHwnd(queue, hwnd, &sd, nullptr, nullptr, &swapChain))) {
            void** scVtable = *reinterpret_cast<void***>(swapChain);
            void** qVtable = *reinterpret_cast<void***>(queue);
            *present = scVtable[8];
            *resize = scVtable[13];
            *execute = qVtable[10];
            ok = true;
        }
        if (swapChain) swapChain->Release();
        if (queue) queue->Release();
        if (device) device->Release();
        if (factory) factory->Release();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return ok;
    }
}

bool Renderer::init() {
    void *present = nullptr, *resize = nullptr, *execute = nullptr;
    if (!getVtables(&present, &resize, &execute)) {
        LOG("failed to create dummy D3D12 swapchain");
        return false;
    }
    bool ok = Hooks::create("ID3D12CommandQueue::Execute", execute, &hkExecuteCommandLists, oExecuteCommandLists);
    ok &= Hooks::create("IDXGISwapChain::ResizeBuffers", resize, &hkResizeBuffers, oResizeBuffers);
    ok &= Hooks::create("IDXGISwapChain::Present", present, &hkPresent, oPresent);
    return ok;
}

void Renderer::shutdown() {
    std::lock_guard lock(g_renderMutex);
    // Both, whatever g_api says: a backend whose init failed (Api::Failed) still holds what it
    // created before failing. Each one only releases what it actually set up.
    Dx12Backend::shutdown();
    Dx11Backend::shutdown();
    if (ID3D12CommandQueue* queue = Dx12Backend::commandQueue.exchange(nullptr)) queue->Release();
    g_api = Api::Unknown;
}
