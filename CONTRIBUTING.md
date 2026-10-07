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

