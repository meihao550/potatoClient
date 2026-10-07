#include "CommandSender.h"
#include "core/Logger.h"
#include <cstddef>
#include <string_view>

/*
 * Sending a command like the chat box does
 * ----------------------------------------
 * The string "CommandRequestPacket" -> CommandRequestPacket::getName -> the packet's vtable
 * -> the code that writes that vtable: its constructor and the function that sends what you
 * type in chat. That function does:
 *
 *   CommandRequestPacketPayload payload(context);       // command string + origin data + version
 *   CommandRequestPacket packet(std::move(payload));
 *   level->getPacketSender()->send(packet);             // Level vtable 328, PacketSender vtable 2
 *
 * We fill the payload ourselves (origin "Player" - the server uses the connection's player
 * anyway) and do the same two calls.
 *
 * Memory: the packet takes over the command string's heap buffer. That buffer comes from
 * our DLL's CRT, so we take it back before destroying the packet - the game must not free it.
 */

namespace {
    // The padding alignas(8) adds is the point: it reproduces the game's layout (checked below)
#pragma warning(push)
#pragma warning(disable : 4324)   // structure was padded due to alignment specifier
    // Layout read from the constructor (offsets into the payload it moves from)
    struct CommandRequestPayload {
        std::string command;          // 0x00
        uint8_t originType = 0;       // 0x20 CommandOriginType::Player
        alignas(8) uint8_t uuid[16] = {};   // 0x28 mce::UUID (echoed back in the command output)
        std::string requestId;        // 0x38
        int64_t playerId = 0;         // 0x58 only used for dev console / test origins
        int32_t version = Offsets::CommandRequestPacket::commandVersion;   // 0x60
        bool internalSource = false;  // 0x64
    };
#pragma warning(pop)
    static_assert(sizeof(std::string) == 0x20, "MSVC std::string layout");
    static_assert(offsetof(CommandRequestPayload, uuid) == 0x28 && offsetof(CommandRequestPayload, requestId) == 0x38 && offsetof(CommandRequestPayload, playerId) == 0x58 &&
                  offsetof(CommandRequestPayload, version) == 0x60 && offsetof(CommandRequestPayload, internalSource) == 0x64,
                  "payload layout");

    using Ctor_t = void (*)(void* packet, CommandRequestPayload* payload);
    using GetName_t = std::string_view* (*)(void* self, std::string_view* out);
    Ctor_t g_ctor = nullptr;
}

bool CommandSender::init() {
    const uintptr_t hit = Memory::scanOrLog("CommandRequestPacket ctor", Offsets::Sig::commandRequestPacketCtor);
    if (!hit) return false;
    // Double-check: the vtable it writes must name itself "CommandRequestPacket"
    auto** vtable = reinterpret_cast<void**>(Memory::resolveRel32(hit + Offsets::Sig::commandRequestPacketVtableLea, 3, 7));
    std::string_view name;
    reinterpret_cast<GetName_t>(vtable[Offsets::CommandRequestPacket::VIndex::getName])(nullptr, &name);
    if (name != "CommandRequestPacket") {
        LOG("CommandRequestPacket check failed (vtable names itself '%.*s')", static_cast<int>(name.size()), name.data());
        return false;
    }
    g_ctor = reinterpret_cast<Ctor_t>(hit);
    return true;
}

bool CommandSender::available() { return g_ctor != nullptr; }

bool CommandSender::send(Actor& localPlayer, const std::string& command) {
    void* level = localPlayer.at<void*>(Offsets::Actor::level);
    if (!g_ctor || !level) return false;
    void* sender = Memory::callVirtual<void*>(level, Offsets::Level::VIndex::getPacketSender);
    if (!sender) return false;

    alignas(16) uint8_t packet[0x100] = {};
    static_assert(sizeof(packet) >= Offsets::CommandRequestPacket::size);
    CommandRequestPayload payload;
    payload.command = command;
    g_ctor(packet, &payload);   // moves command / requestId out of the payload

    Memory::callVirtual<void, void*>(sender, Offsets::PacketSender::VIndex::send, packet);

    // Take our string buffer back (it leaves an empty string in the packet), then destroy the packet
    std::string reclaimed = std::move(*reinterpret_cast<std::string*>(packet + Offsets::CommandRequestPacket::command));
    Memory::callVirtual<void, uint32_t>(packet, Offsets::CommandRequestPacket::VIndex::destructor, 0u);
    LOG("sent command: %s", reclaimed.c_str());
    return true;
}
