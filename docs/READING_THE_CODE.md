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

## 3. Suggested reading order

Read top-down: first how the DLL gets control, then the framework, then one feature,
then the game-memory layer.

1. **`client/src/dllmain.cpp`** (about 50 lines). `DllMain` only starts a thread, and `mainThread`
   shows the complete start-up order and the unload sequence. Every subsystem you meet later is
   initialized here.
2. **`core/`**. `Hooks.h` (one function you will see everywhere), `Logger.h` (`LOG`), and
   `Memory.h`. In `Memory.h`, understand `findSig`, `resolveRel32` and `callVirtual` before going
   further; the SDK is built on them.
3. **`render/Renderer.cpp`**, then **`gui/Input.cpp`** and **`gui/Menu.cpp`**. This is how a frame
   gets drawn (`hkPresent` → backend → `Menu::render`) and how keys reach the client
   (`hookedWndProc`). You can skim the D3D backends.
4. **`modules/Module.h`** and **`modules/ModuleManager.cpp`**. This is the whole plugin model: a base class with
   virtual hooks and a list that is built in `ModuleManager::init()`.
5. **One small module**: `modules/Speed.cpp` (about 20 lines), then `modules/Fly.cpp`. They show the
   typical pattern of reading a component of the player and overwriting it every tick.
6. **`sdk/Offsets.h`**, then **`sdk/Actor.h`**. These show how raw offsets become typed accessors like
   `player.stateVector()`.
7. **`sdk/PlayerTick.h` / `.cpp`**. This is the most important piece of infrastructure: how code gets to run
   on the game's own threads (see the next section).
8. **`sdk/PlayerItems.cpp`**, then **`modules/AutoTotem.cpp`**. This is a complete feature that reads on the
   client and writes on the server.
9. Then anything you are curious about: `modules/Xray.cpp` + `sdk/BlockRegistry.cpp` (data
   patching instead of hooking), `sdk/CommandSender.cpp` (building a network packet),
   `modules/Aimbox.cpp` (world-to-screen projection).

Many `.cpp` files start with a block comment that explains the technique (for example
`Fly.cpp`, `AutoTotem.cpp`, `PlayerItems.cpp`, `CommandSender.cpp`). Read those first.
The "How it works" section of [README.en.md](../README.en.md#how-it-works) explains how each
address and offset was found.

## 4. Threading model

This is the rule that matters most when changing code. Game objects may only be touched on
the thread that owns them. The client code runs on five different threads:

| Thread | Entered through | What runs there |
|---|---|---|
| **Init thread** | `CreateThread` in `DllMain` | Start-up, the retry loop for `Input::hookGameInput()`, unload |
| **Render thread** | `hkPresent` (`render/Renderer.cpp`) | ImGui: `Menu::render`, `Module::onRender`, `Module::renderSettings`, and `Command::execute` (the command bar is ImGui) |
| **Window thread** | `hookedWndProc` (`gui/Input.cpp`) | Hotkeys: Insert / Home / End, and `ModuleManager::onKey` (toggling a module by its key) |
| **Client game thread** | `LocalPlayer::normalTick` hook (`sdk/PlayerTick.cpp`) | `Module::onTick(Actor&)` for every enabled module, and client-side tasks |
| **Server game thread** | `ServerPlayer::normalTick` hook | Server-side tasks. It only exists when the world runs on this PC (single-player or hosting) |

The rules that follow from this:

- **Never touch game objects from the render thread.** Commands run on the render thread, so they
  schedule work instead:
  - `PlayerTick::run(Side::Client or Side::Server, fn)` runs `fn` during the next tick of a player on that side.
    `fn` returns `false` for "not this player", and is dropped after a couple of seconds.
  - `PlayerTick::runOnOwnServerPlayer(fn)` runs `fn` on the server thread with *our* ServerPlayer.
  - `PlayerTick::runOnSelf(fn)` runs `fn` on whichever player owns our real position: the ServerPlayer
    when the server is local, otherwise the LocalPlayer.
- **Data shared between onTick and onRender needs a lock.** `Aimbox` and `InventoryView` take a
  copy in `onTick` under a `std::mutex` member and draw from that copy in `onRender`.
- **Write on the server when there is one.** In single-player, the built-in server owns the real
  state. If you only change the client's copy, the server overwrites it (this is why UP, dupe
  and AutoTotem all write on the server side).
- **Report results with `CommandManager::print()`.** It can be called from any thread.

## 5. Module and command lifecycle

### Modules (`modules/Module.h`)

```cpp
class Module {
    Module(std::string name, std::string description, int key);   // key = virtual-key code, 0 = unbound
    virtual bool onEnable();          // return false to refuse being enabled
    virtual void onDisable();
    virtual void renderSettings();    // ImGui widgets inside the menu's "設定" (Settings) node
    virtual bool isAvailable() const; // false = a signature was not found, so the checkbox is disabled
    virtual void onTick(Actor& player);  // every client tick while enabled (client game thread)
    virtual void onRender();             // every frame while enabled (render thread)
};
```

- **Registration**: `ModuleManager::init()` (`modules/ModuleManager.cpp:22-32`) pushes each module
  into a list. The order matters: later modules' `onTick` runs after earlier ones, which is
  why Fly comes after Speed (Fly wins when both are on).
- **Menu**: `Menu::render()` (`gui/Menu.cpp`) loops over `ModuleManager::modules()` and draws a
  checkbox, a key-bind button, the description and the Settings tree for each. A new module
  appears there automatically.
- **Ticks**: `ModuleManager::init()` registers `onClientTick` with
  `PlayerTick::setClientTickListener`, so every client tick calls `onTick` on each enabled module.
- **Keys**: `hookedWndProc` → `ModuleManager::onKey(vk)` → `toggle()` on every module bound to that key.
- **Unload**: `ModuleManager::shutdown()` disables all modules. Put cleanup in `onDisable` (Xray
  uses it to restore the original block data).

There is no config file. Settings are plain member fields and reset when the DLL is reloaded.

