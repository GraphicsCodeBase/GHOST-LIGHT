# UI
**Purpose:** Dear ImGui setup plus the engine's overlays and panels.
**Owns:** the ImGui context, its GLFW + Vulkan backends, `User/imgui.ini` (window layout, machine-specific).

**Public API**
| Class | What it does |
|---|---|
| `ImGuiLayer` | `initialize(window, renderer, persistLayout)`, `beginFrame()` (also tells `Input` when ImGui wants the keyboard/mouse), `endFrame()`, `record(cmd)` (called through the renderer's overlay hook). Docking enabled, center of the screen stays see-through |
| `ShaderErrorOverlay` | Red panel at the top of the screen: pipeline, `file(line): message`, full compiler output. Shown only while a shader is broken |
| `PerformanceOverlay` | Corner readout: fps, ms, GPU, resolution, validation on/off |

Spawn menu, inspector and technique panels arrive in later steps / M0b.

**Depends on:** Core, Platform, Graphics/Vulkan, Graphics/ShaderCompiler, Graphics/Renderer. ImGui and GLFW are private
(GLFW only for ImGui's input backend).
**Not responsible for:** what the overlays report (they read Renderer/PipelineLibrary state), keyboard shortcuts (App, Sandbox).
