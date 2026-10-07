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

