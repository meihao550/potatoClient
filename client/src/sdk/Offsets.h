#pragma once
#include <cstddef>

// Structure offsets for Minecraft Bedrock 1.26.52 (Minecraft.Windows.exe, GDK).
// Derived from LeviLamina's BDS 1.26.51 headers (mc/world/level/block/BlockType.h)
// and verified against the running client with tools/dump_image.py + memory reads.
// When the game updates, re-verify these first.
namespace Offsets {
    namespace BlockType {
        constexpr size_t fullName      = 0xE8;   // NameInfo::mFullName.mStr  (std::string, "minecraft:stone")
        constexpr size_t translucency  = 0x15C;  // float
        constexpr size_t renderLayer   = 0x162;  // BlockRenderLayer (uint8)
        constexpr size_t flags2        = 0x164;  // bitfield byte: bit1 mSolid, bit3 mIgnoreBlockForInsideCubeRenderer, bit4 mIsOpaqueFullBlock
        constexpr size_t lightBlock    = 0x166;  // Brightness (uint8, 0..15)
        constexpr size_t permutations  = 0x228;  // std::vector<std::unique_ptr<Block>> mBlockPermutations
        constexpr size_t defaultState  = 0x240;  // Block const*
    }

    // Block = one state/permutation of a BlockType. It caches some BlockType data
    // that the renderer uses for face culling and lighting.
    namespace Block {
        constexpr size_t blockType        = 0x68;  // BlockType*
        constexpr size_t isOpaqueFullBlock = 0xA3; // mDirectData.mIsOpaqueFullBlock (bool)
        constexpr size_t light            = 0xA5;  // mDirectData.mLight (how much light it blocks)
        constexpr size_t occlusionShapes  = 0xBE;  // uint16 mOcclusionShapeHandle + uint16[6] per face (0 = empty, 1 = full cube)
        constexpr size_t occlusionShapeCount = 7;
    }

    // std::map<HashedString, std::unique_ptr<BlockType>> node (MSVC _Tree_node)
    namespace MapNode {
        constexpr size_t left = 0x00, parent = 0x08, right = 0x10, isNil = 0x19;
        constexpr size_t keyString = 0x28;   // HashedString::mStr (HashedString starts at 0x20 with its hash)
        constexpr size_t value     = 0x50;   // BlockType*
    }

    namespace RenderLayer {
        constexpr unsigned char Barrier = 14;  // only drawn while holding a barrier item
    }

    // Actor (base of LocalPlayer). Field order from LeviLamina's Actor.h, checked on the live client.
    namespace Actor {
        constexpr size_t dimension   = 0x1C8;  // WeakRef<Dimension>: first qword is the Dimension*
        constexpr size_t level       = 0x1D8;  // Level* (ClientLevel on the client)
        constexpr size_t stateVector = 0x218;  // BuiltInActorComponents::mStateVectorComponent* (pos, posPrev, posDelta)
        constexpr size_t aabbShape   = 0x220;  // BuiltInActorComponents::mAABBShapeComponent*   (AABB, width, height)
        constexpr size_t rotation    = 0x228;  // BuiltInActorComponents::mActorRotationComponent* (pitch, yaw, prev pitch, prev yaw)
        namespace VIndex {                     // virtual function slots (same order as Actor.h)
            constexpr size_t teleportTo     = 21;  // (Vec3 const& pos, bool stopRiding, int cause, int sourceType, bool keepVelocity)
            constexpr size_t normalTick     = 24;  // called once per game tick on the game thread
            constexpr size_t getCarriedItem = 77;  // ItemStack const& () - the selected hotbar slot inside the inventory
        }
    }

    // ItemStack (= ItemStackBase + net id), 0x98 bytes, stored by value in the inventory's vector
    namespace ItemStack {
        constexpr size_t item  = 0x08;   // WeakPtr<Item>: points to a SharedCounter whose first qword is the Item*
        constexpr size_t count = 0x22;   // uchar mCount
    }

    namespace Item {
        constexpr size_t maxStackSize = 0xA8;   // uchar (64, 16 or 1)
        constexpr size_t fullName     = 0x128;  // HashedString mFullName at 0x120 -> std::string after the 8-byte hash
    }

    namespace Level {
        namespace VIndex {
            constexpr size_t getRuntimeActorList = 325;  // std::vector<Actor*> () const - 3 slots before getPacketSender in ILevel.h
            constexpr size_t getPacketSender = 328;  // PacketSender* () - must be called on the game thread
        }
    }

    namespace PacketSender {
        namespace VIndex {
            constexpr size_t send = 2;   // (Packet&) - on the client: to the server
        }
    }

    // CommandRequestPacket = what the client sends when you type "/..." in chat
    namespace CommandRequestPacket {
        constexpr size_t command = 0x30;   // std::string mCommand (moved in from the payload)
        constexpr size_t size    = 0xA0;   // last field is at 0x98
        constexpr int commandVersion = 0x34;   // what every caller in the game passes (CommandVersion::CurrentVersion)
        namespace VIndex {
            constexpr size_t destructor = 0;   // (uint32 flags) - flags 0 = don't free the memory
            constexpr size_t getName    = 2;   // std::string_view ()
        }
    }

    namespace Dimension {
        constexpr size_t blockSource = 0xF0;   // OwnerPtr<BlockSource> mBlockSource (right after mHeightRange at 0xC8)
    }

    // BlockSource = "the world" as seen from one dimension (getBlock, heights, ...).
    // MSVC puts overloaded virtuals in REVERSE declaration order, so the two
    // getAboveTopSolidBlock overloads are swapped compared to the header.
    namespace BlockSource {
        namespace VIndex {
            constexpr size_t getMinHeight          = 35;  // short ()
            constexpr size_t getAboveTopSolidBlock = 47;  // short (int x, int z, bool includeWater, bool includeLeaves)
        }
    }

    namespace Sig {
        // Inside LocalPlayer::~LocalPlayer:  lea rax, [LocalPlayer::vftable] / mov [rcx], rax / lea r14, [rcx+...] / mov rbx, [rcx+...]
        // The rel32 of the lea (offset 3, instruction length 7) gives the vtable.
        constexpr const char* localPlayerVtable = "48 8D 05 ?? ?? ?? ?? 48 89 01 4C 8D B1 ?? ?? 00 00 48 8B 99";
        // Inside ServerPlayer::ServerPlayer:  lea rcx, [ServerPlayer::vftable] / mov [r12], rcx / mov [r12+...], eax
        constexpr const char* serverPlayerVtable = "48 8D 0D ?? ?? ?? ?? 49 89 0C 24 41 89 84 24 ?? ?? 00 00";
        // Start of CommandRequestPacket::CommandRequestPacket(CommandRequestPacketPayload&&).
        // Found from the vtable whose getName returns "CommandRequestPacket".
        constexpr const char* commandRequestPacketCtor =
            "55 41 57 41 56 56 57 53 48 83 EC 48 48 8D 6C 24 40 48 C7 45 00 FE FF FF FF 49 89 D7 48 89 CE "
            "48 B8 02 00 00 00 01 00 00 00 48 89 41 08 66 C7 41 10 00 00 0F 57 C0 0F 11 41 18 C7 41 28 00 00 00 00 "
            "48 8D 05 ?? ?? ?? ?? 48 89 01";
        // Start of EnchantUtils::applyEnchant(ItemStackBase&, EnchantmentInstance const&, bool allowNonVanilla).
        // Found from EnchantCommand::execute (the function that uses "commands.enchant.success").
        constexpr const char* applyEnchant =
            "55 41 56 56 57 53 48 81 EC ?? ?? ?? ?? 48 8D AC 24 ?? ?? ?? ?? 48 C7 45 ?? FE FF FF FF "
            "44 88 C3 48 89 D7 48 89 CE 4C 8D 75 ?? 4C 89 F2 E8 ?? ?? ?? ?? 48 8B 3F";
    }
}
