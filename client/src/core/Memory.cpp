#include "Memory.h"
#include "Logger.h"
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <cstring>
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
    // Anchor = the first fixed byte. memchr jumps from one candidate to the next, which is
    // far faster than testing every position.
    size_t anchor = 0;
    while (anchor < bytes.size() && !bytes[anchor]) ++anchor;
    if (anchor == bytes.size()) return 0;   // empty, or wildcards only

    const auto* moduleStart = reinterpret_cast<const uint8_t*>(moduleBase());
    const auto* moduleEnd = moduleStart + moduleSize();
    const uint8_t* first = nullptr;
    int matches = 0;

    // Only scan committed, readable pages of the exe itself to avoid access violations
    for (const uint8_t* p = moduleStart; p < moduleEnd;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(p, &mbi, sizeof(mbi))) break;
        const uint8_t* regionEnd = (std::min)(static_cast<const uint8_t*>(mbi.BaseAddress) + mbi.RegionSize, moduleEnd);
        const bool readable = mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE)) &&
            !(mbi.Protect & PAGE_GUARD);
        if (readable && static_cast<size_t>(regionEnd - p) >= bytes.size()) {
            const uint8_t* lastStart = regionEnd - bytes.size();
            for (const uint8_t* s = p; s <= lastStart;) {
                const auto* hit = static_cast<const uint8_t*>(
                    memchr(s + anchor, *bytes[anchor], static_cast<size_t>(lastStart - s) + 1));
                if (!hit) break;
                const uint8_t* candidate = hit - anchor;
                bool match = true;
                for (size_t j = 0; j < bytes.size(); ++j)
                    if (bytes[j] && candidate[j] != *bytes[j]) { match = false; break; }
                if (match && !matches++) first = candidate;
                s = candidate + 1;
            }
        }
        p = regionEnd;
    }
    // After a game update a signature can start matching other code too; the first match
    // may then be the wrong function, so say so.
    if (matches > 1)
        LOG("warning: signature matches %d places, using the first (exe+%#llx): %.*s",
            matches, rva(reinterpret_cast<uintptr_t>(first)), static_cast<int>(pattern.size()), pattern.data());
    return reinterpret_cast<uintptr_t>(first);
}

uintptr_t Memory::scanOrLog(const char* name, std::string_view pattern) {
    const uintptr_t hit = findSig(pattern);
    if (hit) LOG("%s at exe+%#llx", name, rva(hit));
    else LOG("%s: signature not found (game updated? see Offsets.h)", name);
    return hit;
}

unsigned long long Memory::rva(uintptr_t address) {
    return static_cast<unsigned long long>(address - moduleBase());
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
