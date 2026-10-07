#pragma once
#include <cstdint>
#include <string_view>

namespace Memory {
    // Base address / size of Minecraft.Windows.exe in memory
    uintptr_t moduleBase();
    size_t moduleSize();

    // IDA-style pattern scan: "48 8B ?? ?? 89" ('?' or '??' = wildcard).
    // Returns 0 when not found.
    uintptr_t findSig(std::string_view pattern);

    // memcpy that returns false instead of crashing on an invalid address (SEH)
    bool safeRead(const void* src, void* dst, size_t size);

    // Resolve a rel32 operand (e.g. E8/E9 call/jmp, or lea/mov rip-relative)
    // `offset` = where the 4-byte displacement starts, `size` = full instruction length.
    uintptr_t resolveRel32(uintptr_t instr, int offset, int size);

    // Call virtual function number `index` of a game object (x64: `this` goes in rcx like any first argument).
    // Always spell out Args (e.g. <void, const Vec3&, bool>) so references are not turned into copies.
    template <class Ret, class... Args>
    Ret callVirtual(void* self, size_t index, Args... args) {
        using Fn = Ret (*)(void*, Args...);
        return reinterpret_cast<Fn>((*static_cast<void***>(self))[index])(self, args...);
    }
}
