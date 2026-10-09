# ShaderLibrary
**Purpose:** shared Slang modules that engine passes and techniques `import`. The only place shader code is shared.
**Owns:** the shader-side view of engine data (bindings, scene layout, G-buffer encoding) and small helpers.

| Module | Provides | Status |
|---|---|---|
| `Math` | `kPi`, `kTwoPi`, `kInvPi`, `linearToSrgb`, `srgbToLinear` | ✅ |
| `Bindless` | `gTextures[]`, `gSamplers[]`, `gStorageImages[]`, sampler slot constants, `sampleTexture/sampleTextureLevel/loadTexture` | ✅ |
| `Scene` | Mirror of `GpuSceneTypes.h` (`Vertex`, `GpuMaterial`, `GpuInstance`, `GpuLight`, `FrameConstants` with typed buffer pointers), `equirectUv`, `reconstructWorldPosition`, `cameraRayDirection` | ✅ (TLAS + hit helpers: step 7) |
| `Packing` | `octahedralEncode/Decode` (unit vectors in two SNORM values) | ✅ |
| `GBuffer` | `GBufferIndices`, `Surface`, `loadSurface(frame, indices, pixel)`; documents the G-buffer layout | ✅ |
| `Environment` | `environmentRadiance(frame, direction)` (HDRI or procedural daylight sky, × intensity, in nits), `environmentAverageRadiance(frame)` | ✅ |
| `Random` | PCG RNG, blue noise lookup | step 9 |

> [!warning] Not here, on purpose
> BRDF importance sampling, MIS, robust ray offsets, reservoirs. Those are techniques ([[08 RTG Technique Catalog]]).

Rules: one module per file, `module <Name>;` at the top, `public` on everything meant for importers, a one-line purpose comment.
