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
| 2026-10-10 | **One bindless descriptor set + one pipeline layout** (set 0, 256 B push constants) for every pipeline; buffers via buffer device address | No per-pass descriptor/reflection plumbing; techniques never manage descriptors |
| 2026-10-10 | Slang target: **SPIR-V 1.6, column-major matrices, scalar layout**, debug info in Debug; one fresh session per compile | Matches glm and C++ struct layout; edited files always re-read |
| 2026-10-10 | **Reverse-Z** depth (near = 1, compare GREATER) | Much better depth precision with a float depth buffer |
| 2026-10-10 | Hot reload polls shader files **4×/s**; a broken edit keeps the last working pipeline and shows the error overlay; broken at startup = logged error | "Engine never gets in my way" guarantee #1, and `run.bat test` still catches shipped breakage |
| 2026-10-10 | Smoke test **breaks and fixes a real shader file** in `Build/SmokeTest/` and checks error location, last-good pipeline and recovery | The hot-reload acceptance criterion is verified on every `run.bat test`, not just by hand |
| 2026-10-10 | ImGui layout lives in **`User/imgui.ini`**; UI uses GLFW privately (ImGui input backend) | Machine-specific state stays in `User/` |
| 2026-10-10 | Render graph **keeps textures across `reset()`** when name + description match; every graph texture gets `SAMPLED` usage | Toggling techniques doesn't reallocate; the texture viewer can show anything |
| 2026-10-10 | Barrier model: per physical image track layout, last write and reads-since-write; transient images keep stage masks across frames so frame N+1 waits for frame N's use (no per-frame copies) | Correct with 2 frames in flight, minimal memory, passes sync validation |
| 2026-10-10 | Screenshots (F12) via a **built-in uncompressed PNG writer**; the smoke test saves `Build/SmokeTest/LastFrame.png` | No extra dependency for M0a; lets every run be inspected visually |
| 2026-10-10 | Placeholder tonemap = exposure + **ACES fit (Narkowicz)** + sRGB encode | Standard and simple; better tonemapping/exposure is an M7 topic |
| 2026-10-10 | Starter assets pinned in `Content/AssetManifest.json`: Sponza (Khronos, 71 files @4995a638), Damaged Helmet, Kloofendal 2K HDRI (Poly Haven). Blue noise added in step 9 | Spec starter set; downloaded from the original sources |
| 2026-10-10 | **Sponza's model files are under the CRYENGINE Limited License Agreement** (not CC), recorded in `CREDITS.md` | Fine for a non-commercial portfolio since we never redistribute it; revisit if the project's use changes |
| 2026-10-10 | Scene errors carry **file + line + field path** via nlohmann `JSON_DIAGNOSTIC_POSITIONS`; problems are collected, the broken entity skipped, an unparsable file changes nothing; shown on screen and logged | Spec guarantee #5 ("bad JSON gives a clear error and doesn't crash") |
| 2026-10-10 | Scene units: meters, degrees, lux (sun), candela (point/spot); exposure as **EV100** per scene; bookmark yaw 0 = -Z, 90 = +X | Physical units make the path tracer and raster lighting comparable |
| 2026-10-10 | Namespaces mirror **modules**: everything in `World/` (incl. Components/, Systems/, Serialization/) is `ghost::world` | Readable component names (`world::Transform`); subfolders organize files only |
| 2026-10-10 | Debug optimization flags are per target; **Assets builds optimized even in Debug** (`OPTIMIZE_IN_DEBUG`) | Sponza import 7.8 s → 0.3 s in Debug (also fixed: block file reads instead of stream iterators) |
| 2026-10-10 | Asset cache (`AssetRegistry`) survives scene reloads; failed loads are not cached | Fast scene hot reload; fixing a missing asset works without restarting |
| 2026-10-10 | New module **`Graphics/GpuScene`** depends on **Assets** (CPU `ModelData`/`ImageData` only) besides Graphics/Vulkan; World → Graphics/GpuScene | The GPU scene consumes plain asset structs; still no ECS types anywhere in Graphics |
| 2026-10-10 | GPU scene = **global** vertex/index/primitive/material buffers (rebuilt when a model is added) + per-frame instance/draw/light/`FrameConstants` buffers per frame in flight; shaders get **one address** (`FrameConstants`) via push constants | One way to reach every scene datum from raster, compute and RT shaders; no descriptor updates |
| 2026-10-10 | Instance = entity × model node; `entityId` = entt entity + 1 (0 = sky), written to `gbuffer.entityId` | Picking and per-entity debug views map pixels back to entities |
| 2026-10-10 | G-buffer: D32 depth, RGBA16 SNORM normals (octahedral shading + geometric), RGBA8 sRGB albedo, RG8 metal/rough, RGBA16F emissive, RG16F motion (prevUV − currUV), R32 uint entity ID. **Vertex pulling** (no vertex input state) | Small, standard, and everything RT techniques need (geometric normal for offsets, motion for temporal reuse) |
| 2026-10-10 | glTF **blend** materials are rendered **alpha-tested** in the G-buffer | Deferred shading can't blend; transparency is a later technique topic |
| 2026-10-10 | Front face = **counter-clockwise** with the Y flip done in the projection matrix (image upright, winding preserved) | Matches glTF; verified on Sponza (back-face culling correct) |
| 2026-10-10 | Placeholder lighting is **Lambert only** (sun + point/spot, no shadows) + constant ambient from the environment's average radiance; **no GGX** | Shadows are M1 and the GGX BRDF is M2, both the user's work |
| 2026-10-10 | Environment radiance = HDRI texel × **intensity in nits** (HDRIs aren't absolute; ~5000 for the Kloofendal sky at EV100 14); no HDRI = **procedural daylight sky** already in nits. Shared by raster and path tracer via `ShaderLibrary/Environment.slang` | Physical exposure needs absolute radiance; one environment function keeps raster and reference comparable. The engine logs the resulting average luminance |
| 2026-10-10 | `BackgroundPass` (procedural splash sky) **removed**; sky pixels are drawn by the lighting pass from the environment | One sky definition instead of two |
| 2026-10-10 | Pass shader files must not share a name with a `ShaderLibrary` module (`GBuffer.slang` → `GBufferFill.slang`) | Slang resolves `import X` next to the importing file first, so a clash silently shadows the library module |
| 2026-10-10 | New module **`Sandbox/Player`** (`FlyController`): RMB-look, WASD, **Space/C up/down**, Shift/Ctrl, wheel speed persisted in `User/settings.json`; scene load teleports to the `player.start` bookmark | Spec fly camera; Q and E stay free for the M0b spawn menu / physics gun ([[16 Sandbox, ECS and Scenes]]) |
| 2026-10-10 | New module **`Graphics/RayTracing`** (→ Core, Vulkan, ShaderCompiler, GpuScene): `SceneAccelerationStructures`, `RayTracingPipeline`, `ShaderBindingTable`, `AccelerationStructure` | [[14 Project Structure]] layout; RT pipelines reuse the pipeline library's hot reload through `addCustom` |
| 2026-10-10 | **One BLAS per mesh, one geometry per primitive**, `PREFER_FAST_TRACE` + **compaction** (Sponza 18.2 → 8.4 MB), rebuilt only when `geometryRevision` changes | Static meshes dominate; `GeometryIndex()` maps straight to the primitive (material); compaction halves memory on an 8 GB card |
| 2026-10-10 | **One TLAS per frame in flight**, full rebuild every frame (first pass of the graph), stored at **bindless binding 3** (`gSceneTlas[frame->tlasIndex]`) | No hazard with the other in-flight frame, no refit bookkeeping; rebuilds are ~0.05 ms at sandbox instance counts |
| 2026-10-10 | TLAS instance custom index = GpuInstance index; alpha-masked geometry non-opaque (any-hit alpha test), everything else opaque; instances never cull faces | Shaders reach instance + material from the hit alone; facing is a material decision |
| 2026-10-10 | Shaders trace with **`sbtRecordStride = 0`**; `sbtRecordOffset` picks the hit group | With one geometry per primitive the stride would index hit records past the table (this hung the GPU: `VK_ERROR_DEVICE_LOST`) |
| 2026-10-10 | Naive reference path tracer = spec version: cosine-sampled Lambert, fixed epsilon offset, sun only as a visible disc of the right solid angle, no NEE/MIS/RR, mip 0 textures, running-mean accumulation in RGBA32F; restarts on camera/scene/resolution change. **F5** toggles raster ↔ path traced | [[05 Architecture#Reference path tracer harness]]; each naive spot is an `RTG-TODO` (RTG1 ch. 6, 15, 16, 17, 20; RTG2 ch. 20) for the user's M2 upgrade. Sunlit Sponza converges very slowly without sun sampling (expected) |
| 2026-10-10 | Shared shader modules **`Material`** (factors × textures, overrides, normal mapping; implicit or explicit LOD via a Slang generic) and **`Random`** (PCG) | Raster and ray tracing evaluate materials identically; blue noise joins `Random` in step 9 |
| 2026-10-10 | Cornell box scene gets environment intensity **0** (black outside) | A closed-box reference scene lit only by its ceiling light, as in the classic setup |
| 2026-10-10 | Technique API: `setup(TechniqueBuilder&)` declares passes/resources/pipelines by **name**, `execute(TechniqueContext&)` records with name lookups (`ctx.storage("ao")`); explicit push-constant structs mirrored in the shader (frame constants address first) | Matches the [[06 Technique API]] target shape; no hidden binding magic to debug |
| 2026-10-10 | Techniques compile into an **OBJECT library** (`Ghost_Techniques`) found by a `CONFIGURE_DEPENDS` glob; registration via static `REGISTER_TECHNIQUE` objects; include rules checked at configure time | No CMake/engine edits per technique; object files keep the static registrars the linker would drop from a static library |
| 2026-10-10 | `Param<T>` registers itself with the technique under construction (thread-local owner set by `Technique`'s constructor); scene key = label in lowerCamelCase; loading a scene resets every technique to defaults (disabled) before applying its block | Params are plain members; scenes reproduce exactly |
| 2026-10-10 | Technique folder found from `__FILE__` (path from the `Techniques` component on, joined to the repo root) | Shader paths stay relative to the technique; still correct if the repo moves |
| 2026-10-10 | Stages PreLighting / Lighting / Denoise / Post around the engine's lighting pass; techniques read and write `scene.color` by name. Path traced mode runs **no** techniques | Simple, predictable order; the reference must not be altered by the thing it validates |
| 2026-10-10 | `_Template` uses compiling placeholder identifiers (`TechniqueName`, `TechniqueCategory`) and is built into the **smoke test only**; `new_technique.bat` rejects ShaderLibrary module names and duplicate class names | The template can never rot; the two mistakes that silently break builds are caught up front |
| 2026-10-10 | UI takes the keyboard **only during text input** (`WantTextInput`), the mouse while hovering a panel | A focused panel (Techniques opens focused) must not swallow WASD and F-key shortcuts; caught by the smoke test's player check |

See also: [[13 Open Questions]]
