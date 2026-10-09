# Graphics/GpuScene
**Purpose:** the scene as the GPU sees it. Plain data in (no ECS types), buffer device addresses out.
**Owns:** the global vertex/index/primitive/material buffers, every model texture (bindless), the environment map, and
the per-frame instance/draw/light buffers + `FrameConstants` (one set per frame in flight).

**Public API**
| Type | What it does |
|---|---|
| `GpuScene` | `uploadModel(ModelData)` → `ModelId` (blocking: geometry appended to the global buffers, textures uploaded with mips); `setEnvironment(image, intensity)`; per frame `setInstances()`, `setLights()`, `setCamera()`, `setExposure()`; the Renderer calls `prepareFrame()` and passes read `frameConstantsAddress()`, `indexBuffer()`, `drawCommands(category)` |
| `GpuSceneTypes.h` | `GpuPrimitive`, `GpuMaterial`, `GpuInstance`, `GpuDraw`, `GpuLight`, `FrameConstants` (scalar layout). **Mirror:** `ShaderLibrary/Scene.slang`; change both together |
| `TextureUploader` | Staged upload in 256 MB batches, mip chain by blits, ends in `SHADER_READ_ONLY_OPTIMAL`, registers bindless indices |

How shaders reach the scene: one push constant (the `FrameConstants` address) → `frame->vertices`, `frame->materials`,
`frame->instances[i]`... See [[05 Architecture]] and `ShaderLibrary/Scene.slang`.

Conventions
- One **instance** = one entity × one node of its model (one mesh placement; one TLAS instance in step 7).
  `entityId` = entt entity + 1 (0 = sky), so picking can map a pixel back to an entity.
- **Draws** are bucketed by pipeline: opaque (back-face culled), double-sided, alpha-masked. glTF "blend" materials
  are drawn alpha-tested (deferred shading can't blend).
- Camera: reverse-Z infinite projection (depth 1 = near plane, 0 = infinitely far), Vulkan clip space (Y flipped in the
  projection, image upright, winding preserved). `previousViewProjection` and `previousWorld` feed motion vectors.
- Environment: radiance = HDRI texel × intensity (nits). The average radiance (solid-angle weighted) is precomputed
  for the placeholder ambient term.

**Depends on:** Core, Assets (CPU-side `ModelData`/`ImageData` only), Graphics/Vulkan.
**Not responsible for:** deciding what's in the scene (World's `GpuSceneExtractionSystem` fills it), acceleration
structures (Graphics/RayTracing, step 7), drawing (Passes).
