# 17 RT Plumbing Explainer
How **this engine** gets from triangles to a `TraceRay` hit, in two pages. Written after M0a for interviews
and for debugging your techniques. Code: `Engine/Graphics/RayTracing/` (see its README), shaders in
`Engine/Graphics/Passes/Shaders/ReferencePathTracer.slang` and `ShaderLibrary/Scene.slang`.

## 1. BLAS vs TLAS
| | BLAS (bottom level) | TLAS (top level) |
|---|---|---|
| Holds | Triangles of **one mesh**: one *geometry* per glTF primitive (Sponza's mesh: 103 geometries) | **Instances**: a BLAS address + 3×4 transform + custom index + mask + SBT offset + flags |
| Built | When geometry changes, i.e. at load (`SceneAccelerationStructures::updateBlases`) | **Every frame** (`recordTlasBuild`), first pass of the render graph |
| Flags | `PREFER_FAST_TRACE` + `ALLOW_COMPACTION` → compacted copy (18.3 → 8.4 MB for the default scenes) | `PREFER_FAST_TRACE`, mode `BUILD` |
| Copies | One per mesh | **One per frame in flight** (bindless binding 3, `gSceneTlas[frame->tlasIndex]`), so frame N+1 rebuilds while frame N still traces |

**Geometry flags:** opaque materials get `OPAQUE` (any-hit never runs, the fast path). Alpha-masked ones get
`NO_DUPLICATE_ANY_HIT_INVOCATION` so the any-hit alpha test runs once per triangle.
**Instance flags:** `TRIANGLE_FACING_CULL_DISABLE` (glTF double-sided materials + paths from inside geometry).
`instanceCustomIndex` = index into `frame->instances`, which is how a hit finds its transform and model.

**Rebuild vs refit.** A *refit* (`MODE_UPDATE`, needs `ALLOW_UPDATE`) keeps the tree topology and only moves the
bounds: cheap, but quality degrades as things move far. A *rebuild* makes a fresh tree. Here the TLAS is **rebuilt
every frame**. It holds a few hundred instances, so it costs ~0.05 ms, and it stays optimal with no state.
BLASes are static. Skinned or deforming meshes would refit their BLAS each frame and rebuild it now and then.
M0b's physics objects only move instances, so only the TLAS changes.

## 2. Shader binding table (SBT)
The SBT is a GPU buffer of **shader group handles**: opaque IDs from `vkGetRayTracingShaderGroupHandlesKHR`.
It tells the GPU which shader to run for raygen, for each miss and for each hit. `ShaderBindingTable.cpp` lays it out:

```
 base-aligned (64 B)        base-aligned            base-aligned
┌──────────────────┬─────┬──────────────────┬─────┬──────────────────────────┐
│ RAYGEN  handle   │ pad │ MISS[0] handle   │ pad │ HIT[0] closestHit+anyHit │
└──────────────────┴─────┴──────────────────┴─────┴──────────────────────────┘
 raygen: stride = size     each record: one handle, spaced by shaderGroupHandleAlignment
```
No local root data: every record is just a handle. Data reaches the shaders through **one bindless set**
and **buffer device addresses** in push constants ([[12 Decisions Log]]).

**Which hit record runs?** `hitRecord = instanceSbtOffset + sbtRecordOffset + sbtRecordStride × GeometryIndex()`.
This engine uses offset 0, offset 0 and **stride 0**, so every geometry of every instance runs `HIT[0]`. The shader
then finds the material itself from `InstanceID()` + `GeometryIndex()` + `PrimitiveIndex()` (`loadHitSurface`).
*War story:* stride 1 made Sponza's 103 geometries index past the single hit record and caused a **device lost**.
Per-material hit groups would need stride 1 and one record per geometry.

## 3. Lifecycle of one `traceRays`
1. **Load:** glTF → GpuScene global vertex/index buffers → one BLAS per mesh (build, query compacted size, compact copy).
2. **Pipeline:** `RayTracingPipelineDesc` (raygen, misses, hit groups) → Slang → SPIR-V → `vkCreateRayTracingPipelinesKHR` → SBT.
   Saving the `.slang` file recompiles it and rebuilds the SBT (hot reload).
3. **Every frame:** GpuScene writes `FrameConstants` + instances. The **TLAS pass** builds this slot's TLAS, then a
   barrier: AS write → AS read.
4. **Path tracer pass:** bind the pipeline and the bindless set, push the constants (frame constants address, accumulation
   image index), then `vkCmdTraceRaysKHR(raygen, miss, hit, callable regions, width, height, 1)`.
5. **On the GPU:** raygen builds a camera ray and calls `TraceRay(tlas, flags, 0xFF, 0, 0, 0, ray, payload)`. Traversal
   walks TLAS → instance transform → BLAS. On non-opaque triangles it runs **any-hit** (alpha test → `IgnoreHit()`).
   It then runs **closest-hit** (writes instance/geometry/triangle/barycentrics into the payload) or **miss**
   (environment). Raygen shades from the payload and loops for the next bounce.
6. The render graph barrier makes the accumulation visible to Tonemap.

## 4. Ray query vs RT pipeline
| | Ray query (inline, `RayQuery<>`) | RT pipeline (`TraceRay`) |
|---|---|---|
| Where | Any shader: compute, fragment, even raygen | raygen / closest-hit / miss only |
| Hit logic | Your loop: `Proceed()`, commit candidates, read `Committed*` | Driver schedules separate shaders via the SBT |
| Setup | Just the TLAS | Pipeline + SBT + payload |
| Best for | Shadows/AO/visibility: "did I hit, where" (simple, coherent) | Many material shaders, recursion, big divergent workloads |
| In this engine | `Techniques/Examples/RayQueryNormals` (compute) | `ReferencePathTracerPass` |
On RTX 20-series both use the same RT cores. Ray query often wins for one-bounce effects, since it skips the shader
handoff. Pipelines win with large divergent workloads (40-series SER is out of scope: [[04 Tech Stack]]).

## 5. Ten likely interview questions
1. **Why two levels?** Instancing and cheap animation: move or duplicate objects by rebuilding a tiny TLAS, never the triangles.
2. **Refit or rebuild?** Refit when topology is stable and motion is small (skinning). Rebuild when things move a lot, or when the structure is small (our TLAS).
3. **What is compaction?** After the build, query the real size and copy into a smaller buffer (`COMPACT` mode). Here it about halves BLAS memory.
4. **Why one TLAS per frame in flight?** The GPU may still trace frame N's TLAS while the CPU records frame N+1's rebuild. Separate copies avoid a stall or a race.
5. **What decides which hit shader runs?** The SBT record formula above: instance offset + ray offset + stride × geometry index.
6. **Any-hit vs closest-hit?** Any-hit runs on every candidate (accept/ignore, for alpha testing). Closest-hit runs once on the final hit. Mark opaque geometry `OPAQUE` so any-hit is skipped.
7. **How does a hit shader find its material without local root data?** `InstanceID()` → instance buffer → model/primitive offset, `GeometryIndex()` → primitive → material, all through buffer addresses (bindless).
8. **Self-intersection?** Offset the ray origin (an epsilon here). Better: RTG1 ch. 6 ("A Fast and Robust Method for Avoiding Self-Intersection"), an `RTG-TODO` in the path tracer.
9. **When ray query over pipelines?** Simple visibility in a compute or fragment pass: shadows, AO, picking (M0b uses it for entity picking).
10. **What does `shaderGroupBaseAlignment` do?** Each SBT region must start on it (64 B on NVIDIA), and handles are spaced by `shaderGroupHandleAlignment`. Wrong alignment = validation error or garbage shaders.

Related: [[05 Architecture]] · [[06 Technique API]] · [[08 RTG Technique Catalog]] (RTG2 ch. 15–17 cover the same ground in depth).
