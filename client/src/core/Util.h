#pragma once
#include <cctype>
#include <string>

// Small helpers shared by modules, commands and the SDK.
namespace Util {
    constexpr float kPi = 3.14159265f;
    constexpr float kDegToRad = kPi / 180.0f;

    // ASCII lower case ("UP" -> "up"); other bytes (e.g. UTF-8 Japanese) are left alone
    inline std::string toLower(std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }
}
