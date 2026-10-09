# Graphics
**Purpose:** everything that talks to the GPU. Not a module itself: each subfolder below is one module.
**Owns:** nothing directly; see the submodules.
**Public API:** techniques use only the Graphics public API (see [[06 Technique API]]).
**Depends on:** Core, Platform. **Never** World/EnTT or Physics: the build rejects such includes, so techniques
can't reach the ECS even by accident.
**Not responsible for:** entities and scene files (World), CPU-side asset decoding (Assets), UI widgets (UI).

| Submodule | Purpose | Status |
|---|---|---|
| [Vulkan](Vulkan/README.md) | Instance, device, swapchain, frames in flight, memory, debug names, GPU crash reports | M0a step 2 ✅ |
| [Renderer](Renderer/README.md) | Per-frame orchestration: acquire → record → submit → present | M0a step 2 ✅ |
| ShaderCompiler | Slang → SPIR-V, hot reload, error overlay data | M0a step 3 |
| RenderGraph | Passes, transient/persistent/history resources, automatic barriers | M0a step 4 |
| GpuScene | ECS → GPU buffers, TLAS instances, previous transforms | M0a step 6 |
| RayTracing | BLAS/TLAS, shader binding table, RT pipelines | M0a step 7 |
| Passes | G-buffer, lighting, tonemap, reference path tracer | M0a steps 4–7 |
| TechniqueRuntime | Technique base class, registry, `Param<T>` | M0a step 8 |
