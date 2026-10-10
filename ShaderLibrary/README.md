# ShaderLibrary
**Purpose:** shared Slang modules that engine passes and techniques `import`. The only place shader code is shared.
**Owns:** the shader-side view of engine data (bindings, scene layout, G-buffer encoding) and small helpers.

| Module | Provides | Status |
|---|---|---|
| `Math` | `kPi`, `kTwoPi`, `kInvPi`, `linearToSrgb`, `srgbToLinear` | ✅ |
| `Bindless` | `gTextures[]`, `gSamplers[]`, `gStorageImages[]`, `gSceneTlas[2]` (one TLAS per frame in flight), sampler slot constants, `sampleTexture/sampleTextureLevel/loadTexture` | ✅ |
| `Scene` | Mirror of `GpuSceneTypes.h` (`Vertex`, `GpuMaterial`, `GpuInstance`, `GpuLight`, `FrameConstants` with typed buffer pointers), `equirectUv`, `reconstructWorldPosition`, `cameraRayDirection`, `sceneTlas(frame)`, `HitSurface loadHitSurface(frame, instance, geometry, triangle, barycentrics)` | ✅ |
| `Material` | `sampleMaterial` (fragment, implicit LOD) / `sampleMaterialLevel` (explicit LOD) → `MaterialSample`, `alphaTestFails`, `applyInstanceOverrides`, `applyNormalMap` | ✅ |
| `Packing` | `octahedralEncode/Decode` (unit vectors in two SNORM values) | ✅ |
| `GBuffer` | `GBufferIndices`, `Surface`, `loadSurface(frame, indices, pixel)`; documents the G-buffer layout | ✅ |
| `Environment` | `environmentRadiance(frame, direction)` (HDRI or procedural daylight sky, × intensity, in nits), `environmentAverageRadiance(frame)` | ✅ |
| `Random` | `pcgHash`, `Rng` (`next()`, `next2()`), `makeRng(pixel, sampleIndex)`, `blueNoise(frame->blueNoiseTexture, pixel, frame->frameIndex)` (4 channels, animated by the golden ratio) | ✅ |

> [!warning] Not here, on purpose
> BRDF importance sampling, MIS, robust ray offsets, reservoirs. Those are techniques ([[08 RTG Technique Catalog]]).

Rules: one module per file, `module <Name>;` at the top, `public` on everything meant for importers, a one-line purpose comment.
