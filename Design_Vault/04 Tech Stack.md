---
tags: [spec, tech]
---

# 04 · Tech Stack

## Core
| Choice | Decision | Why |
|---|---|---|
| Language | **C++20** | Studio standard, interview language, learned at uni |
| Graphics API | **Vulkan 1.3** + KHR ray tracing | Cross-vendor, explicit, valued on resumes (RTG2 ch.16) |
| Shading language | **Slang** → SPIR-V | Modern, good RT support, HLSL-like like most RTG listings |
| Compiler / build | **MSVC + CMake + Ninja**, all from the **VS 2022 C++ workload** | Only one prerequisite to install, see [[15 Setup and Portability]] |
| Dependencies | `Scripts/Dependencies.json` manifest, downloaded by `run.bat` into `.tools/`, **pinned versions + SHA-256** | Automatic, reproducible, offline after the first run |
| Platform | Windows 10/11 x64, **NVIDIA RTX** GPU | Matches requirements |

## Libraries (all fetched automatically)
| Library | Purpose | Module |
|---|---|---|
| Vulkan-Headers + **volk** | Vulkan API without the SDK (loader comes with the driver) | Graphics |
| Vulkan Validation Layers | Debug validation, stored in `.tools/`, no system install | Graphics |
| vk-bootstrap | Device/swapchain setup | Graphics |
| Vulkan Memory Allocator | GPU memory | Graphics |
| Slang (prebuilt release) | Runtime shader compilation + hot reload | Graphics |
| GLFW | Window, input | Platform |
| glm | CPU math | Core |
| **EnTT** | ECS | World |
| **Jolt Physics** | Rigid bodies, character controller, raycasts | Physics |
| **nlohmann/json** | Scene/prefab/config JSON | World, Core |
| Dear ImGui (docking) | Debug UI | UI |
| **ImGuizmo** | Translate/rotate/scale gizmos | Sandbox |
| cgltf | glTF 2.0 loading | Assets |
| stb_image / stb_image_write | PNG/JPG/HDR load, PNG write | Assets, DebugTools |
| tinyexr | EXR write | DebugTools |
| Tracy (optional) | CPU profiling | Core |

## Vulkan features and extensions
- Core 1.3: dynamic rendering, synchronization2, timeline semaphores, buffer device address, descriptor indexing
- `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, `VK_KHR_deferred_host_operations`

## 🎮 Hardware notes (RTX 2070 Super = minimum target)
| Fact | Implication |
|---|---|
| Turing, 1st-gen RT cores | Supports everything above ✅ |
| No SER / opacity micromaps (Ada+) | Don't depend on them. Mark alpha-tested geometry carefully |
| 8 GB VRAM | BC7/BC5 compressed textures, VRAM budget display |
| Modest RT throughput | 1080p target, half-res effects, 1 ray per pixel + denoise |

## Optional debugging tools (not required to run)
- **NVIDIA Nsight Graphics**: RT debugging
- **RenderDoc**: raster passes

See also: [[05 Architecture]], [[15 Setup and Portability]]
