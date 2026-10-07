#pragma once
#include "GameObject.h"
#include "Offsets.h"
#include <cstdint>
#include <string>
#include <vector>

// We never construct game objects; we only look at their memory through these helpers.
struct MsvcString {   // layout of std::string in MSVC
    union { char buf[16]; char* ptr; };
    size_t size;
    size_t capacity;
    const char* c_str() const { return capacity > 15 ? ptr : buf; }
};

class Block : public GameObject {
public:
    uint8_t& isOpaqueFullBlock() { return at<uint8_t>(Offsets::Block::isOpaqueFullBlock); }
    uint8_t& light()             { return at<uint8_t>(Offsets::Block::light); }
    uint16_t* occlusionShapes()  { return &at<uint16_t>(Offsets::Block::occlusionShapes); }
};

class BlockType : public GameObject {
public:
    std::string name()       { return at<MsvcString>(Offsets::BlockType::fullName).c_str(); }
    float& translucency()    { return at<float>(Offsets::BlockType::translucency); }
    uint8_t& renderLayer()   { return at<uint8_t>(Offsets::BlockType::renderLayer); }
    uint8_t& flags2()        { return at<uint8_t>(Offsets::BlockType::flags2); }
    uint8_t& lightBlock()    { return at<uint8_t>(Offsets::BlockType::lightBlock); }

    // Valid states only: entries can be null, so each one must point back to this BlockType
    std::vector<Block*> permutations();
};
