---
tags: [spec, setup, portability]
---

# 15 · Setup and Portability

> [!goal] Specs 1 + 6
> Clone on any Windows PC that meets the requirements → double-click `run.bat` → it runs. **No config edits, no path changes, no manual steps.** Anything beyond clicking `run.bat` is avoided at all costs.

## Requirements (the only things installed manually, documented in README)
| Requirement | Why |
|---|---|
| Windows 10/11 x64 | Platform |
| **Visual Studio 2022 or newer, or Build Tools** with **"Desktop development with C++"** | Compiler + CMake + Ninja (bundled) |
| **Git** | Clone |
| **NVIDIA RTX GPU** (20-series+) + recent driver | Hardware RT (the Vulkan loader ships with the driver) |
| Internet on first run | Dependencies + assets |

**Explicitly NOT required:** Vulkan SDK, standalone CMake, Python, environment variables, admin rights.

## What `run.bat` does
```
[1/6] Checking requirements ...... finds VS via vswhere, checks GPU/driver
[2/6] Bootstrapping tools ........ Slang + validation layers → .tools/ (pinned, checksummed)
[3/6] Configuring ................ CMake + Ninja via the VS developer environment
[4/6] Building ................... incremental
[5/6] Checking assets ............ downloads missing assets from manifest (shows size)
[6/6] Launching .................. Build/<config>/GhostLight.exe
```
- **Clear, actionable errors** at every step, e.g. *"Visual Studio C++ tools not found. Install 'Desktop development with C++' (see README → Requirements)."*
- Full output logged to `Build/run.log`.
- **Installs nothing system-wide, never asks for admin.**
- Environment changes (VS dev environment, removing any `VULKAN_SDK`/`VK_*` variables) apply **only to that process**.
- **Validation layers:** Khronos publishes no Windows binaries, so `Scripts/BuildValidationLayers.ps1` builds a pinned tag once and the zip is mirrored on GHOST-LIGHT's GitHub Releases; `run.bat` fetches it into `.tools/`. The **engine itself** points the Vulkan loader at `.tools/` (always, so an installed SDK can't stand in) and disables known overlay layers (Steam, Epic, OBS, ...), so it behaves the same when launched from VS, RenderDoc or Nsight.
- **Downloaded dependency sources** live in `.tools/deps/`, so `run.bat clean` (which wipes `Build/`) never needs the internet.

| Command | Action |
|---|---|
| `run.bat` | Debug build + launch |
| `run.bat release` | Release build + launch |
| `run.bat test` | Build + smoke test (exit code for CI) |
| `run.bat clean` | Wipe `Build/` and rebuild |
| `new_technique.bat <Category> <Name>` | Scaffold a technique from `_Template/` |

## Path rules
1. **No absolute paths anywhere**: code, CMake, JSON, scripts.
2. The engine finds the **repo root** from the exe location (marker file at the root). All paths resolve from it.
3. Scene/prefab JSON paths are relative to `Content/`.
4. Must work with **spaces in the path**, any drive letter, and short-ish paths (keep `Build/` nesting shallow for the 260-character limit).

## Machine-specific state
- `User/settings.json`: window size, last scene, UI layout, camera speed. **Gitignored**, auto-created with defaults.
- Nothing machine-specific ever goes into committed files.

## `.gitignore`
`Build/` · `.tools/` · `User/` · `Captures/` · `Content/Assets/*` (except `CREDITS.md`) · shader caches

## Reproducibility
- Every dependency and tool is **pinned to a version** (and SHA-256 for downloads).
- After the first run, everything works **offline**.

## Portability acceptance test (must pass at the end of M0a)
1. Fresh `git clone` into `C:\Users\<name>\Desktop\My Projects\Ghost Light\`
2. Machine has VS 2022 + Git + RTX driver only (**no Vulkan SDK**)
3. Double-click `run.bat`
4. ✅ Engine launches into the default scene, zero validation errors, no edits made

See also: [[09 Assets]], [[14 Project Structure]]
