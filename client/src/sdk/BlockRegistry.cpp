#include "BlockRegistry.h"
#include "core/Logger.h"
#include "core/Memory.h"
#include <Windows.h>
#include <cstring>

/*
 * Finding BlockTypeRegistry without a signature
 * ---------------------------------------------
 * The registry lives in the exe's .data section and starts with
 *     std::map<HashedString, std::unique_ptr<BlockType>> mBlockLookupMap;
 * An MSVC std::map is just { Node* head; size_t size; }. So we walk .data 8 bytes
 * at a time, treat each pair of qwords as a possible map, and accept it only if
 * the nodes look exactly right: the key is "minecraft:..." and the value points to
 * a BlockType whose own name field holds the same string. Guesses that are wrong
 * usually point at garbage memory, so every read goes through safeRead().
 */

namespace {
    uintptr_t g_map = 0;   // address of mBlockLookupMap inside .data

    using Memory::safeRead;

    template <class T> bool readT(uintptr_t addr, T& out) {
        return addr > 0x10000 && addr < 0x7FFFFFFFFFFF && safeRead(reinterpret_cast<void*>(addr), &out, sizeof(T));
    }

    bool readString(uintptr_t addr, char* out, size_t outSize) {
        MsvcString s;
        if (!readT(addr, s) || s.size >= outSize || s.size > s.capacity || s.capacity < 15) return false;
        // SSO: strings up to 15 chars are stored inline, longer ones on the heap
        const void* chars = s.capacity > 15 ? static_cast<const void*>(s.ptr) : reinterpret_cast<const void*>(addr);
        if (!safeRead(chars, out, s.size)) return false;
        out[s.size] = 0;
        return true;
    }

    // A node is good if its key is "minecraft:xxx" and BlockType::name says the same
    bool nodeLooksValid(uintptr_t node) {
        namespace MapNode = Offsets::MapNode;
        char key[128], name[128];
        uintptr_t blockType = 0;
        if (!readString(node + MapNode::keyString, key, sizeof(key)) || strncmp(key, "minecraft:", 10) != 0) return false;
        if (!readT(node + MapNode::value, blockType)) return false;
        if (!readString(blockType + Offsets::BlockType::fullName, name, sizeof(name))) return false;
        return strcmp(key, name) == 0;
    }

    bool mapLooksValid(uintptr_t mapAddr) {
        uintptr_t head = 0, root = 0, leftmost = 0;
        size_t size = 0;
        uint8_t isNil = 0;
        if (!readT(mapAddr, head) || !readT(mapAddr + 8, size)) return false;
        if (size < 300 || size > 20000) return false;   // vanilla has ~1500 block types
        if (!readT(head + Offsets::MapNode::isNil, isNil) || isNil != 1) return false;
        if (!readT(head + Offsets::MapNode::parent, root) || !readT(head + Offsets::MapNode::left, leftmost)) return false;
        return nodeLooksValid(root) && nodeLooksValid(leftmost);
    }

    bool dataSection(uintptr_t& start, size_t& size) {
        const uintptr_t base = Memory::moduleBase();
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
            if (memcmp(sec->Name, ".data\0\0\0", 8) == 0) {
                start = base + sec->VirtualAddress;
                size = sec->Misc.VirtualSize;
                return true;
            }
        }
        return false;
    }
}

bool BlockRegistry::find() {
    if (g_map && mapLooksValid(g_map)) return true;
    g_map = 0;
    uintptr_t start = 0;
    size_t size = 0;
    if (!dataSection(start, size)) return false;
    for (uintptr_t p = start; p + 16 <= start + size; p += 8) {
        if (mapLooksValid(p)) {
            g_map = p;
            size_t count = 0;
            readT(p + 8, count);
            LOG("BlockTypeRegistry map found at exe+%#llx (%zu block types)", Memory::rva(p), count);
            return true;
        }
    }
    LOG("BlockTypeRegistry map not found (are you inside a world?)");
    return false;
}

std::vector<BlockType*> BlockRegistry::all() {
    std::vector<BlockType*> out;
    if (!find()) return out;

    using namespace Offsets::MapNode;
    uintptr_t head = 0, root = 0;
    size_t size = 0;
    readT(g_map, head);
    readT(g_map + 8, size);
    readT(head + parent, root);
    out.reserve(size);

    // Iterative tree walk (the map is a red-black tree; nil nodes have isNil == 1)
    std::vector<uintptr_t> stack{ root };
    while (!stack.empty() && out.size() <= size) {
        const uintptr_t node = stack.back();
        stack.pop_back();
        uint8_t nil = 1;
        if (!readT(node + isNil, nil) || nil) continue;
        uintptr_t l = 0, r = 0, bt = 0;
        readT(node + left, l);
        readT(node + right, r);
        if (readT(node + value, bt) && bt) out.push_back(reinterpret_cast<BlockType*>(bt));
        stack.push_back(l);
        stack.push_back(r);
    }
    return out;
}
