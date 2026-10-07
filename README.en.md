# PotatoClient — a Minecraft Bedrock client

[![build](https://github.com/meihao550/potatoClient/actions/workflows/build.yml/badge.svg)](https://github.com/meihao550/potatoClient/actions/workflows/build.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

[日本語](README.md) | English

A Horion-style internal (DLL) client built from scratch to learn how cheat clients work.
Target: **Minecraft Bedrock 1.26.52 (GDK build / `Minecraft.Windows.exe`)**

> Use it only in single-player or in your own worlds. Using it on public servers or Realms breaks their rules and can get you banned.
> Some features also work in multiplayer, but only use them on servers that allow it.

New to the code? Start with **[docs/READING_THE_CODE.md](docs/READING_THE_CODE.md)**. Want to contribute? See **[CONTRIBUTING.md](CONTRIBUTING.md)**.

## How to use

### 1. Requirements
Needed whichever way you build:
- Windows 10 / 11 (x64)
- Minecraft Bedrock **1.26.52** (the GDK build, installed from the Microsoft Store or the Xbox app)

There are three ways to get the client. **Pick the one that fits your setup.** All of them produce the same `client.dll` and `injector.exe`.

| Way | What else you need | Good for |
|---|---|---|
| **A. Visual Studio** (recommended) | Visual Studio 2019 / 2022 / 2026 (Community or Build Tools) with "Desktop development with C++" | Reading or changing the code |
| **B. Docker** | Docker Desktop in Windows-containers mode | Not installing Visual Studio |
| **C. No build** | A GitHub account | Just trying it out |

Python is not required. (Install Python 3.10+ 64-bit only if you want the older `injector\injector.py`.)

### 2. Build

#### A. Build with Visual Studio
1. If you don't have Visual Studio yet, install **Community** or **Build Tools** from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/). In the installer, tick **"Desktop development with C++"** (it includes CMake).
2. Open **"Developer PowerShell for VS"** (or "Developer Command Prompt for VS") from the Start menu. A plain PowerShell can't find `cmake`.
3. Go to the repository folder (the one that contains `CMakeLists.txt`) and run these two lines:
   ```
   cmake -S . -B build -A x64
   cmake --build build --config Release
   ```
4. It worked if `client.dll` and `injector.exe` are in `build\Release\` (`client.pdb` is for crash analysis).

- After the first build, only the second line (`--build`) needs to be run again when you change the source
- The newest installed Visual Studio is used automatically. If you switch Visual Studio versions, delete the `build` folder and start again from the first line

#### B. Build with Docker
Switch Docker Desktop to **Windows containers**, then run this in the repository folder. (The container only builds; injecting into the game can't be done from a container.)
```
docker build -t potatoclient-build .
docker create --name potatoclient-out potatoclient-build
docker cp potatoclient-out:C:\out .\out
docker rm potatoclient-out
```
`client.dll` and `injector.exe` end up in `out\`. On Windows 10, add `--build-arg WINDOWS_VERSION=ltsc2019` or build with `--isolation=hyperv`.

#### C. Use it without building
GitHub builds the project on every push.
1. Open the [Actions page](https://github.com/meihao550/potatoClient/actions/workflows/build.yml) and pick the newest run with a green check (✓)
2. Download `potatoclient` under **Artifacts** and extract it (you need to be signed in to GitHub)
3. It contains `client.dll` and `injector.exe`

#### If the build fails
| Symptom | Fix |
|---|---|
| `cmake` is not recognized | You are in a plain PowerShell. Use "Developer PowerShell for VS" |
| No "Developer PowerShell for VS" in the Start menu | Open the Visual Studio Installer, click "Modify" and add "Desktop development with C++" |
| `generator does not match` / `Visual Studio 16 2019 could not find` | A `build` folder made with other settings is still there. Delete `build` and start again from the first line |
| `git` not found (FetchContent fails during the build) | Run from the Developer PowerShell, or install [Git for Windows](https://git-scm.com/download/win) |

### 3. Inject
1. Start Minecraft and **enter a single-player world** (Xray's block list is only created once you are in a world).
2. Double-click `build\Release\injector.exe` (or run it from a terminal):
   ```
   build\Release\injector.exe
   ```
   - It injects the `client.dll` in the same folder. For another DLL: `injector.exe <path to dll>`
   - If you have Python, the older button-based `python injector\injector.py` works too
3. It worked if the injector prints 「Inject 成功!」 ("Inject succeeded!") and a console window opens next to the game printing `Injected!`.

> The in-game UI (menu, module descriptions, command messages) is in Japanese.

### 4. Controls
| Key | Action |
|---|---|
| Insert | Show / hide the menu (while it is open, camera, movement and clicks don't reach the game) |
| X (rebindable in the menu) | Xray on/off |
| F (rebindable in the menu) | Fly on/off |
| Home | Open the command bar (type `up`, press Enter to run, Esc to close) |
| End | Unload (the game keeps running and you can inject again) |

Reading the menu:
- **Checkbox**: turns the module on/off
- **「キー: X」 (Key: X) button**: click it, then press any key to rebind. Press Esc to leave it unbound
- **「設定」 (Settings)**: expands the module's own settings

### 5. Using Xray
1. Open the menu with Insert and tick Xray (or press X).
2. Open Settings and choose what to show:
   - **Ores to show**: toggle diamond, iron, gold and so on individually. There are also "all on" / "all off" buttons
   - **Extra blocks**: type part of a block name and click "add" (e.g. `amethyst` shows everything amethyst-related). `x` removes an entry
   - **Let light through**: when on, underground ores are not darkened
3. **Go to Settings → Video and toggle "Smooth Lighting" off and back on once**, so every chunk picks up the change.
   - Changing only the render distance rebuilds just the chunks that newly come into range, so only some of them become transparent
4. After turning Xray off, toggle Smooth Lighting the same way to get the normal look back.

### 6. The UP command (get to the surface)
1. In a world, press **Home**. A command bar appears in the top-left corner.
2. Type `up` and press **Enter** (`.up` and `UP` also work).
3. You are teleported onto the highest block directly above your X/Z, and the result is shown in the top-left for a few seconds.
   - Water surfaces and leaves count as "ground" (on the sea you land on the water, under a tree you land on top of it)
   - In the Nether the bedrock ceiling is the highest block, so you end up above the ceiling
   - `help` lists the commands, Esc closes the bar

### 6.5 Movement (also works on multiplayer servers)
In single-player, the built-in server's ServerPlayer is moved through the same path as `/tp`. **On a remote server, the LocalPlayer's position and velocity are written directly.** The client sends its own position to the server every tick, so this goes through on servers without movement checks.

| Command | Action |
|---|---|
| `up` | Onto the highest block directly above you (multiplayer OK) |
| `vclip 10` / `vclip -5` | Move n blocks straight up / down (through walls) |
| `hclip 5` | Move n blocks in the direction you are facing |
| `tp 100 64 -20` / `tp ~ ~20 ~` | Move to coordinates (feet position; `~` means relative) |

| Module | Action |
|---|---|
| Fly (F) | WASD to move, Space to go up, Shift to go down, no keys to hover. Speed and anti-kick in Settings |
| Speed | While WASD is held, your horizontal speed is set to the configured value |

- Keys are read directly with `GetAsyncKeyState`, so they are **fixed to WASD / Space / Shift** (the game's key bindings are ignored). They are ignored while the menu or command bar is open, and while the game is not in the foreground
- Turning Fly off makes you fall normally and take fall damage. Land first, then turn it off
- If the server has movement checks (anti-cheat), large teleports and very fast movement get pulled back. Lower the speed or move in shorter steps

`dupe` — raises the held item's count to the maximum stack size (64 / 16). `dupe 10` sets it to 10
- Items that don't stack, such as tools and armor, can't be duplicated
- Like UP, this is for your own world only

`enchant` — enchants the weapon or armor in your hand (`enchant sharpness` / `enchant all` / `enchant list`)
- **Also works on multiplayer servers (operator permission required)**: it sends `/enchant @s <name> <level>` to the server the same way chat does. Only enchantments that can be applied to the held item are sent, so `enchant all` doesn't flood you with errors. Levels go up to the maximum the server's `/enchant` allows
- Full manual: **[docs/ENCHANT.md](docs/ENCHANT.md)** (Japanese only)

### 6.6 Inventory
| Module | Action |
|---|---|
| Inventory | Shows the 36 inventory slots and the off hand in the top-right corner. The selected hotbar slot is yellow. While the menu is open you can move the window, and hovering a slot shows the item name and count (multiplayer OK) |
| AutoTotem | When the off hand has no Totem of Undying and the inventory has one, moves it to the off hand automatically. Whatever was in the off hand is moved to an empty slot (can be turned off in Settings). **Single-player only** |

### 7. Unloading and rebuilding
- Press **End** in game to unload. The console window closes (Xray is restored automatically if it was on)
- The injector loads a copy of the DLL (`client_loaded_<timestamp>.dll`), so **you can rebuild while it is injected**. Old copies are deleted automatically on the next inject
- To try a new build: unload with End, then inject again

### 8. Troubleshooting
| Symptom | Fix |
|---|---|
| "Minecraft.Windows.exe is not running" | Start Minecraft first |
| "LoadLibraryW failed" | Check the DLL path. Make sure it is a 64-bit Release build |
| Insert doesn't open the menu | Check that the console shows `renderer api = D3D12`. If not, move around in the world a bit so it renders |
| Xray doesn't stay ticked | Turn it on after entering a world (the console says the block list was not found) |
| Xray only affects some chunks | Toggle Smooth Lighting once |
| `up` says to use it after entering a world | Player ticks stop while the game is paused, so go back to the game first. Check that the log shows `hook LocalPlayer::normalTick` and `hook ServerPlayer::normalTick` with `MH_OK`. It doesn't work on servers (Realms, etc.) |
| The game crashed | Check the last line of `build\Release\client.log` and the error under Event Viewer → Windows Logs → Application (the faulting offset in `client.dll`). `client.pdb` maps that offset to a source line |
| It stopped working after a game update | The offsets have changed. See "When the game updates" below |

Log: `build\Release\client.log` (the console window shows the same output).

## How it works

### 1. Injection — `injector/injector.cpp` (Python version: `injector/injector.py`)
`OpenProcess` → `VirtualAllocEx` → `WriteProcessMemory` (the DLL path) → `CreateRemoteThread(LoadLibraryW)`.
This is the classic technique of making the game call `LoadLibraryW` on our DLL itself. The C++ and Python versions follow the same steps.
A timestamped copy of the DLL is injected each time, so you can rebuild while it is injected.

### 2. DLL entry — `client/src/dllmain.cpp`
`DllMain` runs under the loader lock, so it does nothing except start a thread that does the initialization.

### 3. Render hook (ImGui overlay) — `client/src/render/`
We create our own dummy D3D12 device and swap chain, read the addresses of
`IDXGISwapChain::Present` (8) / `ResizeBuffers` (13) / `ID3D12CommandQueue::ExecuteCommandLists` (10) from their COM vtables,
and hook them with MinHook. ImGui is drawn every time the game calls Present.

### 4. Input — `client/src/gui/Input.cpp`, `GameInputHook.cpp`
- The WndProc is subclassed to catch key presses (Insert / End / key binds)
- This GDK build reads mouse and keyboard through **GameInput v3**, so
  `IGameInputReading::GetMouseState` (14) / `GetKeyState` (13) are hooked. While the menu is open, the camera position is frozen and keys are reported as "not pressed"

### 5. Xray — `client/src/modules/Xray.cpp`, `client/src/sdk/`
Instead of hooking functions, this **rewrites the game's data**.
- Every kind of block has a `BlockType` object, stored in the `std::map` of the `BlockTypeRegistry`
- The registry is found without a signature: we scan the exe's `.data` section for a `std::map` whose names start with `minecraft:` and validate it (`BlockRegistry.cpp`)
- `minecraft:barrier` is already "invisible and doesn't hide its neighbours' faces", so every hidden block gets the same values as barrier
  - `BlockType`: `mRenderLayer=Barrier(14)`, `mIsOpaqueFullBlock=false`, `mTranslucency=1`, `mLightBlock=0`
  - Each `Block` (state): the cached `mIsOpaqueFullBlock`, `mLight` and occlusion shapes (`occlusionShapes`) are set to 0
- The original values are saved once and restored exactly when Xray is turned off

### 6. UP command — `client/src/commands/`, `client/src/sdk/PlayerTick.cpp`, `Actor.h`
The exe has almost no class names (RTTI) and no fixed pointer to the player, so **code is used as the landmark**.
- **LocalPlayer's vtable**: LocalPlayer's destructor writes its vtable with `lea rax,[vftable]` → `mov [rcx],rax`. This instruction sequence is the signature, and the vtable address comes from the `lea`'s rel32 (`Offsets::Sig`)
- **Getting onto the game thread**: vtable slot 24 = `normalTick` is hooked with MinHook. Every tick we get `this` (the current player) and run on the game's own thread. The command bar (render thread) schedules work with `PlayerTick::run()`
- **Move the server-side player**: even in single-player a server runs inside the same process, and the real position belongs to the server. Moving only the client's LocalPlayer gets you pulled back by the server (the first version failed this way). So `normalTick` of ServerPlayer (signature: the vtable assignment in its constructor) is hooked too, and the ServerPlayer is moved on the server thread just like `/tp`. The server then tells the client about the move
- **Finding the highest block**: `Actor+0x1C8` → Dimension, `+0xF0` → BlockSource (the "world" of that dimension). BlockSource's virtual `getAboveTopSolidBlock(x, z, water, leaves)` returns the Y one above the highest solid block
  - MSVC lays out overloaded virtual functions **in reverse declaration order**, so they are off by one from the header's order (`getBlock`, `getAboveTopSolidBlock`). This was confirmed by checking argument usage in the disassembly
- **Moving**: ServerPlayer's virtual function 21 `teleportTo(pos, ...)` (the same path as `/tp`). The position is at eye height (feet + 1.62), so the current "eye height − feet" is added before passing it
- Commands derive from `Command` and are registered in `CommandManager::init()` (the same pattern as modules)

### 6.5 Movement — `sdk/Actor.h` (`moveFeetTo`), `modules/Fly.cpp`, `Speed.cpp`, `commands/MoveCommands.cpp`
- `Actor+0x228` → ActorRotationComponent (pitch, yaw, previous pitch, previous yaw). yaw 0 = facing +Z, −90 = facing +X
- **Teleport**: `PlayerTick::runOnSelf` picks "whoever owns the position". If the built-in server is running, ServerPlayer's `teleportTo`; otherwise the LocalPlayer's `StateVector.pos/posPrev` and AABB are shifted by the same amount
- **Fly / Speed**: `StateVector.posDelta` (velocity, blocks/tick). Each tick the game "moves by posDelta → applies gravity and drag", so overwriting it **right after** the LocalPlayer's normalTick makes the next tick move by exactly that value. A module just implements `Module::onTick` to be called every tick

### 7. DUPE command — `client/src/commands/DupeCommand.cpp`
- The inventory contents are an array of `ItemStack` (0x98 bytes). The count is the 1-byte `mCount` (+0x22)
- Actor virtual function 77 `getCarriedItem()` returns "a reference to the ItemStack of the selected slot" (internally: PlayerInventory at `Actor+0x5B8` → selected slot index (+0x10) → container (+0xB8) `getItem(slot)`). It is a reference, so writing to it changes the real stack
- The maximum stack size comes from ItemStack → `WeakPtr<Item>` (+0x08) → Item's `mMaxStackSize` (+0xA8)
- **The same value is written on both server and client.** The server's copy is the real one (used for saving and placing), the client's is for display. The server doesn't resend slots it thinks are unchanged, so writing only one side makes the display go out of sync

### 8. ENCHANT command — `client/src/sdk/Enchant.cpp`, `commands/EnchantCommand.cpp`
- The string `"commands.enchant.success"` → `EnchantCommand::execute`, which uses it → `EnchantUtils::applyEnchant(ItemStackBase&, EnchantmentInstance const&, bool)`, called inside it. The start of that function is the signature
- NBT isn't built by hand; the game's function does the work, so the rules for what can be applied are the same as `/enchant`. The same thing is done to the held item on both server and client (same reason as dupe)
- **Remote servers**: `sdk/CommandSender.cpp`. The string `"CommandRequestPacket"` → `getName` → the vtable → the constructor that writes it and "the function that sends a chat command". Like that function, a packet is built from a payload (command string / origin = Player / version 0x34) and sent via the Level at `Actor+0x1D8` → virtual function 328 `getPacketSender` → virtual function 2 `send`. The server checks permissions, so operator permission is required
  - The packet takes over the command string's buffer, but that buffer was allocated by the DLL's CRT. So that the game doesn't free it, we take it back after sending and then destroy the packet
- Manual: [docs/ENCHANT.md](docs/ENCHANT.md) (Japanese only)

### 9. Inventory / AutoTotem — `client/src/sdk/PlayerItems.cpp`, `modules/InventoryView.cpp`, `modules/AutoTotem.cpp`
- **Inventory**: PlayerInventory at `Actor+0x5B8` → Inventory at `+0xB8` (a Container with 36 slots: 0..8 hotbar, 9..35 the rest). Same path as `getCarriedItem` (77). Container's virtual functions are `getItem` (7) / `setItem` (12) / `removeItem` (14) / `getContainerSize` (20)
- **Off hand**: not part of the inventory; it is slot 1 of the 2-slot "hand container" returned by `ActorEquipment::getHandContainer(EntityContext&)`. That function has no signature, so its call target is read at runtime from the `mov rsi,rcx / add rcx,8 / call ...` at the start of `Actor::getEquippedTotem` (79) (`Actor+8` is the EntityContext)
- **AutoTotem**: decides "no totem in the off hand & one in the inventory" from the client-side copy, then on the server thread calls `setItem` (move the off-hand item out of the way) → `setOffhandSlot` (78) → `removeItem` on the ServerPlayer. Since the game's own functions do the moving, no manual sync like dupe's is needed

## When the game updates (reverse-engineering procedure)
All offsets live in `client/src/sdk/Offsets.h`.
1. Start the game and enter a world
2. `python tools\dump_image.py dump\Minecraft.Windows.dump.exe` — restores the encrypted exe from memory into a form Ghidra/IDA can open (this step alone needs Python 3.10+)
3. Check the struct layouts in [LeviLamina](https://github.com/LiteLDev/LeviLamina)'s `src/mc/world/level/block/BlockType.h` and `Block.h` (they are for BDS, but almost identical to the client)
4. Read live memory to confirm that known values (names, etc.) are at those offsets, then update `Offsets.h`
5. For the UP command: check in the disassembly that the `LocalPlayer` / `ServerPlayer` vtable signatures still match exactly one location and that the vtable indices (`VIndex`) are still right
   - The player can be found by searching memory for the "AABBShapeComponent with a 0.6×1.8 hitbox" and following the Actor that points to it (+0x220)

## Layout
```
injector/injector.cpp         Inject (built as build\Release\injector.exe)
injector/injector.py          the same in Python (one-button GUI)
tools/dump_image.py           dumps the exe from the running game
client/src/
  dllmain.cpp                 init thread / unload
  core/                       logging, pattern scan, MinHook wrapper
  render/                     Present hook, ImGui drawing for D3D12 / D3D11
  gui/                        menu, WndProc, input hooks
  modules/                    Module base, ModuleManager, Xray, Fly, Speed, Aimbox, Inventory, AutoTotem
  commands/                   Command base, CommandManager, up / vclip / hclip / tp / dupe / enchant / help
  sdk/                        game structures (offsets, BlockType, Actor, registry lookup, player tick hooks)
```
A new feature derives from `Module` and is registered in `ModuleManager::init()` to show up in the menu.
A new command derives from `Command` and is registered in `CommandManager::init()` to be usable from the command bar.
A detailed walkthrough is in [docs/READING_THE_CODE.md](docs/READING_THE_CODE.md).

## License
[Apache License 2.0](LICENSE)
