# Graphics/Renderer
**Purpose:** runs one frame on the GPU; the only graphics class App talks to.
**Owns:** the Vulkan objects from `Graphics/Vulkan` (instance → surface → device → frames → swapchain), the shader
compiler + pipeline library, the render graph, the GPU scene, the scene's acceleration structures and the built-in
passes, and their order of creation and destruction.

**Public API**
- `initialize(window, {validation, vsync})`: returns false with a logged reason (no RTX GPU, old driver, ...).
- `gpuScene()`: fill it (World's extraction system does) before `renderFrame()`.
- `renderFrame(overlay)`: skips minimized windows, recreates the swapchain on resize, picks up edited shaders, rebuilds
  BLASes if geometry changed, then acquire → `gpuScene.prepareFrame()` → render graph → submit (timeline + present
  semaphores) → present.
- `setMode(Mode::Raster | Mode::PathTraced)` (F5 in the app), `pathTracer()` (sample count, bounces),
  `accelerationStructures()`, `techniques()`. Mode switches and technique toggles rebuild the graph before the next frame.
- `requestCapture(callback)` (screenshots), `gpuTimings()`, `gpuFrameMilliseconds()`, `validationActive()`, `gpuName()`.

Render graph today: **TLAS → GBuffer → [PreLighting] → Lighting → [Lighting] → [Denoise] → [Post] → Tonemap → [TextureViewer] → UI**
(raster; brackets = enabled techniques of that stage) or **TLAS → PathTracer → Tonemap → [TextureViewer] → UI** (path traced; TextureViewer only while a texture is chosen in F3), → Capture
when requested.

**Depends on:** Core, Platform, Graphics/Vulkan, ShaderCompiler, RenderGraph, GpuScene, RayTracing, Passes, TechniqueRuntime.
**Not responsible for:** window events (Platform), deciding what to draw (World → GpuScene), technique logic (Techniques/).
