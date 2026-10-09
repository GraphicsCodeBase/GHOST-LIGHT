# ShaderLibrary
**Purpose:** shared Slang modules that engine passes and techniques `import`. The only place shader code is shared.
**Owns:** the shader-side view of engine data (bindings, scene layout, G-buffer encoding) and small helpers.

| Module | Provides | Status |
|---|---|---|
| `Math` | `kPi`, `kTwoPi`, `kInvPi`, `linearToSrgb`, `srgbToLinear` | ✅ |
| `Bindless` | `gTextures[]`, `gSamplers[]`, `gStorageImages[]`, sampler slot constants, `sampleTexture/sampleTextureLevel/loadTexture` | ✅ |
| `Scene` | Geometry/material fetch, barycentrics, TLAS, lights, previous transforms | steps 6–7 |
| `Random` | PCG RNG, blue noise lookup | step 9 |
| `Packing` | Octahedral normals and other packing helpers | step 6 |
| `GBuffer` | Decode the G-buffer into a surface struct | step 6 |

> [!warning] Not here, on purpose
> BRDF importance sampling, MIS, robust ray offsets, reservoirs. Those are techniques ([[08 RTG Technique Catalog]]).

Rules: one module per file, `module <Name>;` at the top, `public` on everything meant for importers, a one-line purpose comment.
