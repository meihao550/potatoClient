/*
 * PotatoClient injector (C++) - the same steps as injector.py, without needing Python.
 * Built by CMake next to client.dll:  build\Release\injector.exe
 *
 *   injector.exe                    injects client.dll from the same folder
 *   injector.exe path\to\some.dll   injects that DLL
 *
 *  1. find the PID of Minecraft.Windows.exe          (CreateToolhelp32Snapshot)
 *  2. copy the DLL to <name>_loaded_<time>.dll       (so the original can be rebuilt while injected)
 *  3. grant "ALL APPLICATION PACKAGES" read access   (sandboxed UWP/GDK processes need it)
 *  4. OpenProcess -> VirtualAllocEx -> WriteProcessMemory(dll path)
 *  5. CreateRemoteThread(start = LoadLibraryW, arg = dll path)
 *     -> the game itself loads our DLL and runs its DllMain.
 */
#include <Windows.h>
#include <AclAPI.h>
#include <TlHelp32.h>
#include <sddl.h>
#include <clocale>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {
    constexpr const wchar_t* kTarget = L"Minecraft.Windows.exe";

    DWORD findPid(const wchar_t* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W entry{ sizeof(entry) };
        DWORD pid = 0;
        for (BOOL ok = Process32FirstW(snap, &entry); ok; ok = Process32NextW(snap, &entry)) {
            if (_wcsicmp(entry.szExeFile, name) == 0) { pid = entry.th32ProcessID; break; }
        }
        CloseHandle(snap);
        return pid;
    }

    // *S-1-15-2-1 = ALL APPLICATION PACKAGES. Without it an AppContainer process can't open the DLL.
    bool grantAppPackages(const fs::path& file) {
        PSID sid = nullptr;
        if (!ConvertStringSidToSidW(L"S-1-15-2-1", &sid)) return false;
        PACL oldAcl = nullptr, newAcl = nullptr;
        PSECURITY_DESCRIPTOR sd = nullptr;
        bool ok = false;
        if (GetNamedSecurityInfoW(file.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                  nullptr, nullptr, &oldAcl, nullptr, &sd) == ERROR_SUCCESS) {
            EXPLICIT_ACCESSW access{};
            access.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
            access.grfAccessMode = GRANT_ACCESS;
            access.grfInheritance = NO_INHERITANCE;
            access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            access.Trustee.ptstrName = static_cast<LPWSTR>(sid);
            if (SetEntriesInAclW(1, &access, oldAcl, &newAcl) == ERROR_SUCCESS) {
                ok = SetNamedSecurityInfoW(const_cast<LPWSTR>(file.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                           nullptr, nullptr, newAcl, nullptr) == ERROR_SUCCESS;
                LocalFree(newAcl);
            }
            LocalFree(sd);
        }
        LocalFree(sid);
        return ok;
    }

    // Inject a timestamped copy so the original can be rebuilt while the game holds the copy open
    fs::path makeCopy(const fs::path& dll) {
        const std::wstring prefix = dll.stem().wstring() + L"_loaded_";
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dll.parent_path(), ec)) {
            const std::wstring name = entry.path().filename().wstring();
            if (name.rfind(prefix, 0) == 0 && entry.path().extension() == dll.extension())
                fs::remove(entry.path(), ec);   // fails (and is skipped) while still loaded in the game
        }
        fs::path copy = dll.parent_path() / (prefix + std::to_wstring(std::time(nullptr)) + dll.extension().wstring());
        fs::copy_file(dll, copy, fs::copy_options::overwrite_existing);
        return copy;
    }

    // Returns 0 on success, otherwise prints why and returns an exit code
    int inject(const fs::path& original) {
        if (!fs::is_regular_file(original)) {
            wprintf(L"DLL が見つかりません: %s\n", original.c_str());
            return 1;
        }
        const DWORD pid = findPid(kTarget);
        if (!pid) {
            wprintf(L"%s が起動していません。先にマイクラを起動して、ワールドに入ってください。\n", kTarget);
            return 2;
        }

        fs::path dll;
        try {
            dll = makeCopy(original);
        } catch (const fs::filesystem_error& e) {
            printf("DLL のコピーに失敗しました: %s\n", e.what());
            return 3;
        }
        if (!grantAppPackages(dll)) wprintf(L"警告: DLL にゲームからの読み取り権限を付けられませんでした\n");

        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!process) {
            wprintf(L"OpenProcess 失敗 (error %lu)\n", GetLastError());
            return 4;
        }

        const std::wstring path = dll.wstring();
        const SIZE_T size = (path.size() + 1) * sizeof(wchar_t);
        int result = 0;
        void* remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remote) {
            wprintf(L"VirtualAllocEx 失敗 (error %lu)\n", GetLastError());
            result = 5;
        } else if (!WriteProcessMemory(process, remote, path.c_str(), size, nullptr)) {
            wprintf(L"WriteProcessMemory 失敗 (error %lu)\n", GetLastError());
            result = 6;
        } else {
            // kernel32.dll is mapped at the same address in every process, so our LoadLibraryW
            // address is valid inside the game too.
            auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
                GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
            HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
            if (!thread) {
                wprintf(L"CreateRemoteThread 失敗 (error %lu)\n", GetLastError());
                result = 7;
            } else {
                WaitForSingleObject(thread, 10000);
                DWORD code = 0;
                GetExitCodeThread(thread, &code);   // low 32 bits of the HMODULE
                CloseHandle(thread);
                if (code == 0) {
                    wprintf(L"LoadLibraryW が失敗しました (DLL の依存関係 / 権限を確認)\n");
                    result = 8;
                } else {
                    wprintf(L"Inject 成功! (PID %lu)  Insert でメニュー、End でアンロード\n", pid);
                }
            }
        }
        if (remote) VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return result;
    }

    fs::path exeFolder() {
        wchar_t buf[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buf, MAX_PATH);
        return fs::path(buf).parent_path();
    }

    // Double-clicked (the console belongs to us alone): keep the window open to show the result
    void pauseIfOwnConsole() {
        DWORD processes[2];
        if (GetConsoleProcessList(processes, 2) == 1) {
            wprintf(L"\nEnter で閉じます...");
            (void)getchar();
        }
    }
}

int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);
    _wsetlocale(LC_ALL, L".UTF8");
    const fs::path dll = argc > 1 ? fs::absolute(argv[1]) : exeFolder() / L"client.dll";
    wprintf(L"対象: %s\nDLL : %s\n", kTarget, dll.c_str());
    const int result = inject(dll);
    pauseIfOwnConsole();
    return result;
}
