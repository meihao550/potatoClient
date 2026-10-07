#pragma once
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>

// Strict number parsing for command arguments: the whole word must be a number,
// otherwise the result is empty (atoi would silently turn "abc" into 0).
namespace Args {
    // "12.5" -> 12.5. Empty, "12x", "nan", "inf" -> nothing
    inline std::optional<float> parseFloat(const std::string& text) {
        if (text.empty()) return std::nullopt;
        char* end = nullptr;
        const float value = std::strtof(text.c_str(), &end);
        if (*end || !std::isfinite(value)) return std::nullopt;
        return value;
    }

    // "12" -> 12. Empty, "1.5", "abc", out of int range -> nothing
    inline std::optional<int> parseInt(const std::string& text) {
        if (text.empty()) return std::nullopt;
        char* end = nullptr;
        errno = 0;
        const long value = std::strtol(text.c_str(), &end, 10);
        if (*end || errno == ERANGE || value < INT_MIN || value > INT_MAX) return std::nullopt;
        return static_cast<int>(value);
    }
}
