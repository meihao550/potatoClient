"""
LearnClient injector - classic LoadLibrary injection, written with ctypes so every
WinAPI call is visible.

  1. find the PID of Minecraft.Windows.exe          (CreateToolhelp32Snapshot)
  2. grant "ALL APPLICATION PACKAGES" read access   (sandboxed UWP processes need it)
  3. OpenProcess -> VirtualAllocEx -> WriteProcessMemory(dll path)
  4. CreateRemoteThread(start = LoadLibraryW, arg = dll path)
     -> the game itself loads our DLL and runs its DllMain.
"""
import ctypes
import glob
import os
import shutil
import subprocess
import time
import tkinter as tk
from ctypes import wintypes
from tkinter import filedialog

TARGET = "Minecraft.Windows.exe"
DEFAULT_DLL = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "build", "Release", "client.dll"))

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

PROCESS_ALL_ACCESS = 0x1F0FFF
MEM_COMMIT_RESERVE = 0x3000
MEM_RELEASE = 0x8000
PAGE_READWRITE = 0x04
TH32CS_SNAPPROCESS = 0x2


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [("dwSize", wintypes.DWORD), ("cntUsage", wintypes.DWORD),
                ("th32ProcessID", wintypes.DWORD), ("th32DefaultHeapID", ctypes.c_void_p),
                ("th32ModuleID", wintypes.DWORD), ("cntThreads", wintypes.DWORD),
                ("th32ParentProcessID", wintypes.DWORD), ("pcPriClassBase", ctypes.c_long),
                ("dwFlags", wintypes.DWORD), ("szExeFile", wintypes.WCHAR * 260)]


# Declare signatures so 64-bit pointers/handles aren't truncated to int
kernel32.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
kernel32.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(PROCESSENTRY32W)]
kernel32.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(PROCESSENTRY32W)]
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.VirtualAllocEx.restype = ctypes.c_void_p
kernel32.VirtualAllocEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
kernel32.VirtualFreeEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wintypes.DWORD]
kernel32.WriteProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
kernel32.GetModuleHandleW.restype = wintypes.HMODULE
kernel32.GetProcAddress.restype = ctypes.c_void_p
kernel32.GetProcAddress.argtypes = [wintypes.HMODULE, ctypes.c_char_p]
kernel32.CreateRemoteThread.restype = wintypes.HANDLE
kernel32.CreateRemoteThread.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD, ctypes.c_void_p]
kernel32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
kernel32.GetExitCodeThread.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]


def find_pid(name: str) -> int | None:
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    entry = PROCESSENTRY32W()
    entry.dwSize = ctypes.sizeof(entry)
    try:
        ok = kernel32.Process32FirstW(snap, ctypes.byref(entry))
        while ok:
            if entry.szExeFile.lower() == name.lower():
                return entry.th32ProcessID
            ok = kernel32.Process32NextW(snap, ctypes.byref(entry))
    finally:
        kernel32.CloseHandle(snap)
    return None


def grant_app_packages(path: str) -> None:
    # *S-1-15-2-1 = ALL APPLICATION PACKAGES. Without it an AppContainer process can't open the DLL.
    subprocess.run(["icacls", path, "/grant", "*S-1-15-2-1:(RX)"], capture_output=True, check=False)


def make_copy(dll_path: str) -> str:
    """Inject a timestamped copy so the original can be rebuilt while the game holds the copy open."""
    base, ext = os.path.splitext(dll_path)
    for old in glob.glob(f"{base}_loaded_*{ext}"):
        try:
            os.remove(old)          # fails (and is skipped) while still loaded in the game
        except OSError:
            pass
    copy = f"{base}_loaded_{int(time.time())}{ext}"
    shutil.copyfile(dll_path, copy)
    return copy


def inject(dll_path: str) -> str:
    if not os.path.isfile(dll_path):
        return f"DLL が見つかりません: {dll_path}"
    pid = find_pid(TARGET)
    if pid is None:
        return f"{TARGET} が起動していません。先にマイクラを起動してください。"

    dll_path = make_copy(dll_path)
    grant_app_packages(dll_path)
    process = kernel32.OpenProcess(PROCESS_ALL_ACCESS, False, pid)
    if not process:
        return f"OpenProcess 失敗 (error {ctypes.get_last_error()})"

    data = ctypes.create_unicode_buffer(dll_path)
    size = ctypes.sizeof(data)
    remote = None
    try:
        remote = kernel32.VirtualAllocEx(process, None, size, MEM_COMMIT_RESERVE, PAGE_READWRITE)
        if not remote:
            return f"VirtualAllocEx 失敗 (error {ctypes.get_last_error()})"
        if not kernel32.WriteProcessMemory(process, remote, data, size, None):
            return f"WriteProcessMemory 失敗 (error {ctypes.get_last_error()})"

        # kernel32.dll is mapped at the same address in every process, so our LoadLibraryW
        # address is valid inside the game too.
        load_library = kernel32.GetProcAddress(kernel32.GetModuleHandleW("kernel32.dll"), b"LoadLibraryW")
        thread = kernel32.CreateRemoteThread(process, None, 0, load_library, remote, 0, None)
        if not thread:
            return f"CreateRemoteThread 失敗 (error {ctypes.get_last_error()})"
        kernel32.WaitForSingleObject(thread, 10000)
        code = wintypes.DWORD()
        kernel32.GetExitCodeThread(thread, ctypes.byref(code))   # low 32 bits of HMODULE
        kernel32.CloseHandle(thread)
        if code.value == 0:
            return "LoadLibraryW が失敗しました (DLL の依存関係/権限を確認)"
        return f"Inject 成功! (PID {pid})  Insert でメニュー、End でアンロード"
    finally:
        if remote:
            kernel32.VirtualFreeEx(process, remote, 0, MEM_RELEASE)
        kernel32.CloseHandle(process)


def main() -> None:
    root = tk.Tk()
    root.title("LearnClient Injector")
    root.resizable(False, False)

    dll_var = tk.StringVar(value=DEFAULT_DLL)
    status = tk.StringVar(value=f"対象: {TARGET}")

    frame = tk.Frame(root, padx=12, pady=12)
    frame.pack()
    tk.Entry(frame, textvariable=dll_var, width=60).grid(row=0, column=0, padx=(0, 6))
    tk.Button(frame, text="参照...", command=lambda: dll_var.set(
        filedialog.askopenfilename(filetypes=[("DLL", "*.dll")]) or dll_var.get())).grid(row=0, column=1)
    tk.Button(frame, text="Inject", font=("Meiryo", 16, "bold"), width=20, bg="#2d7d46", fg="white",
              command=lambda: status.set(inject(dll_var.get()))).grid(row=1, column=0, columnspan=2, pady=12)
    tk.Label(frame, textvariable=status, wraplength=480).grid(row=2, column=0, columnspan=2)
    root.mainloop()


if __name__ == "__main__":
    main()
