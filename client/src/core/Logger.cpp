#include "Logger.h"
#include <Windows.h>
#include <cstdarg>
#include <mutex>
#include <share.h>
#include <string>

extern HMODULE g_module;

namespace {
    FILE* g_console = nullptr;
    FILE* g_file = nullptr;
    std::mutex g_mutex;
}

void Logger::init() {
    AllocConsole();
    SetConsoleTitleW(L"LearnClient console");
    freopen_s(&g_console, "CONOUT$", "w", stdout);

    // client.log next to client.dll
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(g_module, path, MAX_PATH);
    std::wstring logPath = path;
    logPath = logPath.substr(0, logPath.find_last_of(L"\\/") + 1) + L"client.log";
    g_file = _wfsopen(logPath.c_str(), L"w", _SH_DENYNO);   // shared, so it can be read while injected
}

void Logger::shutdown() {
    std::lock_guard lock(g_mutex);
    if (g_file) { fclose(g_file); g_file = nullptr; }
    if (g_console) { fclose(g_console); g_console = nullptr; }
    FreeConsole();
}

void Logger::log(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    std::lock_guard lock(g_mutex);
    if (g_console) { fprintf(stdout, "[client] %s\n", buf); fflush(stdout); }
    if (g_file) { fprintf(g_file, "%s\n", buf); fflush(g_file); }
}
