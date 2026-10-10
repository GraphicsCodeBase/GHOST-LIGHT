# Graphics/Passes
**Purpose:** the engine's built-in render passes. Techniques plug in between them (see [[06 Technique API]]).
**Owns:** the pass classes and their Slang shaders (`Shaders/`).

| Pass | What it does | Status |
|---|---|---|
| `GBufferPass` | Raster, vertex pulling from the GPU scene: writes `gbuffer.depth/normal/albedo/material/emissive/motion/entityId` (layout in `ShaderLibrary/GBuffer.slang`). Three pipelines: opaque, double-sided, alpha-masked | ✅ |
| `LightingPass` | Compute: placeholder deferred lighting into `scene.color` (Lambert sun + point/spot, no shadows, constant environment ambient, emission; sky pixels show the environment) | ✅ |
| `TonemapPass` | Raster: `scene.color` × exposure (from the scene's EV100) → ACES fit → sRGB into the swapchain | ✅ |
| `TextureViewerPass` | Raster, after Tonemap, only while a texture is chosen (F3 panel): draws any render graph texture point-sampled into an inset or full screen; channels RGB/R/G/B/A, signed, octahedral normals; value range; NaN/inf in magenta. Integer formats (`gbuffer.entityId`) are not viewable yet | ✅ |
| `ReferencePathTracerPass` | Ray tracing pipeline: naive progressive path tracer (cosine-sampled Lambert, epsilon offset, no light sampling, no MIS; every naive spot is an `RTG-TODO`). Accumulates a running mean in `pathtracer.accumulation` (RGBA32F, persistent) and restarts when the camera, `GpuScene::sceneRevision()`, resolution or bounce count change. `maxBounces`, `maxSamples` | ✅ |

Frame today: TLAS → GBuffer → Lighting → Tonemap → (TextureViewer) → UI in raster mode, TLAS → PathTracer → Tonemap → UI in path traced
mode (F5), plus Capture when a screenshot is requested. Passes add themselves with `pass.addTo(graph, scene)`; the TLAS,
UI and frame-capture passes are added by the Renderer.

**Depends on:** Core, Graphics/Vulkan, Graphics/ShaderCompiler, Graphics/RenderGraph, Graphics/GpuScene, Graphics/RayTracing.
**Not responsible for:** technique code (Techniques/), frame orchestration (Renderer), resource barriers (RenderGraph),
shadows and BRDFs (those are the user's techniques: M1 and M2 in [[07 Roadmap]]).
