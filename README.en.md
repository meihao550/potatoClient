# PotatoClient — a Minecraft Bedrock client

[日本語](README.md) | English

A Horion-style internal (DLL) client built from scratch to learn how cheat clients work.
Target: **Minecraft Bedrock 1.26.52 (GDK build / `Minecraft.Windows.exe`)**

> Use it only in single-player or in your own worlds. Using it on public servers or Realms breaks their rules and can get you banned.
> Some features also work in multiplayer, but only use them on servers that allow it.

## How to use

### 1. Requirements
- Windows 10 / 11 (x64)
- Minecraft Bedrock **1.26.52** (the GDK build, installed from the Microsoft Store or the Xbox app)
- Visual Studio 2019 Build Tools (with the "Desktop development with C++" workload)
- Python 3.10 or later (64-bit)
- git (the build downloads MinHook and Dear ImGui automatically)

### 2. Build
Run these commands in the project folder (the one that contains `CMakeLists.txt`):
```
set CMAKE="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
%CMAKE% -S . -B build -G "Visual Studio 16 2019" -A x64
%CMAKE% --build build --config Release
```
The build succeeded if `build\Release\client.dll` exists (`client.pdb` is for crash analysis).
After the first build, only the `--build` line needs to be run again when you change the source.

> The CMake project is named `LearnClient`, and the injector's docstring still uses that name. It is the same project as PotatoClient.

