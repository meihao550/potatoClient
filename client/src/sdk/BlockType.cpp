#include "BlockType.h"
#include "core/Memory.h"

std::vector<Block*> BlockType::permutations() {
    struct Vec { Block** begin; Block** end; Block** cap; };
    std::vector<Block*> out;
    Vec v{};
    if (!Memory::safeRead(&at<Vec>(Offsets::BlockType::permutations), &v, sizeof(v))) return out;
    const size_t count = static_cast<size_t>(v.end - v.begin);
    if (v.end < v.begin || count > 1 << 16) return out;

    std::vector<Block*> raw(count);
    if (count && !Memory::safeRead(v.begin, raw.data(), count * sizeof(Block*))) return out;
    for (Block* state : raw) {
        BlockType* owner = nullptr;
        if (state && Memory::safeRead(reinterpret_cast<uint8_t*>(state) + Offsets::Block::blockType, &owner, sizeof(owner)) &&
            owner == this)
            out.push_back(state);
    }
    return out;
}
