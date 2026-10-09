# Graphics/Passes
**Purpose:** the engine's built-in render passes. Techniques plug in between them (see [[06 Technique API]]).
**Owns:** the pass classes and their Slang shaders (`Shaders/`).

| Pass | What it does | Status |
|---|---|---|
| `SplashPass` | Placeholder: full-screen ghost-light glow until real frames exist | step 3 (replaced in step 4) |
| G-buffer, lighting, tonemap, reference path tracer | Raster G-buffer with motion vectors + entity ID, placeholder sun light, tonemap to the swapchain, naive path tracer | steps 4–7 |

**Depends on:** Core, Graphics/Vulkan, Graphics/ShaderCompiler.
**Not responsible for:** technique code (Techniques/), frame orchestration (Renderer), resource barriers (RenderGraph).
