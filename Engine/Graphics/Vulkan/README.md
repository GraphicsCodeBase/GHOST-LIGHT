# Graphics/Vulkan
**Purpose:** the Vulkan core every other graphics module builds on.
**Owns:** `VkInstance`, debug messenger, `VkSurfaceKHR`, `VkDevice` + graphics queue, the VMA allocator, the swapchain,
per-frame command pools/buffers, the frame timeline semaphore.

**Public API**
| Class | What it does |
|---|---|
| `Instance` | Loads the Vulkan loader with volk (no SDK), creates the instance via vk-bootstrap, enables the pinned validation layer + **synchronization validation** in Debug, logs validation errors/warnings into the engine log |
| `LoaderConfiguration` | Before the loader loads: `VK_LAYER_PATH` = `.tools/validation-layers` (an installed SDK can never stand in), overlay layers (Steam, Epic, OBS, RTSS...) disabled. RenderDoc/Nsight still work |
| `Surface` | Win32 surface from `platform::Window::nativeHandle()` |
| `Device` | Picks an RTX GPU (Vulkan 1.3 + `VK_KHR_acceleration_structure` / `ray_tracing_pipeline` / `ray_query`), enables bindless, buffer device address, timeline semaphores, dynamic rendering, sync2; creates VMA; exposes RT/AS properties |
| `Swapchain` | `B8G8R8A8_UNORM` images (tonemap writes display-encoded values), FIFO or MAILBOX/IMMEDIATE, recreate on resize, one present semaphore per image |
| `FrameScheduler` | 2 frames in flight. One **timeline semaphore**: frame N signals value N; reusing a slot waits for its last value (no fences) |
| `BindlessDescriptors` | The one global descriptor set (16384 sampled images, 32 samplers, 4096 storage images, 2 scene TLASes; update-after-bind, partially bound) and **the one pipeline layout** every pipeline uses (set 0 + 256 B push constants). `addSampledImage(view)` → shader index. Default samplers: linear/clamp/nearest/anisotropic |
| `ImmediateSubmit` | `run([](VkCommandBuffer){...})`: records, submits and waits (load-time uploads only; never inside a frame) |
| `DeletionQueue` | `push(lastFrameUsingIt, fn)`: destroys replaced GPU objects (hot-reloaded pipelines, resized images) once that frame finished on the GPU |
| `DebugUtils` | `setName(handle, "name")` overloads + command-buffer labels |
| `GpuCrashReporter` | `checkpoint(cmd, "PassName")` breadcrumbs (`VK_NV_device_diagnostic_checkpoints`); on `VK_ERROR_DEVICE_LOST` logs the last pass reached and what to check |
| `VK_CHECK(call)` (`VulkanCheck.h`) | Fatal check with call text, `VkResult` name, file and line |

**Depends on:** Core, Platform (window handle only). Third-party: volk, vk-bootstrap, VMA (public), Vulkan-Headers.
**Not responsible for:** what gets rendered (Renderer, passes), shader compilation (ShaderCompiler), resource
lifetimes per frame (RenderGraph).
