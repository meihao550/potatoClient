#pragma once
#include "BlockType.h"
#include <vector>

namespace BlockRegistry {
    // Locates BlockTypeRegistry::mBlockLookupMap. Returns false if not found (e.g. not in a world yet).
    bool find();
    // Every registered BlockType (walks the std::map). Empty if find() failed.
    std::vector<BlockType*> all();
}
