---
tags: [spec, structure]
---

# 14 · Project Structure

> [!goal] Spec 7
> Navigating the codebase should be obvious. Every folder has one responsibility, every file has one clear purpose, nothing is left loose.

## Rules
1. **Module = folder = CMake library target.** Dependencies are declared explicitly. A module can't use another module it doesn't list.
2. **Dependencies flow one way.** No cycles (see diagram).
3. **Every module folder has a `README.md`**: purpose, what it owns, public classes, what it depends on, **what it is NOT responsible for**.
4. **One class per file**, file name = class name (`RenderGraph.h` / `RenderGraph.cpp`).
5. **Every file starts with a one-line purpose comment.**
6. **Namespaces mirror folders**: `ghost::graphics::raytracing`.
7. **Internals in `Internal/`** subfolders. Files you see first are the ones meant to be used.
8. **Root stays minimal**: `run.bat`, `new_technique.bat`, `README.md`, `CLAUDE.md`, `CMakeLists.txt`, `.gitignore`, `.gitattributes`.
9. **~500 lines soft limit** per file. Split when bigger.
10. **PascalCase** folder and file names.

## Dependency direction
```
            Core
              │
           Platform
              │
   ┌──────────┼───────────┬──────────┐
 Assets    Graphics     World      Physics
   │          │        (ECS)          │
   └──────────┴───────┬───┴───────────┘
                   Sandbox  ── DebugTools ── UI
                      │
                     App

 Techniques ──▶ Graphics (public API) + ShaderLibrary only
```
**Exact edges among the middle row** (decided 2026-10-10, see [[12 Decisions Log]]):
- `Graphics` → Core, Platform. **Never** World/EnTT, so techniques can't reach the ECS.
- `Assets` → Core only. CPU-side data (meshes, images); no Vulkan.
- `World` → Core, Assets, Graphics. Its `GpuSceneExtractionSystem` pushes plain data into `GpuScene`.
- `Physics` → World (M0b).
- `UI` → Graphics, Platform · `DebugTools` → UI, Graphics · `App` → everything.

## Layout
```
Raytracing_Engine/
├─ run.bat · new_technique.bat · README.md · CLAUDE.md · CMakeLists.txt · .gitignore
│
├─ Engine/                        ← Claude's code
│  ├─ Core/                       logging, asserts, math types, time, root-relative paths, JSON helpers, settings
│  ├─ Platform/                   window, input
│  ├─ Graphics/
│  │  ├─ Vulkan/                  device, swapchain, memory, sync, bindless descriptors, pipelines
│  │  ├─ RayTracing/              BLAS/TLAS, shader binding table, RT pipelines
│  │  ├─ RenderGraph/             passes, resources, barriers, history buffers
│  │  ├─ ShaderCompiler/          Slang compilation, hot reload, error overlay data
│  │  ├─ GpuScene/                ECS → GPU buffers, TLAS instances, prev transforms
│  │  ├─ Passes/                  G-buffer, tonemap, present, reference path tracer
│  │  └─ TechniqueRuntime/        Technique base class, registry, Param<T>
│  ├─ World/                      ECS (EnTT)
│  │  ├─ Components/              Transform, MeshRenderer, lights, RigidBody, …
│  │  ├─ Systems/                 transform, lights, prefab spawning
│  │  ├─ Prefabs/                 prefab loading + instancing
│  │  └─ Serialization/           scene JSON load/save
│  ├─ Physics/                    Jolt world, body ↔ ECS sync, character controller
│  ├─ Assets/                     glTF import, textures (BC compression), asset registry, manifest
│  ├─ Sandbox/
│  │  ├─ Tools/                   PhysicsGun, GizmoTool, SpawnTool, DeleteTool
│  │  ├─ Player/                  FlyController, WalkController, mode toggle
│  │  ├─ Selection/               picking (TLAS ray query), selection state
│  │  └─ Undo/                    undo stack
│  ├─ DebugTools/                 texture viewer, GPU timers, A/B view, metrics, benchmark, screenshots, camera paths
│  ├─ UI/                         ImGui setup, spawn menu, inspector, technique panels, overlays
│  └─ App/                        main(), engine loop, startup checks (GPU, RT support)
│
├─ Techniques/                    ← MY code
│  ├─ _Template/
│  ├─ Shadows/  Sampling/  Reflections/  ManyLights/  Denoising/  GlobalIllumination/
│
├─ ShaderLibrary/                 shared Slang modules: Scene, Random, Packing, Math, GBuffer
│
├─ Content/
│  ├─ Scenes/                     *.scene.json   (committed)
│  ├─ Prefabs/                    *.prefab.json  (committed), grouped: Props/, Lights/, Shapes/, Static/
│  ├─ AssetManifest.json          (committed)
│  └─ Assets/                     downloaded (gitignored) + CREDITS.md (committed)
│
├─ Scripts/                       PowerShell/batch helpers used by run.bat
├─ Tests/                         smoke tests
└─ Design_Vault/                  this spec

Generated (gitignored): Build/  .tools/  User/  Captures/
```

## Module README template
```markdown
# <Module>
**Purpose:** one sentence.
**Owns:** the data/resources this module is responsible for.
**Public API:** main classes and what they do.
**Depends on:** modules.
**Not responsible for:** common confusions (e.g. "Physics does not render debug shapes; DebugTools does").
```

See also: [[05 Architecture]], [[10 Coding Standards]]
