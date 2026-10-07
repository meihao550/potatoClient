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

## 2. Repository map

```
CMakeLists.txt          The whole build: fetches MinHook + Dear ImGui, builds client.dll
Dockerfile              Builds client.dll in a Windows container (build only, no injection)
README.md / .en.md      Usage and a "how it works" overview (Japanese / English)
CONTRIBUTING.md         How to build, test and send changes
docs/
  ENCHANT.md            Manual for the `enchant` command (Japanese)
  READING_THE_CODE.md   This file
injector/injector.py    tkinter + ctypes injector (copies the DLL, then LoadLibraryW)
tools/dump_image.py     Dumps the decrypted game exe from memory for Ghidra / IDA
client/src/
  dllmain.cpp           Entry point: start-up order and unload sequence
  core/                 Building blocks with no game knowledge
    Hooks.*             MinHook wrapper: Hooks::create(name, target, detour, &original)
    Logger.*            Console window + client.log, the LOG(...) macro
    Memory.*            Module base, findSig (pattern scan), safeRead, resolveRel32, callVirtual<>
  render/               Getting a frame to draw on
    Renderer.*          Finds the DXGI / D3D12 vtables, hooks Present / ResizeBuffers / ExecuteCommandLists
    Backends.h          Interface of the two ImGui draw backends
    Dx12Backend.cpp     ImGui on D3D12 (what the game normally uses)
    Dx11Backend.cpp     ImGui on D3D11 (fallback)
  gui/                  What the user sees and presses
    Menu.*              Module menu (Insert), command bar (Home), message toast
    Input.*             WndProc subclass (hotkeys), GetRawInputData hook (camera lock)
    GameInputHook.cpp   GameInput v3 hooks: hides mouse/keys from the game while the menu is open
  modules/              Toggleable features
    Module.h            Base class every feature derives from
    ModuleManager.*     Owns the modules, dispatches ticks / renders / key presses
    MoveInput.*         Helper: WASD / Space / Shift -> a direction for movement modules
    Xray, Fly, Speed, Aimbox, InventoryView, AutoTotem   (.h + .cpp each)
  commands/             Things typed into the command bar
    Command.h           Base class every command derives from
    CommandManager.*    Owns the commands, parses the line, also defines `help`
    UpCommand, MoveCommands (vclip / hclip / tp), DupeCommand, EnchantCommand
  sdk/                  Everything that knows the game's memory layout
    Offsets.h           ALL offsets, vtable indices and signatures
    Actor.h             Thin views over game objects: Actor, Container, ItemStack, Vec3, AABB, ...
    PlayerTick.*        normalTick hooks: our way onto the client and server game threads
    PlayerItems.*       Inventory / selected slot / off hand access
    ActorList.*         All entities in the level
    BlockType.*, BlockRegistry.*   Block data and the registry that holds it (used by Xray)
    Enchant.*           EnchantUtils::applyEnchant and the enchantment table
    CommandSender.*     Sends "/commands" to a server through CommandRequestPacket
```

Build output goes to `build/` and dumps go to `dump/`. Both are ignored by git.

