#include "Memory.h"
#include <Windows.h>
#include <Psapi.h>
#include <optional>
#include <string>
#include <vector>

namespace {
    MODULEINFO g_info{};

    void ensureInfo() {
        if (g_info.lpBaseOfDll) return;
        GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(nullptr), &g_info, sizeof(g_info));
    }

    std::vector<std::optional<uint8_t>> parse(std::string_view pattern) {
        std::vector<std::optional<uint8_t>> bytes;
        for (size_t i = 0; i < pattern.size();) {
            if (pattern[i] == ' ') { ++i; continue; }
            if (pattern[i] == '?') {
                bytes.emplace_back(std::nullopt);
                i += (i + 1 < pattern.size() && pattern[i + 1] == '?') ? 2 : 1;
                continue;
            }
            bytes.emplace_back(static_cast<uint8_t>(strtoul(std::string(pattern.substr(i, 2)).c_str(), nullptr, 16)));
            i += 2;
        }
        return bytes;
    }
}

uintptr_t Memory::moduleBase() { ensureInfo(); return reinterpret_cast<uintptr_t>(g_info.lpBaseOfDll); }
size_t Memory::moduleSize() { ensureInfo(); return g_info.SizeOfImage; }

uintptr_t Memory::findSig(std::string_view pattern) {
    const auto bytes = parse(pattern);
    if (bytes.empty()) return 0;
    const auto* start = reinterpret_cast<const uint8_t*>(moduleBase());
    const size_t size = moduleSize();

    // Only scan committed, executable/readable pages to avoid access violations
    for (size_t off = 0; off < size;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(start + off, &mbi, sizeof(mbi))) break;
        const auto* regionStart = static_cast<const uint8_t*>(mbi.BaseAddress);
        const size_t regionSize = mbi.RegionSize;
        const bool readable = mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE)) &&
            !(mbi.Protect & PAGE_GUARD);
        if (readable && regionSize >= bytes.size()) {
            for (size_t i = 0; i + bytes.size() <= regionSize; ++i) {
                bool match = true;
                for (size_t j = 0; j < bytes.size(); ++j) {
                    if (bytes[j] && regionStart[i + j] != *bytes[j]) { match = false; break; }
                }
                if (match) return reinterpret_cast<uintptr_t>(regionStart + i);
            }
        }
        off = static_cast<size_t>(regionStart + regionSize - start);
    }
    return 0;
}

bool Memory::safeRead(const void* src, void* dst, size_t size) {
    __try {
        memcpy(dst, src, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

uintptr_t Memory::resolveRel32(uintptr_t instr, int offset, int size) {
    if (!instr) return 0;
    const int32_t rel = *reinterpret_cast<int32_t*>(instr + offset);
    return instr + size + rel;
}
