# Contributing to PotatoClient

Thanks for your interest! PotatoClient is a learning project: it exists to show how an internal
game client works. Contributions that make the code clearer, fix bugs, keep it working after game
updates, or add well-explained features are welcome.

Before you start, read [docs/READING_THE_CODE.md](docs/READING_THE_CODE.md). It explains the
layout, the threading rules and how to add modules and commands.

## Ground rules

- **Test only in single-player or in a test world you own.** Don't run the client on public servers
  or Realms. That breaks their rules and can get accounts banned. Don't submit features whose only
  purpose is to harm other players or servers.
- Keep the project educational. Explain *how* a technique works (in a block comment at the top
  of the file, as the existing code does), not just *that* it works.

## Prerequisites

- Windows 10 / 11 (x64)
- Minecraft Bedrock **1.26.52**, GDK build (needed to test, not to build)
- Visual Studio 2019 Build Tools with "Desktop development with C++", **or** Docker in Windows-containers mode
- Python 3.10 or later (64-bit) for `injector/injector.py` and `tools/dump_image.py`
- git

## Building

### Locally

From the project folder:
```
set CMAKE="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
%CMAKE% -S . -B build -G "Visual Studio 16 2019" -A x64
%CMAKE% --build build --config Release
```
Output: `build\Release\client.dll` and `client.pdb`. New `.cpp` files under `client/src/` are
picked up automatically.

### With Docker

The `Dockerfile` builds the DLL in a Windows container. It can only build: injecting into the game
has to happen on the host.
```
docker build -t potatoclient-build .
docker create --name potatoclient-out potatoclient-build
docker cp potatoclient-out:C:\out .\out
docker rm potatoclient-out
```
On Windows 10 add `--build-arg WINDOWS_VERSION=ltsc2019`, or build with `--isolation=hyperv`.

> MinHook is fetched from its `master` branch (`CMakeLists.txt`), so two builds made at different
> times may use different MinHook versions. Keep this in mind if a build suddenly breaks.

## Testing your change

There are no automated tests. The game itself is the test environment.

1. Build in Release with no new warnings (the project compiles with `/W3`).
2. Start the game, enter a **single-player** world, and run `python injector\injector.py`.
3. Exercise your change. Watch the console window and `build\Release\client.log`.
4. Press **End** to unload, and check that the game keeps running and that your module cleans up
   after itself (for example, Xray restores the original blocks in `onDisable`).
5. Inject again to make sure re-injection still works.

If the game crashes, the faulting offset in `client.dll` (Event Viewer → Windows Logs →
Application) plus `client.pdb` tells you the source line.

## Code style

Follow the surrounding code. In short:

- C++20, 4-space indentation, braces on the same line, `#pragma once` in headers.
- PascalCase for classes, files and namespaces. camelCase for functions.
- `m_` for members, `g_` for file-local globals (inside an anonymous namespace), `hk` / `o` for hook detours / originals.
- **Comments in English. User-facing strings (menu, descriptions, command messages) in Japanese.**
- Every offset, vtable index and signature goes in `client/src/sdk/Offsets.h`, with a comment that
  says what it is and how it was found. No magic numbers elsewhere.
- Touch game objects only on the game threads (`Module::onTick` or `PlayerTick::run*`), never
  from the render thread. See the threading section of [READING_THE_CODE.md](docs/READING_THE_CODE.md#4-threading-model).

Details and examples are in [READING_THE_CODE.md](docs/READING_THE_CODE.md#7-naming-and-style-conventions).

