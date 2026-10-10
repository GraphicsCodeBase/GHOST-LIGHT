# Engine
**Purpose:** all engine code (Claude's side of the [[03 Division of Labor]]); one subfolder per module.
**Owns:** the module folders below. Each folder with a `CMakeLists.txt` is one static library (`Ghost_<Path>`).
**Public API:** see each module's README. Headers are included from this folder as root: `#include "Core/Log.h"`.
**Depends on:** third-party targets from `Scripts/CMake/Dependencies.cmake`.
**Not responsible for:** techniques (`Techniques/`), shared shader code (`ShaderLibrary/`), content (`Content/`).

## Modules (so far)
| Module | Purpose | Depends on |
|---|---|---|
| [Core](Core/README.md) | Logging, asserts, root-relative paths, JSON files, user settings, frame timing | glm, nlohmann/json |
| [Platform](Platform/README.md) | OS window and keyboard/mouse input | Core, GLFW (private) |
| [Assets](Assets/README.md) | glTF/image/HDR import, procedural meshes, asset registry (CPU only) | Core, cgltf + stb (private) |
| [Graphics/Vulkan](Graphics/Vulkan/README.md) | Vulkan instance, device, swapchain, frames in flight, VMA, crash reports | Core, Platform, volk, vk-bootstrap, VMA |
| [Graphics/ShaderCompiler](Graphics/ShaderCompiler/README.md) | Slang → SPIR-V, pipeline library, hot reload | Core, Graphics/Vulkan, Slang (private) |
| [Graphics/RenderGraph](Graphics/RenderGraph/README.md) | Pass declarations → allocations, barriers, timings | Core, Graphics/Vulkan |
| [Graphics/GpuScene](Graphics/GpuScene/README.md) | Scene geometry/materials/textures/instances/lights in GPU buffers | Core, Assets, Graphics/Vulkan |
| [Graphics/RayTracing](Graphics/RayTracing/README.md) | BLAS/TLAS, shader binding tables, RT pipelines | Core, Graphics/Vulkan, ShaderCompiler, GpuScene |
| [Graphics/Passes](Graphics/Passes/README.md) | Built-in render passes + their shaders | Core, Graphics/Vulkan, ShaderCompiler, RenderGraph, GpuScene, RayTracing |
| [Graphics/TechniqueRuntime](Graphics/TechniqueRuntime/README.md) | Technique base class, `Param<T>`, registry, technique passes | Core, Graphics/Vulkan, ShaderCompiler, RenderGraph, GpuScene, RayTracing |
| [Graphics/Renderer](Graphics/Renderer/README.md) | Per-frame orchestration | Core, Platform, Graphics/* |
| [World](World/README.md) | EnTT world, components, systems, scene/prefab JSON loading, ECS → GPU scene extraction | Core, Assets, Graphics/GpuScene, EnTT |
| [Sandbox/Player](Sandbox/Player/README.md) | Fly camera (walk mode in M0b) | Core, Platform |
| [UI](UI/README.md) | Dear ImGui, overlays (shader errors, performance) | Core, Platform, Graphics/*, ImGui (private) |
| [DebugTools](DebugTools/README.md) | Screenshots (F12) | Core, Graphics/Renderer |
| [App](App/README.md) | `Engine` class, frame loop, `GhostLight.exe` | Core, Platform, Assets, World, Graphics/Renderer, UI, DebugTools, Sandbox/Player |

More Graphics submodules arrive in later M0a steps (see [Graphics](Graphics/README.md)). Physics and the rest of Sandbox arrive in M0b.

## Rules enforced by the build
- A module may only `#include` modules it lists in `DEPENDS` (directly or transitively). Breaking this fails
  the CMake configure step with the offending file and line (`ghost_check_module_includes`).
- `Internal/` headers are private to their module.
- `/W4 /WX`: warnings are errors in engine code.
