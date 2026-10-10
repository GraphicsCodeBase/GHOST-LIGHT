# GHOST LIGHT

*A ray tracing engine. In theatre, the ghost light is the single lamp left burning on an empty stage.*

A C++20 / Vulkan real-time ray tracing sandbox for implementing techniques from
*Ray Tracing Gems* I & II, with an ECS, Jolt physics, and a Garry's Mod-style
sandbox to spawn and interact with objects.

> **Status:** building milestone M0a (renderer core). `run.bat` builds and starts the engine: Sponza in a raster
> G-buffer with placeholder lighting, and a naive reference path tracer on **F5**.
> The specification lives in [`Design_Vault/`](Design_Vault/) (open the repo folder as an Obsidian vault).

## Requirements (install once per machine)

| Requirement | Notes |
|---|---|
| Windows 10/11 x64 | |
| **Visual Studio 2022 (17.5 or later) or newer** (Community is fine) or the matching **Build Tools** with the **"Desktop development with C++"** workload | Provides the compiler, CMake, and Ninja |
| **Git** | To clone the repository |
| **NVIDIA RTX GPU** (RTX 20-series or newer) with a recent driver | Hardware ray tracing via Vulkan |
| Internet connection on first run | Dependencies and assets are downloaded once; works offline afterwards |

**Not required:** Vulkan SDK, a separate CMake install, environment variables, or any
path configuration. `run.bat` takes care of everything else.

## Quick start

```bat
git clone https://github.com/GraphicsCodeBase/GHOST-LIGHT.git
cd GHOST-LIGHT
run.bat
```

The first run downloads tools and assets and builds the engine (a few minutes).
Later runs only rebuild what changed and start in seconds.

| Command | What it does |
|---|---|
| `run.bat` | Build (Debug, incremental) and launch |
| `run.bat release` | Build Release and launch |
| `run.bat test` | Build and run the smoke test |
| `run.bat clean` | Delete build output and rebuild |
| `run.bat --scene Scenes/CornellBox.scene.json` | Anything after the mode is passed to the engine (`--scene`, `--frames`, `--no-validation`) |
| `new_technique.bat <Category> <Name>` | Create a new technique from the template |

## Controls

| Input | Action |
|---|---|
| Hold **right mouse** + move | Look around |
| **W A S D**, **Space** / **C** | Fly; up / down |
| **Shift** / **Ctrl** | Faster / slower; **mouse wheel** sets the base speed |
| **F2** | Techniques panel: enable techniques, tune their parameters |
| **F3** | Texture viewer: show any G-buffer target or technique output on screen |
| **F5** | Raster ↔ reference path tracer (accumulates while the camera stands still) |
| **F11** / **F12** | Fullscreen / screenshot to `Captures/` |

## Repository layout

| Folder | Contents |
|---|---|
| `Engine/` | Engine modules (Core, Platform, Graphics, World, Physics, Assets, Sandbox, …) |
| `Techniques/` | Ray tracing techniques, grouped by category, each with its own write-up |
| `ShaderLibrary/` | Shared Slang shader modules |
| `Content/` | Scenes and prefabs (JSON), downloaded assets |
| `Scripts/` | Helpers used by `run.bat` |
| `Tests/` | Smoke tests |
| `Design_Vault/` | Design spec and study notes (Obsidian vault) |

## Credits

Third-party assets and their licenses are listed in `Content/Assets/CREDITS.md`.
