#pragma once
#include <Windows.h>
#include <atomic>

/*
 * Unloading safely
 * ----------------
 * Game threads can be inside one of our detours (Present, normalTick, the WndProc, ...)
 * at the moment End is pressed. If the DLL is freed then, they return into memory that no
 * longer exists and the game crashes. Every detour puts an InFlight::Guard on its first
 * line, and unloading waits until the count drops to zero after the hooks are disabled.
 */
namespace InFlight {
    inline std::atomic<int> count = 0;

    struct Guard {
        Guard() { ++count; }
        ~Guard() { --count; }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    };

    // Returns false if detours were still running when the timeout ran out
    inline bool waitUntilIdle(unsigned long timeoutMs) {
        const ULONGLONG deadline = GetTickCount64() + timeoutMs;
        while (count > 0) {
            if (GetTickCount64() >= deadline) return false;
            Sleep(10);
        }
        return true;
    }
}
