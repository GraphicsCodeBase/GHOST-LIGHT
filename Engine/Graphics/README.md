# Graphics
**Purpose:** everything that talks to the GPU. Not a module itself: each subfolder below is one module.
**Owns:** nothing directly; see the submodules.
**Public API:** techniques use only the Graphics public API (see [[06 Technique API]]).
**Depends on:** Core, Platform; GpuScene also reads Assets' CPU-side model/image structs. **Never** World/EnTT or
Physics: the build rejects such includes, so techniques can't reach the ECS even by accident.
**Not responsible for:** entities and scene files (World), CPU-side asset decoding (Assets), UI widgets (UI).

| Submodule | Purpose | Status |
|---|---|---|
| [Vulkan](Vulkan/README.md) | Instance, device, swapchain, frames in flight, memory, debug names, GPU crash reports | M0a step 2 ✅ |
| [Renderer](Renderer/README.md) | Per-frame orchestration: acquire → record → submit → present | M0a step 2 ✅ |
| [ShaderCompiler](ShaderCompiler/README.md) | Slang → SPIR-V, pipelines, hot reload, error overlay data | M0a step 3 ✅ |
| [RenderGraph](RenderGraph/README.md) | Passes, transient/persistent/history resources, automatic barriers, GPU timers | M0a step 4 ✅ |
| [GpuScene](GpuScene/README.md) | Global geometry/material buffers, bindless textures, per-frame instances/lights/frame constants | M0a step 6 ✅ |
| RayTracing | BLAS/TLAS, shader binding table, RT pipelines | M0a step 7 |
| [Passes](Passes/README.md) | G-buffer, placeholder lighting, tonemap (step 6 ✅), reference path tracer (step 7) | M0a steps 3–7 |
| TechniqueRuntime | Technique base class, registry, `Param<T>` | M0a step 8 |
