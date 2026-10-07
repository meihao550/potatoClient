#include "Input.h"
#include "core/Hooks.h"
#include "core/InFlight.h"
#include "core/InputFocus.h"
#include "core/Logger.h"
#include <Unknwn.h>
#include <cstdint>

/*
 * Camera lock for GameInput
 * -------------------------
 * The GDK build reads mouse/keyboard through Microsoft GameInput v3
 * (GameInputRedist.dll), not window messages. Every frame it asks for an
 * IGameInputReading and calls GetMouseState / GetKeyState on it. All readings
 * share one COM vtable, so we create a reading of our own, take the function
 * pointers out of its vtable and hook them.
 *
 * Mouse-look uses the accumulated positionX/Y. Zeroing it would make the camera
 * jump when the menu closes, so instead we keep an offset that freezes the
 * reported position while the menu is open.
 *
 * Layout from GameInput.h (Microsoft.GameInput NuGet 3.5, API version 3):
 *   IGameInput:        [4] GetCurrentReading(kind, device, IGameInputReading**)
 *   IGameInputReading: [13] GetKeyState(count, GameInputKeyState*)  [14] GetMouseState(GameInputMouseState*)
 */

namespace {
    constexpr uint32_t GameInputKindKeyboard = 0x10;
    constexpr uint32_t GameInputKindMouse = 0x20;
    // IID_IGameInput (v3) {20EFC1C7-5D9A-43BA-B26F-B807FA48609C}
    constexpr GUID IID_IGameInput = { 0x20efc1c7, 0x5d9a, 0x43ba, { 0xb2, 0x6f, 0xb8, 0x07, 0xfa, 0x48, 0x60, 0x9c } };

    struct GameInputMouseState {
        uint32_t buttons;
        uint32_t positions;
        int64_t positionX, positionY;
        int64_t absolutePositionX, absolutePositionY;
        int64_t wheelX, wheelY;
    };
    struct GameInputKeyState;

    using GameInputInitialize_t = HRESULT(WINAPI*)(REFIID, void**);
    using GetCurrentReading_t = HRESULT(STDMETHODCALLTYPE*)(void* self, uint32_t kind, void* device, IUnknown** reading);
    using GetMouseState_t = bool(STDMETHODCALLTYPE*)(void* self, GameInputMouseState* state);
    using GetKeyState_t = uint32_t(STDMETHODCALLTYPE*)(void* self, uint32_t count, GameInputKeyState* states);
    GetMouseState_t oGetMouseState = nullptr;
    GetKeyState_t oGetKeyState = nullptr;

    // What we report = real - offset. While the menu is open the offset grows so the report stays still.
    struct Freeze {
        int64_t offset = 0, lastOut = 0;
        int64_t apply(int64_t real, bool frozen) {
            if (frozen) offset = real - lastOut;
            lastOut = real - offset;
            return lastOut;
        }
    };
    Freeze g_x, g_y, g_wheelX, g_wheelY;

    bool STDMETHODCALLTYPE hkGetMouseState(void* self, GameInputMouseState* state) {
        InFlight::Guard guard;   // see core/InFlight.h
        const bool ok = oGetMouseState(self, state);
        if (ok && state) {
            const bool frozen = InputFocus::overlayHasInput();
            state->positionX = g_x.apply(state->positionX, frozen);
            state->positionY = g_y.apply(state->positionY, frozen);
            state->wheelX = g_wheelX.apply(state->wheelX, frozen);
            state->wheelY = g_wheelY.apply(state->wheelY, frozen);
            if (frozen) state->buttons = 0;   // clicks go to the menu, not the world
        }
        return ok;
    }

    uint32_t STDMETHODCALLTYPE hkGetKeyState(void* self, uint32_t count, GameInputKeyState* states) {
        InFlight::Guard guard;   // see core/InFlight.h
        const uint32_t pressed = oGetKeyState(self, count, states);
        return InputFocus::overlayHasInput() ? 0 : pressed;   // report "no keys held" so the player stops moving
    }

    void** readingVtable(void* gameInput, uint32_t kind) {
        auto getCurrentReading = reinterpret_cast<GetCurrentReading_t>((*reinterpret_cast<void***>(gameInput))[4]);
        IUnknown* reading = nullptr;
        if (FAILED(getCurrentReading(gameInput, kind, nullptr, &reading)) || !reading) return nullptr;
        void** vtable = *reinterpret_cast<void***>(reading);
        reading->Release();
        return vtable;
    }
}

bool Input::hookGameInput() {
    static bool done = false;
    if (done) return true;

    HMODULE dll = GetModuleHandleW(L"GameInputRedist.dll");
    if (!dll) return false;
    auto init = reinterpret_cast<GameInputInitialize_t>(GetProcAddress(dll, "GameInputInitialize"));
    IUnknown* gameInput = nullptr;
    if (!init || FAILED(init(IID_IGameInput, reinterpret_cast<void**>(&gameInput))) || !gameInput) return false;

    // A reading only exists once the device produced input, so this may need a few retries
    void** mouse = readingVtable(gameInput, GameInputKindMouse);
    void** keyboard = readingVtable(gameInput, GameInputKindKeyboard);
    gameInput->Release();
    if (!mouse && !keyboard) return false;

    void** vt = mouse ? mouse : keyboard;
    const bool mouseOk = Hooks::create("IGameInputReading::GetMouseState", vt[14], &hkGetMouseState, oGetMouseState);
    const bool keysOk = Hooks::create("IGameInputReading::GetKeyState", vt[13], &hkGetKeyState, oGetKeyState);
    if (!mouseOk || !keysOk)
        LOG("warning: GameInput hooks incomplete (mouse %d, keys %d); the menu may not block game input", mouseOk, keysOk);
    if (mouse && keyboard && mouse[14] != keyboard[14])
        LOG("warning: mouse and keyboard readings use different vtables; keyboard lock may not work");
    done = true;   // don't retry: a second MH_CreateHook on the same target would fail anyway
    return true;
}
