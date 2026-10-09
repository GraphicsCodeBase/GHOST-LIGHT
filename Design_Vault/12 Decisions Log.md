---
tags: [decisions]
---

# 12 · Decisions Log

> [!note] Rule for Claude
> Decisions here are **settled**. Don't re-open them unless I ask. Add new ones at the bottom with date + reason.

| Date | Decision | Why |
|---|---|---|
| 2026-10-09 | Build a **custom engine** | Targeting graphics programmer roles |
| 2026-10-09 | Focus: **Ray Tracing Gems I & II** | Free, industry-authored syllabus |
| 2026-10-09 | **C++20** (not Rust) | Studio standard, interview language, learned at uni |
| 2026-10-09 | **Vulkan 1.3 + KHR ray tracing** | Cross-vendor, resume value |
| 2026-10-09 | **Slang** shaders | Modern, RT-friendly, HLSL-like |
| 2026-10-09 | **Hybrid renderer**, path tracer as reference | How shipped games work |
| 2026-10-09 | **Claude builds infrastructure incl. AS/SBT; I build techniques** | My time goes to math/algorithms |
| 2026-10-09 | **Tutor mode** by default for technique help | Techniques must be genuinely mine |
| 2026-10-09 | **RTX 2070 Super, 1080p**, half-res effects | My hardware, realistic constraints |
| 2026-10-10 | Spec lives in this vault + root `CLAUDE.md` | Avoid misunderstandings across sessions |
| 2026-10-10 | **One-click `run.bat`** with `release`/`test`/`clean` | Spec 1 |
| 2026-10-10 | **Reliability guarantees**: shader error overlay, GPU hang reports, smoke test, technique generator | Spec 2: engine never gets in my way |
| 2026-10-10 | **JSON** scenes and prefabs | Spec 3: simple, diffable, read + write |
| 2026-10-10 | **Garry's Mod-style sandbox** with **Jolt Physics** | Spec 4: interactive demos stress dynamic RT |
| 2026-10-10 | **EnTT** ECS | Spec 5: mature library, keeps ECS out of my workload |
| 2026-10-10 | **Player toggle (V)**: noclip fly ↔ walking character | Freedom to explore or play |
| 2026-10-10 | **Portable**: no absolute paths, machine state only in gitignored `User/` | Spec 6 |
| 2026-10-10 | **Prerequisites**: VS 2022 C++ workload + Git + RTX GPU, documented in README. **No Vulkan SDK**; run.bat installs nothing system-wide | Spec 6: a compiler is an acceptable prerequisite |
| 2026-10-10 | RTX GPU is a **hard requirement**; no raster-only fallback | Every target machine has RTX |
| 2026-10-10 | **Asset hosting**: free assets direct from source (pinned + SHA-256); gated assets mirrored on my GitHub Releases if license allows, otherwise replaced; nothing manual | Spec 6 + licensing |
| 2026-10-10 | **Module structure**: PascalCase folders, module = CMake target, one-way dependencies, README per module | Spec 7 |
| 2026-10-10 | My code folder is **`Techniques/`**, grouped by RTG category | Matches the book and catalog |
| 2026-10-10 | Techniques **never touch ECS/physics**, only the GPU scene | Keeps technique code clean and RTG-like |
| 2026-10-10 | M0 split into **M0a** (core + ECS + setup) and **M0b** (sandbox + tools) | Keeps each milestone verifiable |
| 2026-10-10 | Sharing prebuilt demo builds: **deferred** | Revisit near M7 |
| 2026-10-10 | Engine name: **GHOST LIGHT** (exe `GhostLight.exe`, C++ namespace `ghost::`) | Kojima-style; the lamp left burning on an empty stage, like a sandbox waiting for objects |
| 2026-10-10 | **Claude commits freely** on milestone branches, using the **feature-log commit format** (what was built / what it does / how to verify / roadmap item). Push only on request | Solo project; `git log` doubles as a changelog |
| 2026-10-10 | **Repo root = `D:\Raytracing_Engine`**, the working tree of `GraphicsCodeBase/GHOST-LIGHT`. GitHub's initial commit adopted (no force-push) | Matches the spec layout and `CLAUDE.md` |
| 2026-10-10 | Repo is **public from day one** | `run.bat` must download Release files (validation layers, mirrored assets) without a login |
| 2026-10-10 | **Validation layers**: built once from a pinned Vulkan-ValidationLayers tag by `Scripts/BuildValidationLayers.ps1`, zipped (with licenses) onto **GHOST-LIGHT GitHub Releases**, fetched by `run.bat` (pinned + SHA-256) | Khronos ships no Windows binaries; the LunarG SDK installer needs license acceptance; building on every first run costs 10–20 min + Python |
| 2026-10-10 | The **engine itself** points the Vulkan loader at the `.tools/` layers and disables implicit layers (Steam/Epic/OBS overlays) | Same behavior from `run.bat`, VS, RenderDoc or Nsight; no third-party layer noise in validation. OBS *game* capture won't hook (window capture works) |
| 2026-10-10 | Accept **VS 2022 or newer** (vswhere picks the newest with the C++ workload) | New machines may only have VS 2026 |
| 2026-10-10 | Downloaded dependency sources live in **`.tools/deps/`**, not `Build/` | `run.bat clean` must not re-download; offline after first run |
| 2026-10-10 | **Graphics never links EnTT** (CMake-enforced). A World system pushes plain data into `GpuScene`. **Assets is CPU-only** | Keeps techniques physically unable to reach the ECS ([[05 Architecture#Key boundary]]) |
| 2026-10-10 | **Linear render graph**: passes run in declared order (sorted by stage); barriers automatic; no pass reordering or memory aliasing | Solo-project scope; easy to read and explain |
| 2026-10-10 | Prefabs: **M0a** = scene entities may reference a prefab + transform; **M0b** = overrides, physics prefabs, starter set | Removes the M0a/M0b overlap |
| 2026-10-10 | Blue noise textures come through the **asset manifest**; the lookup code lives in `ShaderLibrary/Random` | "Commit source only" |
| 2026-10-10 | Module folders are created **when they get code** (Physics, Sandbox in M0b) | No empty stub modules |
| 2026-10-10 | Root also holds **`.gitattributes`** (CRLF for `.bat`/`.ps1`) | `run.bat` must work whatever the cloner's git line-ending settings |
| 2026-10-10 | Deferred from M0a: BC7 compression (Sponza fits as RGBA8), Tracy, emissive triangle list (M4) | Not needed yet |
| 2026-10-10 | Naive reference path tracer = cosine diffuse, epsilon offset, hard-shadow sun ray, mip 0 textures, no MIS / Russian roulette, each marked `RTG-TODO` | Correct but naive baseline the user upgrades in M2 |
| 2026-10-10 | Engine example technique lives in `Techniques/Examples/RayQueryNormals/` | Shows the technique pattern without being an RTG technique |
| 2026-10-10 | Dependencies come from **`Scripts/Dependencies.json`** (version, URL, SHA-256, size, license), downloaded by `run.bat`'s PowerShell bootstrap into `.tools/`; CMake only builds them (**no FetchContent**) | One downloader for tools, code and assets; sizes shown up front; checksums for every file; CMake never touches the network, so offline builds are guaranteed |
| 2026-10-10 | Minimum **VS 2022 17.5** (bundles CMake 3.25) | Needed for `/Z7` via `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT` and `add_subdirectory(... SYSTEM)` |
| 2026-10-10 | The build **enforces module dependencies**: a quoted `#include` of a module not listed in `DEPENDS` (or another module's `Internal/`) fails the CMake configure step | Rules 1–2 of [[14 Project Structure]] can't silently erode |
| 2026-10-10 | Implicit layers: only a **denylist of overlays** is disabled (`*steam*,*eos*,*obs*,*rtss*,*bandicam*,*overlay*,*fossilize*`), not all implicit layers | RenderDoc and Nsight inject as implicit layers and must keep working (refines the earlier "disable implicit layers" entry) |
| 2026-10-10 | `VK_LAYER_PATH` is **always** set to `.tools/validation-layers`, even when missing | Found on the dev machine: without it, the loader silently picked up the 2022 SDK layer from the registry |
| 2026-10-10 | Debug validation includes **synchronization validation** | Catches missing/wrong barriers, exactly what the render graph must get right |
| 2026-10-10 | New module **`Graphics/Renderer`**: per-frame orchestration, the only graphics class App uses | Keeps App free of Vulkan details; not in the original layout |
| 2026-10-10 | Swapchain is **`B8G8R8A8_UNORM`**; tonemap writes display-encoded values. **vsync** is a user setting (default on) | ImGui colors are authored in sRGB; uncapped mode exists for timing work |
| 2026-10-10 | Frames in flight = **2**, synchronized by **one timeline semaphore** (no fences) | Simpler and explainable; matches Vulkan 1.3 practice |
| 2026-10-10 | Validation layer zip: Release, **static CRT**, mimalloc with `MI_OVERRIDE=OFF` (VVL defines its own new/delete) | Runs on any machine without extra runtimes; avoids duplicate-symbol link error |
| 2026-10-10 | Smoke test **fails in Debug if validation isn't active** and resizes the window twice | "Zero validation errors" is meaningless if validation didn't run; resize exercises swapchain recreation |

See also: [[13 Open Questions]]
