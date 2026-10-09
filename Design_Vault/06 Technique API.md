---
tags: [spec, api]
---

# 06 · Technique API

> [!goal] Design goal
> A new technique is **one folder** created by `new_technique.bat <Category> <Name>`: one C++ file + shaders + write-up. **No engine edits, no CMake edits.** The build picks it up automatically.

## Where techniques live
```
Techniques/
├─ _Template/                     ← copied by new_technique.bat
├─ Shadows/
│  └─ RayTracedShadows/
│     ├─ RayTracedShadows.cpp     ← technique logic (passes, params)
│     ├─ Shaders/
│     │  ├─ ShadowRayGen.slang
│     │  └─ ShadowMiss.slang
│     └─ README.md                ← write-up ([[Technique Writeup]])
├─ Sampling/
├─ Reflections/
├─ ManyLights/
├─ Denoising/
└─ GlobalIllumination/
```
Categories mirror [[08 RTG Technique Catalog]].

## What a technique looks like (target shape)
Illustrative. Claude finalizes the exact API in M0a, but it must stay this simple.

```cpp
// Ray traced ambient occlusion: example of the technique pattern.
class RTAmbientOcclusion : public Technique {
public:
    TECHNIQUE_INFO("RT Ambient Occlusion", "Example");

    Param<float> radius       {"Radius",         1.0f, 0.1f, 5.0f};
    Param<int>   rayCount     {"Rays per pixel", 1,    1,    16};
    Param<bool>  useBlueNoise {"Blue noise",     true};

    void setup(GraphBuilder& g) override {
        g.read ("gbuffer.depth");
        g.read ("gbuffer.normal");
        g.write("ao", Format::R16F, Scale::Half);
        g.rayPipeline("rtao", {"Shaders/RtaoRayGen.slang", "Shaders/RtaoMiss.slang"});
    }

    void execute(PassContext& ctx) override {
        ctx.pushConstants(radius, rayCount, useBlueNoise, ctx.frameIndex());
        ctx.traceRays(ctx.resolution("ao"));
    }
};
REGISTER_TECHNIQUE(RTAmbientOcclusion);
```

## Required capabilities
| Need | API |
|---|---|
| Read G-buffer / other outputs | `g.read("name")` |
| Output texture at full/half res | `g.write("name", format, scale)` |
| Temporal data | `g.history("name", format)` → `prev` + `curr` |
| Structured buffers (reservoirs, hash grids) | `g.buffer("name", elementSize, count, Persistent)` |
| RT pipeline | `g.rayPipeline(...)` |
| Ray queries in compute | `g.computePipeline(...)`, TLAS bound automatically |
| Multiple passes per technique | e.g. ReSTIR: initial → temporal → spatial → shade |
| Scene data in shaders | `import Scene;` → geometry, materials, textures, lights, TLAS, prev transforms |
| Debug output | everything written is visible in the texture viewer |
| Tunable params | `Param<T>` → ImGui + saved in scene JSON (`"techniques"` block) |
| Ordering | stage: `PreLighting`, `Lighting`, `Denoise`, `Post` |
| Reacting to changes | `ctx.sceneChanged()`, `ctx.lightsChanged()` (e.g. rebuild alias table when a light is spawned) |

## Rules
- Techniques **only** use the Graphics public API and `ShaderLibrary/`. **Never** EnTT, Jolt, or Sandbox (see [[05 Architecture#Key boundary]]).
- Technique shaders only `import` from `ShaderLibrary/` or their own `Shaders/` folder.

## Shader library (`ShaderLibrary/`)
| Module | Provides |
|---|---|
| `Scene` | geometry/material fetch, barycentrics, TLAS, lights, prev transforms |
| `Random` | PCG RNG, blue noise lookup |
| `Packing` | octahedral normals, etc. |
| `Math` | constants, tangent frames, helpers |
| `GBuffer` | decode G-buffer into a surface struct |

> [!warning] Not in the shared library
> BRDF importance sampling, MIS, robust offsets, reservoirs. **Those are my techniques.**

See also: [[05 Architecture]], [[14 Project Structure]]
