# Graphics/Renderer
**Purpose:** runs one frame on the GPU; the only graphics class App talks to.
**Owns:** the Vulkan objects from `Graphics/Vulkan` (instance → surface → device → frames → swapchain) and their order of creation and destruction.
**Public API:**
- `Renderer::initialize(window, {validation, vsync})`: returns false with a logged reason (no RTX GPU, old driver, ...).
- `Renderer::renderFrame(time)`: skips minimized windows, recreates the swapchain on resize/out-of-date, then
  acquire → record → submit (timeline + present semaphores) → present.
- `validationActive()`, `gpuName()`.

Right now the frame is a clear to a slowly breathing amber. Step 4 replaces it with the render graph
(G-buffer → techniques → tonemap → UI).

**Depends on:** Core, Platform, Graphics/Vulkan.
**Not responsible for:** window events (Platform), deciding what to draw (World → GpuScene), technique logic (Techniques/).
