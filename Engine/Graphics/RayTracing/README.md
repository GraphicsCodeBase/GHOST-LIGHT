# Graphics/RayTracing
**Purpose:** everything needed to trace rays against the GPU scene: acceleration structures, shader binding tables and
hot-reloadable ray tracing pipelines.
**Owns:** every BLAS, the per-frame TLASes and their instance/scratch buffers, SBT buffers.

**Public API**
| Class | What it does |
|---|---|
| `SceneAccelerationStructures` | `updateBlases(scene)` (between frames, only when `GpuScene::geometryRevision()` changed): one BLAS per mesh, one geometry per primitive, built `PREFER_FAST_TRACE`, then **compacted** (Sponza: 18.2 MB → 8.4 MB). `recordTlasBuild(cmd, slot, scene)` (the Renderer's first pass each frame): rebuilds that frame slot's TLAS from `GpuScene::instances()` |
| `RayTracingPipeline` | `initialize(pipelines, device, deletionQueue, desc)` → hot-reloadable pipeline; `prepare()` each frame (false while broken; rebuilds the SBT after a reload); `traceRays(cmd, w, h)` |
| `RayTracingPipelineDesc` | ray generation entry, misses, triangle hit groups (closest hit + optional any-hit) |
| `ShaderBindingTable` | group handles laid out in aligned raygen / miss / hit regions (no per-record data) |
| `AccelerationStructure` | one BLAS or TLAS + its buffer + device address |

How shaders use it (see `ShaderLibrary/Scene.slang`)
```slang
TraceRay(sceneTlas(frame), RAY_FLAG_NONE, 0xFF, /*sbtRecordOffset*/ 0, /*sbtRecordStride*/ 0, /*missIndex*/ 0, ray, payload);
// closest hit: InstanceID() = index into frame->instances, GeometryIndex() = primitive within the mesh
HitSurface hit = loadHitSurface(frame, InstanceID(), GeometryIndex(), PrimitiveIndex(), attributes.barycentrics);
```
Ray queries work the same way: `RayQuery<...> q; q.TraceRayInline(sceneTlas(frame), ...)`.

Rules worth knowing
- **`sbtRecordStride` must be 0** with these BLASes: the hit record index is `instanceOffset + sbtRecordOffset +
  stride × GeometryIndex()`, and a mesh can have hundreds of geometries (one per primitive). `sbtRecordOffset`
  picks the hit group instead.
- Geometry of alpha-masked materials is non-opaque (any-hit runs, use `alphaTestFails()` from `Material`); all other
  geometry is opaque. Instances disable facing culling: shaders decide what a back face means.
- One TLAS **per frame in flight**, at bindless binding 3, index = frame slot (`frame->tlasIndex`). Rebuilding the
  TLAS of slot N never touches the TLAS the other in-flight frame is tracing against.

**Depends on:** Core, Graphics/Vulkan, Graphics/ShaderCompiler, Graphics/GpuScene.
**Not responsible for:** what gets traced (passes, techniques), deciding the scene's content (GpuScene/World).
