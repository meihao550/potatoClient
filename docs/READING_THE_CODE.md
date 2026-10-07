# Reading the code

This guide is a map for people who want to understand, modify or extend PotatoClient.
It explains what each file is for, which order to read them in, and the few rules
(threads, game memory) that the whole codebase is built around.

For usage instructions see [README.en.md](../README.en.md). For the contribution workflow see
[CONTRIBUTING.md](../CONTRIBUTING.md).

## 1. What this project is

PotatoClient is an **internal client** for Minecraft Bedrock 1.26.52 (the GDK build,
`Minecraft.Windows.exe`). It has two parts:

- **`client.dll`** (C++20, built with CMake and MSVC). Once it is loaded into the game process it hooks
  the game's rendering, input and player tick. It draws an ImGui menu and changes game state
  by reading and writing the game's own objects in memory.
- **`injector/injector.py`** (Python, standard library only). It makes the game load the DLL with
  the classic `CreateRemoteThread(LoadLibraryW)` technique.

There is no game API and no mod loader. Everything the DLL knows about the game (struct
offsets, virtual function slots, byte signatures) was found by reverse engineering and lives in
one file, `client/src/sdk/Offsets.h`.

> The CMake project and the menu title are still called `LearnClient`, the project's working
> name. It is the same thing.

