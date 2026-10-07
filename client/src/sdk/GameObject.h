#pragma once
#include <cstddef>
#include <cstdint>

// Base of our views of game objects (Actor, ItemStack, BlockType, ...). We never construct
// these: `this` points into the game's memory and fields are read at the offsets in Offsets.h.
// No virtual functions and no data, so deriving from it doesn't change the object's layout.
class GameObject {
public:
    template <class T> T& at(size_t off) {
        return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + off);
    }
    template <class T> const T& at(size_t off) const {
        return *reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(this) + off);
    }
};
