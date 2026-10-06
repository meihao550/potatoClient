#pragma once
#include "BlockType.h"
#include "Offsets.h"
#include "core/Memory.h"
#include <cstdint>

struct Vec3 { float x, y, z; };
struct AABB { Vec3 min, max; };

// For players pos is at eye level: pos.y = aabb.min.y (feet) + 1.62
struct StateVectorComponent { Vec3 pos, posPrev, posDelta; };
struct AABBShapeComponent { AABB aabb; float width, height; };
// Degrees. yaw 0 = facing +Z (south), -90 = facing +X (east)
struct ActorRotationComponent { float pitch, yaw, prevPitch, prevYaw; };

class BlockSource {
public:
    short getMinHeight() {
        return Memory::callVirtual<short>(this, Offsets::BlockSource::VIndex::getMinHeight);
    }
    // Y of the first free block above the highest solid block in column (x, z)
    short getAboveTopSolidBlock(int x, int z, bool includeWater, bool includeLeaves) {
        return Memory::callVirtual<short, int, int, bool, bool>(
            this, Offsets::BlockSource::VIndex::getAboveTopSolidBlock, x, z, includeWater, includeLeaves);
    }
};

class ItemStack {
public:
    template <class T> T& at(size_t off) { return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + off); }

    // nullptr for an empty slot
    uint8_t* item() {
        auto* counter = at<uint8_t**>(Offsets::ItemStack::item);
        return counter ? *counter : nullptr;
    }
    uint8_t& count()      { return at<uint8_t>(Offsets::ItemStack::count); }
    uint8_t maxStackSize() { return item()[Offsets::Item::maxStackSize]; }
    const char* name()    { return reinterpret_cast<MsvcString*>(item() + Offsets::Item::fullName)->c_str(); }
    bool isEmpty()        { return !item() || count() == 0; }
};

// A list of item slots: the player inventory, the two hands, chests, ...
// Everything that changes slots goes through the game's own functions, so the
// container tells its listeners (and the server tells the client) about it.
class Container {
public:
    ItemStack* getItem(int slot) {
        return Memory::callVirtual<ItemStack*, int>(this, Offsets::Container::VIndex::getItem, slot);
    }
    void setItem(int slot, const ItemStack& item) {   // copies item
        Memory::callVirtual<void, int, const ItemStack&>(this, Offsets::Container::VIndex::setItem, slot, item);
    }
    void removeItem(int slot, int count) {
        Memory::callVirtual<void, int, int>(this, Offsets::Container::VIndex::removeItem, slot, count);
    }
    int size() {
        return Memory::callVirtual<int>(this, Offsets::Container::VIndex::getContainerSize);
    }
};

class Actor {
public:
    template <class T> T& at(size_t off) { return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + off); }

    StateVectorComponent* stateVector() { return at<StateVectorComponent*>(Offsets::Actor::stateVector); }
    AABBShapeComponent* aabbShape()     { return at<AABBShapeComponent*>(Offsets::Actor::aabbShape); }
    ActorRotationComponent* rotation()  { return at<ActorRotationComponent*>(Offsets::Actor::rotation); }

    BlockSource* blockSource() {
        auto* dimension = at<uint8_t*>(Offsets::Actor::dimension);
        return dimension ? *reinterpret_cast<BlockSource**>(dimension + Offsets::Dimension::blockSource) : nullptr;
    }

    // The item in the selected hotbar slot. It is a reference into the inventory, so writing
    // to it changes the real stack (an empty hand returns a shared empty stack - check item()).
    ItemStack* getCarriedItem() {
        return Memory::callVirtual<ItemStack*>(this, Offsets::Actor::VIndex::getCarriedItem);
    }

    // Copies item into the off hand (the game's own setter, like /replaceitem)
    void setOffhandSlot(const ItemStack& item) {
        Memory::callVirtual<void, const ItemStack&>(this, Offsets::Actor::VIndex::setOffhandSlot, item);
    }

    // Same call the /tp command ends up in. pos uses the same space as StateVectorComponent::pos (eye level).
    void teleportTo(const Vec3& pos) {
        Memory::callVirtual<void, const Vec3&, bool, int, int, bool>(
            this, Offsets::Actor::VIndex::teleportTo, pos, true, 0, 0, false);
    }

    // Moves the player so its feet (bottom center of the hitbox) end up at `feet`.
    // server = this is our ServerPlayer (single-player): teleportTo, like /tp.
    // client = our LocalPlayer on a remote server: the client reports its own position
    //          to the server every tick, so we just write the new position ourselves.
    void moveFeetTo(const Vec3& feet, bool server) {
        StateVectorComponent* state = stateVector();
        AABBShapeComponent* shape = aabbShape();
        if (!state || !shape) return;
        const Vec3 eye{ feet.x, feet.y + (state->pos.y - shape->aabb.min.y), feet.z };
        if (server) { teleportTo(eye); return; }

        const Vec3 d{ eye.x - state->pos.x, eye.y - state->pos.y, eye.z - state->pos.z };
        state->pos = eye;
        state->posPrev = eye;          // no interpolation from the old spot
        state->posDelta = { 0, 0, 0 };
        for (Vec3* v : { &shape->aabb.min, &shape->aabb.max }) { v->x += d.x; v->y += d.y; v->z += d.z; }
    }
};
