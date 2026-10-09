# Graphics/Passes
**Purpose:** the engine's built-in render passes. Techniques plug in between them (see [[06 Technique API]]).
**Owns:** the pass classes and their Slang shaders (`Shaders/`).

| Pass | What it does | Status |
|---|---|---|
| `GBufferPass` | Raster, vertex pulling from the GPU scene: writes `gbuffer.depth/normal/albedo/material/emissive/motion/entityId` (layout in `ShaderLibrary/GBuffer.slang`). Three pipelines: opaque, double-sided, alpha-masked | ✅ |
| `LightingPass` | Compute: placeholder deferred lighting into `scene.color` (Lambert sun + point/spot, no shadows, constant environment ambient, emission; sky pixels show the environment) | ✅ |
| `TonemapPass` | Raster: `scene.color` × exposure (from the scene's EV100) → ACES fit → sRGB into the swapchain | ✅ |
| Reference path tracer | Naive path tracer with accumulation | step 7 |

Frame today: GBuffer → Lighting → Tonemap → UI (→ Capture when a screenshot is requested). Passes add themselves with
`pass.addTo(graph, scene)`; the UI and frame-capture passes are added by the Renderer.

**Depends on:** Core, Graphics/Vulkan, Graphics/ShaderCompiler, Graphics/RenderGraph, Graphics/GpuScene.
**Not responsible for:** technique code (Techniques/), frame orchestration (Renderer), resource barriers (RenderGraph),
shadows and BRDFs (those are the user's techniques: M1 and M2 in [[07 Roadmap]]).
