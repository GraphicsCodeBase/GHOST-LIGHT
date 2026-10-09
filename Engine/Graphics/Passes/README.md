# Graphics/Passes
**Purpose:** the engine's built-in render passes. Techniques plug in between them (see [[06 Technique API]]).
**Owns:** the pass classes and their Slang shaders (`Shaders/`).

| Pass | What it does | Status |
|---|---|---|
| `BackgroundPass` | Compute: procedural HDR sky + ghost-light bulb into `scene.color` (fallback background when a scene has no HDRI) | ✅ |
| `TonemapPass` | Raster: `scene.color` → exposure → ACES fit → sRGB into the swapchain | ✅ |
| G-buffer, lighting, reference path tracer | Raster G-buffer with motion vectors + entity ID, placeholder sun light, naive path tracer | steps 6–7 |

Passes add themselves with `pass.addTo(graph)`; the UI and frame-capture passes are added by the Renderer.

**Depends on:** Core, Graphics/Vulkan, Graphics/ShaderCompiler.
**Not responsible for:** technique code (Techniques/), frame orchestration (Renderer), resource barriers (RenderGraph).
