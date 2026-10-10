# UI
**Purpose:** Dear ImGui setup plus the engine's overlays and panels.
**Owns:** the ImGui context, its GLFW + Vulkan backends, `User/imgui.ini` (window layout, machine-specific).

**Public API**
| Class | What it does |
|---|---|
| `ImGuiLayer` | `initialize(window, renderer, persistLayout)`, `beginFrame()` (tells `Input` that the UI owns the keyboard only while a text field is active, and the mouse while it hovers a panel), `endFrame()`, `record(cmd)` (called through the renderer's overlay hook). Docking enabled, center of the screen stays see-through |
| `ShaderErrorOverlay` | Red panel at the top of the screen: pipeline, `file(line): message`, full compiler output. Shown only while a shader is broken |
| `SceneErrorOverlay` | Bottom panel: scene/prefab errors (red) and warnings (amber) as `file(line): field: message`, with Dismiss |
| `PerformanceOverlay` | Corner readout: fps, ms, GPU, resolution, render mode + path tracer samples, validation on/off, GPU time per pass |
| `TechniquePanel` | Techniques window (F2): techniques by category, enable checkbox, `Param<T>` widgets (slider / checkbox / color), Reset |
| `TextureViewerPanel` | Texture viewer window (F3): any render graph texture (format, size), channels incl. octahedral normals, value range, presets per texture name, inset or full screen. Drawn by `Graphics/Passes/TextureViewerPass` |

Spawn menu and inspector arrive in M0b.

**Depends on:** Core, Platform, Graphics/Vulkan, Graphics/ShaderCompiler, Graphics/Renderer (and through it the technique runtime). ImGui and GLFW are private
(GLFW only for ImGui's input backend).
**Not responsible for:** what the overlays report (they read Renderer/PipelineLibrary state), keyboard shortcuts (App, Sandbox).
