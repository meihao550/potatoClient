#pragma once
#include <cstdio>

namespace Logger {
    void init();      // opens a console window + log file next to the DLL
    void shutdown();
    void log(const char* fmt, ...);
}

#define LOG(...) Logger::log(__VA_ARGS__)
