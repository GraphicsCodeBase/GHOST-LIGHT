# Graphics/Renderer
**Purpose:** runs one frame on the GPU; the only graphics class App talks to.
**Owns:** the Vulkan objects from `Graphics/Vulkan` (instance → surface → device → frames → swapchain), the shader
compiler + pipeline library, the render graph, the GPU scene and the built-in passes, and their order of creation and
destruction.

**Public API**
- `initialize(window, {validation, vsync})`: returns false with a logged reason (no RTX GPU, old driver, ...).
- `gpuScene()`: fill it (World's extraction system does) before `renderFrame()`.
- `renderFrame(overlay)`: skips minimized windows, recreates the swapchain on resize, picks up edited shaders, then
  acquire → `gpuScene.prepareFrame()` → render graph → submit (timeline + present semaphores) → present.
- `requestCapture(callback)` (screenshots), `gpuTimings()`, `gpuFrameMilliseconds()`, `validationActive()`, `gpuName()`.

Render graph today: **GBuffer → Lighting → Tonemap → UI** (→ Capture when requested). Techniques slot in between
Lighting and Tonemap from step 8.

**Depends on:** Core, Platform, Graphics/Vulkan, ShaderCompiler, RenderGraph, GpuScene, Passes.
**Not responsible for:** window events (Platform), deciding what to draw (World → GpuScene), technique logic (Techniques/).
