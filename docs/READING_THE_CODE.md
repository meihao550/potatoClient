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
- **`injector.exe`** (`injector/injector.cpp`, built next to the DLL). It makes the game load the DLL
  with the classic `CreateRemoteThread(LoadLibraryW)` technique. `injector/injector.py` does the
  same in Python with a small GUI.

There is no game API and no mod loader. Everything the DLL knows about the game (struct
offsets, virtual function slots, byte signatures) was found by reverse engineering and lives in
one file, `client/src/sdk/Offsets.h`.

## 2. Repository map

```
CMakeLists.txt          The whole build: fetches MinHook, Dear ImGui and nlohmann/json, builds client.dll + injector.exe
Dockerfile              Builds client.dll in a Windows container (build only, no injection)
README.md / .en.md      Usage and a "how it works" overview (Japanese / English)
CONTRIBUTING.md         How to build, test and send changes
docs/
  ENCHANT.md            Manual for the `enchant` command (Japanese)
  READING_THE_CODE.md   This file
  REFACTORING.md        What the 2026 refactoring changed and why (start here when reviewing it)
injector/injector.cpp   Console injector (copies the DLL, then LoadLibraryW) -> injector.exe
injector/injector.py    The same steps in Python (tkinter + ctypes), optional
tools/dump_image.py     Dumps the decrypted game exe from memory for Ghidra / IDA
client/src/
  dllmain.cpp           Entry point: start-up order and unload sequence
  core/                 Building blocks with no game knowledge
    Hooks.*             MinHook wrapper: Hooks::create(name, target, &detour, original)
    InFlight.h          Counts threads inside our detours, so unloading can wait for them
    InputFocus.h        Whether the menu / command bar own the keyboard and mouse
    Logger.*            Console window + client.log, the LOG(...) macro
    Memory.*            Module base, findSig / scanOrLog (pattern scan), rva, safeRead, resolveRel32, callVirtual<>
    Util.h              toLower, kDegToRad
  render/               Getting a frame to draw on
    Renderer.*          Finds the DXGI / D3D12 vtables, hooks Present / ResizeBuffers / ExecuteCommandLists
    Backends.h          Interface of the two ImGui draw backends
    Overlay.*           The API-independent half: ImGui context, building each frame's UI
    Dx12Backend.cpp     ImGui on D3D12 (what the game normally uses)
    Dx11Backend.cpp     ImGui on D3D11 (fallback)
  gui/                  What the user sees and presses
    Menu.*              Module menu (Insert), command bar (Home), message toast
    Input.*             WndProc subclass (hotkeys), GetRawInputData hook (camera lock)
    GameInputHook.cpp   GameInput v3 hooks: hides mouse/keys from the game while the menu is open
  modules/              Toggleable features
    Module.h            Base class every feature derives from (+ Category)
    Setting.*           Typed settings (bool / float / color) the menu draws and the config saves
    Config.*            Saves / loads %LOCALAPPDATA%\PotatoClient\config.json
    ModuleManager.*     Owns the modules, dispatches ticks (by priority) / renders / key presses
    MoveInput.*         Helper: WASD / Space / Shift -> a direction for movement modules
    Xray, Fly, Speed, Aimbox, InventoryView, AutoTotem   (.h + .cpp each)
  commands/             Things typed into the command bar
    Command.h           Base class every command derives from
    CommandManager.*    Owns the commands, parses the line, also defines `help`
    Args.h              Strict number parsing for arguments
    Require.*           Shared checks with a message: in a world, own world, holding an item, ...
    UpCommand, MoveCommands (vclip / hclip / tp), DupeCommand, EnchantCommand
  sdk/                  Everything that knows the game's memory layout
    Offsets.h           ALL offsets, vtable indices and signatures
    GameObject.h        Base of the views below: at<T>(offset)
    Actor.h             Thin views over game objects: Actor (+ refs()), Container, ItemStack, Vec3, AABB, ...
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
4. **`modules/Module.h`**, **`modules/Setting.h`** and **`modules/ModuleManager.cpp`**. This is the whole plugin model: a base class with
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
  - `PlayerTick::runOnServerThenClient(serverFn, clientFn)` changes the server's copy first, then
    the client's copy the same way (dupe and enchant use it).
- **Data shared between onTick and onRender needs a lock.** `Aimbox` and `InventoryView` take a
  copy in `onTick` under a `std::mutex` member and draw from that copy in `onRender`.
- **Flags and settings read on several threads are atomic.** `Setting` values, `Module::isEnabled`,
  and `InputFocus` (is the menu open?) can be read from any thread without a lock.
- **Every detour starts with `InFlight::Guard guard;`.** Unloading waits until no thread is inside
  one of our hooks before the DLL is freed (`core/InFlight.h`). Add it to any new hook.
- **Write on the server when there is one.** In single-player, the built-in server owns the real
  state. If you only change the client's copy, the server overwrites it (this is why UP, dupe
  and AutoTotem all write on the server side).
- **Report results with `CommandManager::print()`.** It can be called from any thread.

## 5. Module and command lifecycle

### Modules (`modules/Module.h`)

```cpp
class Module {
    // category = menu tab; key = virtual-key code, 0 = unbound
    Module(std::string name, std::string description, Category category, int key);
    virtual bool onEnable();          // return false to refuse being enabled
    virtual void onDisable();
    virtual void renderSettings();    // the menu's "設定" node; default: every registered Setting
    virtual bool isAvailable() const; // false = a hook / signature it needs is missing: checkbox disabled
    virtual void onTick(Actor& player);  // every client tick while enabled (client game thread)
    virtual void onRender();             // every frame while enabled (render thread)
    virtual void saveExtra(nlohmann::json&);        // config state that isn't a Setting (Xray's lists)
    virtual void loadExtra(const nlohmann::json&);
protected:
    void addSettings({ &m_speed, ... });   // register Setting members (menu + config)
    void setTickPriority(int);             // higher = onTick runs later, so its writes win
};
```

- **Registration**: `ModuleManager::init()` (`modules/ModuleManager.cpp`) creates each module.
  The list order is the order inside each menu tab. The `onTick` order is a separate list sorted
  by `tickPriority()`: Fly has priority 1, so when Fly and Speed are both on, Fly's velocity wins.
- **Settings**: a module keeps `FloatSetting` / `BoolSetting` / `ColorSetting` members
  (`modules/Setting.h`) and registers them with `addSettings`. Each has a stable English id (the
  config key - don't rename it), a Japanese label, limits and an optional hint. The menu draws
  them and the config saves them; the module only reads them (`m_speed` converts to `float`).
- **Menu**: `Menu::render()` (`gui/Menu.cpp`) shows one tab per `Category` and, for each module in
  it, a checkbox, a key-bind button, the description and the Settings tree. A new module appears
  there automatically.
- **Ticks**: `ModuleManager::init()` registers `onClientTick` with
  `PlayerTick::setClientTickListener`, so every client tick calls `onTick` on each enabled module.
- **Keys**: `hookedWndProc` → `ModuleManager::onKey(vk)` → `toggle()` on every module bound to that key.
- **Config**: `ModuleManager::init()` ends with `Config::load()` (key binds, settings, and turns on
  what was on last time). `Config::save()` runs when the menu closes and on unload.
  The file is `%LOCALAPPDATA%\PotatoClient\config.json`; its exact path is logged.
- **Unload**: `ModuleManager::shutdown()` saves the config, then disables all modules. Put cleanup in
  `onDisable` (Xray uses it to restore the original block data).

### Commands (`commands/Command.h`)

```cpp
class Command {
    // name in lower case; usage = how to type it ("tp <x> <y> <z> ...")
    Command(std::string name, std::string description, std::string usage);
    virtual void execute(const std::vector<std::string>& args) = 0;   // args[0] = the name
};
```

- **Registration**: `CommandManager::init()` (`commands/CommandManager.cpp`).
- **Parsing**: `CommandManager::execute(line)` strips a leading `.` or spaces, splits on whitespace,
  lower-cases the first word and finds the matching command. `help` lists all of them, one per
  line; `help <name>` shows that command's usage.
- **Arguments**: parse numbers with `Args::parseInt` / `Args::parseFloat` (empty result = not a
  number) and answer bad input with `CommandManager::printUsage(*this)`.
- **Checks**: `Require::inWorld()`, `Require::ownWorld()`, `Require::playerRefs(player)` and
  `Require::heldItem(player, message)` print the reason themselves, so a command just returns
  when one fails.
- **Threading**: `execute` runs on the render thread, so use `PlayerTick::run*` for anything
  that touches the game, and `CommandManager::print` to report.

## 6. The SDK layer

The game has no symbols, so the SDK locates things in three ways. All of them are collected in
`sdk/Offsets.h`:

| Kind | Example | How it is used |
|---|---|---|
| **Field offsets** | `Offsets::Actor::stateVector = 0x218` | `Actor.h` turns them into accessors (`player.stateVector()`) |
| **Virtual function slots** | `Offsets::Actor::VIndex::teleportTo = 21` | `Memory::callVirtual<Ret, Args...>(obj, index, args...)` |
| **Byte signatures** | `Offsets::Sig::localPlayerVtable` | `Memory::scanOrLog(name, pattern)` scans the exe and logs the result (and warns if it matches more than once); `Memory::resolveRel32` follows the `lea`/`call` operand to the real address |

The views over game objects (`Actor`, `ItemStack`, `Block`, `BlockType`) derive from `GameObject`,
which provides `at<T>(offset)`. They have no data and no virtual functions, so `this` is exactly
the game's object. `Actor::refs()` returns the state, hitbox and rotation components together,
or nothing if one is missing - check it once instead of three null checks.

Guidelines that the existing code follows:

- **Call the game's own functions to change state** (`setItem`, `teleportTo`, `applyEnchant`, ...)
  rather than writing fields. Then the game's own rules and network sync still apply.
- **Spell out `callVirtual`'s template arguments**, for example `callVirtual<void, const Vec3&, bool>`,
  so that references are not silently turned into copies.
- **Read unknown pointers with `Memory::safeRead`** when they might be invalid. It uses SEH, so it
  returns `false` instead of crashing the game.
- **Never hard-code an address.** The exe is relocated (ASLR) and changes with every update. Use
  a signature, or follow pointers from an object you already have.
- **Fail safely when a signature is missing.** A feature must disable itself instead of crashing.
  Commands check an `available()` function first (`EnchantCommand` checks `Enchant::available()` and
  `CommandSender::available()`). A module that depends on a hook or signature overrides
  `isAvailable()` (the `onTick` modules return `PlayerTick::hooked(...)`). The menu then disables its
  checkbox and shows 「シグネチャ未検出のため無効」 ("disabled: signature not found").

## 7. Naming and style conventions

| Thing | Convention | Example |
|---|---|---|
| Classes, files | PascalCase | `AutoTotem`, `AutoTotem.cpp` |
| Namespaces | PascalCase | `PlayerItems`, `Offsets::Actor` |
| Functions, methods | camelCase | `findTotem`, `setOffhandSlot` |
| Member fields | `m_` prefix | `m_speed`, `m_enabled` |
| File-local globals | `g_` prefix, in an anonymous namespace | `g_modules`, `g_client` |
| Hook detours / originals | `hk` / `o` prefix | `hkPresent` / `oPresent` |
| Headers | `#pragma once` | |
| Formatting | 4 spaces, braces on the same line | |

Code comments are in English. User-facing strings (module names and descriptions, menu text,
command messages) are in Japanese. Keep that split.

## 8. Recipes

### Adding a module

1. Create `client/src/modules/MyModule.h`:
   ```cpp
   #pragma once
   #include "Module.h"
   #include "sdk/PlayerTick.h"

   // One line: what it does
   class MyModule : public Module {
   public:
       MyModule() : Module("MyModule", "説明 (shown in the menu)", Category::Movement, 0) {
           addSettings({ &m_value, &m_enabledThing });
       }
       void onTick(Actor& player) override;
       // onTick needs the player tick hook
       bool isAvailable() const override { return PlayerTick::hooked(PlayerTick::Side::Client); }

   private:
       // id (config key, never rename), label, default, min, max, slider format, hint
       FloatSetting m_value{ "value", "値", 1.0f, 0.1f, 5.0f, "%.2f", "ヒント (任意)" };
       BoolSetting m_enabledThing{ "thing", "何かを有効にする", true };
   };
   ```
2. Create `client/src/modules/MyModule.cpp`. Implement `onTick` (game thread) and/or
   `onRender` (render thread). `Speed.h` / `Speed.cpp` is a good template. The menu and the
   config file pick up the registered settings by themselves.
3. Add `#include "MyModule.h"` and `g_modules.push_back(std::make_unique<MyModule>());` to
   `ModuleManager::init()`.
4. Rebuild. CMake picks up new `.cpp` files automatically (`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`).

### Adding a command

1. Declare a class deriving from `Command` with a name, a description and a usage string
   (see `commands/MoveCommands.h`).
2. In `execute`, validate `args`, then schedule the actual work:
   ```cpp
   void MyCommand::execute(const std::vector<std::string>& args) {
       const auto amount = args.size() > 1 ? Args::parseFloat(args[1]) : std::nullopt;
       if (!amount) { CommandManager::printUsage(*this); return; }
       if (!Require::inWorld()) return;

       PlayerTick::runOnSelf([value = *amount](Actor& player, bool server) {
           // game thread: safe to touch `player` here
           const auto refs = Require::playerRefs(player);
           if (!refs) return;
           CommandManager::print("...");
       });
   }
   ```
3. Register it in `CommandManager::init()`.

### Using a new game function

1. Find it in the dumped exe (see the next section) and add its signature or vtable index to
   `sdk/Offsets.h`, with a comment that says how it was found.
2. Wrap it in `sdk/` (an accessor in `Actor.h`, or a new `sdk/Xxx.{h,cpp}` with an `init()` that
   resolves the signature and an `available()` check).
3. If it needs `init()`, call it from `dllmain.cpp` before `CommandManager::init()`.

## 9. When the game updates

After a Minecraft update, offsets and signatures usually break. Signatures that are not found are
logged to `client.log` (for example `LocalPlayer vtable reference: signature not found`), so start there.

1. Enter a world and run `python tools\dump_image.py dump\Minecraft.Windows.dump.exe` to dump the
   decrypted exe.
2. Open the dump in Ghidra or IDA. Use the comments in `Offsets.h` (they describe the instructions
   around each signature and where each offset comes from) to find the new values.
3. Cross-check struct layouts against the [LeviLamina](https://github.com/LiteLDev/LeviLamina) BDS
   headers, then confirm with live memory reads before changing `Offsets.h`.

The full procedure is in [README.en.md](../README.en.md#when-the-game-updates-reverse-engineering-procedure).

## 10. Further reading

- [README.en.md](../README.en.md): usage, plus "How it works" for every feature
- [README.md](../README.md): the same in Japanese (the original)
- [docs/ENCHANT.md](ENCHANT.md): the enchant command manual (Japanese only)
- [CONTRIBUTING.md](../CONTRIBUTING.md): building, testing and sending changes
