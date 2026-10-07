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

#### Building with Docker (optional)
If you only want the DLL and don't want to install Visual Studio, you can use the bundled `Dockerfile`. Docker must be in **Windows containers** mode. The container only builds the DLL: injecting it into the game cannot be done from a container.
```
docker build -t potatoclient-build .
docker create --name potatoclient-out potatoclient-build
docker cp potatoclient-out:C:\out .\out
docker rm potatoclient-out
```
This produces `out\client.dll`. On Windows 10, add `--build-arg WINDOWS_VERSION=ltsc2019` or build with `--isolation=hyperv`.

### 3. Inject
1. Start Minecraft and **enter a single-player world** (Xray's block list is only created once you are in a world).
2. In another window, run:
   ```
   python injector\injector.py
   ```
3. Click the **Inject** button in the window that opens.
   - The DLL path defaults to `build\Release\client.dll`. Use the browse button (「参照...」) to pick a different DLL.
4. It worked if the injector shows 「Inject 成功!」 ("Inject succeeded!") and a console window opens next to the game printing `Injected!`.

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

### 1. Injection — `injector/injector.py`
`OpenProcess` → `VirtualAllocEx` → `WriteProcessMemory` (the DLL path) → `CreateRemoteThread(LoadLibraryW)`.
This is the classic technique of making the game call `LoadLibraryW` on our DLL itself.
A timestamped copy of the DLL is injected each time, so you can rebuild while it is injected.

### 2. DLL entry — `client/src/dllmain.cpp`
`DllMain` runs under the loader lock, so it does nothing except start a thread that does the initialization.

### 3. Render hook (ImGui overlay) — `client/src/render/`
We create our own dummy D3D12 device and swap chain, read the addresses of
`IDXGISwapChain::Present` (8) / `ResizeBuffers` (13) / `ID3D12CommandQueue::ExecuteCommandLists` (10) from their COM vtables,
and hook them with MinHook. ImGui is drawn every time the game calls Present.

