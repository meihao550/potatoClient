"""
Dump Minecraft.Windows.exe from memory.

The GDK build on disk is encrypted / access-restricted, so static analysis tools
can't open it. While the game runs, the decrypted image sits in its memory; this
script copies it out and fixes the section headers (raw offset = virtual address)
so Ghidra / IDA / pefile can load the result.

usage:  python tools/dump_image.py [out.exe]      (game must be running)
"""
import ctypes
import struct
import sys
from ctypes import wintypes

TARGET = "Minecraft.Windows.exe"
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
LIST_MODULES_ALL = 0x03
PAGE = 0x1000

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(ctypes.c_void_p), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
psapi.EnumProcesses.argtypes = [ctypes.POINTER(wintypes.DWORD), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
psapi.GetModuleBaseNameW.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.LPWSTR, wintypes.DWORD]


class MODULEINFO(ctypes.Structure):
    _fields_ = [("lpBaseOfDll", ctypes.c_void_p), ("SizeOfImage", wintypes.DWORD), ("EntryPoint", ctypes.c_void_p)]


psapi.GetModuleInformation.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.POINTER(MODULEINFO), wintypes.DWORD]


def open_target():
    pids = (wintypes.DWORD * 4096)()
    needed = wintypes.DWORD()
    psapi.EnumProcesses(pids, ctypes.sizeof(pids), ctypes.byref(needed))
    for pid in pids[: needed.value // 4]:
        h = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
        if not h:
            continue
        name = ctypes.create_unicode_buffer(260)
        if psapi.GetModuleBaseNameW(h, None, name, 260) and name.value.lower() == TARGET.lower():
            return h, pid
        kernel32.CloseHandle(h)
    sys.exit(f"{TARGET} is not running")


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "Minecraft.Windows.dump.exe"
    process, pid = open_target()
    modules = (ctypes.c_void_p * 1)()
    psapi.EnumProcessModulesEx(process, modules, ctypes.sizeof(modules), ctypes.byref(wintypes.DWORD()), LIST_MODULES_ALL)
    info = MODULEINFO()
    psapi.GetModuleInformation(process, modules[0], ctypes.byref(info), ctypes.sizeof(info))
    base, size = info.lpBaseOfDll, info.SizeOfImage
    print(f"pid {pid}  base {base:#x}  size {size:#x}")

    image = bytearray(size)
    page = (ctypes.c_char * PAGE)()
    unreadable = 0
    for off in range(0, size, PAGE):
        read = ctypes.c_size_t()
        if kernel32.ReadProcessMemory(process, base + off, page, PAGE, ctypes.byref(read)):
            image[off:off + PAGE] = page.raw
        else:
            unreadable += 1
    print(f"unreadable pages: {unreadable}")

    # Rewrite section headers so file layout == memory layout, and record the runtime base
    e_lfanew = struct.unpack_from("<I", image, 0x3C)[0]
    num_sections = struct.unpack_from("<H", image, e_lfanew + 6)[0]
    opt_size = struct.unpack_from("<H", image, e_lfanew + 20)[0]
    struct.pack_into("<Q", image, e_lfanew + 24 + 24, base)   # OptionalHeader.ImageBase (PE32+)
    sec = e_lfanew + 24 + opt_size
    for i in range(num_sections):
        hdr = sec + i * 40
        vsize, vaddr = struct.unpack_from("<II", image, hdr + 8)
        struct.pack_into("<II", image, hdr + 16, vsize, vaddr)  # SizeOfRawData, PointerToRawData
    with open(out, "wb") as f:
        f.write(image)
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
