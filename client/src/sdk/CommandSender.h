#pragma once
#include "Actor.h"
#include <string>

// Sends a slash command to the server, exactly like typing it in chat
// (CommandRequestPacket). The server checks permissions as usual, so e.g.
// /enchant only works with operator permission.
namespace CommandSender {
    bool init();   // finds the CommandRequestPacket constructor (signature)
    bool available();
    // Client thread only (PlayerTick::run(Side::Client, ...)). command includes the '/'.
    bool send(Actor& localPlayer, const std::string& command);
}
