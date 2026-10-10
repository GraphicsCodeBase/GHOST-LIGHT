# Graphics/TechniqueRuntime
**Purpose:** the API every technique in `Techniques/` is written against, and the runtime that turns techniques into
render graph passes. Spec: [[06 Technique API]].
**Owns:** one instance of every registered technique, their pipelines and enable state.

## Writing a technique
`new_technique.bat <Category> <Name>` creates `Techniques/<Category>/<Name>/` from `Techniques/_Template/`. The build
picks the folder up by itself (no CMake or engine edits) and the technique appears in the Techniques panel (F2).

```cpp
class RTAmbientOcclusion : public Technique {
public:
    TECHNIQUE_INFO("RT Ambient Occlusion", "GlobalIllumination");
    TechniqueStage stage() const override { return TechniqueStage::PreLighting; }

    Param<float> radius{"Radius", 1.0f, 0.1f, 5.0f};
    Param<int> rayCount{"Rays per pixel", 1, 1, 16};
    Param<bool> useBlueNoise{"Blue noise", true};

    void setup(TechniqueBuilder& b) override {
        b.read("gbuffer.depth").read("gbuffer.normal");
        b.write("ao", VK_FORMAT_R16_SFLOAT, TechniqueBuilder::Scale::Half);
        b.rayPipeline("rtao", {{"Shaders/Rtao.slang", "rayGeneration"}, {{"Shaders/Rtao.slang", "miss"}}, {}});
    }

    void execute(TechniqueContext& ctx) override {
        ctx.pushConstants(Constants{ctx.frameConstants(), ctx.sampled("gbuffer.depth"), ctx.sampled("gbuffer.normal"),
                                    ctx.storage("ao"), radius, rayCount, useBlueNoise ? 1u : 0u});
        ctx.traceRays("rtao", ctx.extent("ao"));
    }
};
REGISTER_TECHNIQUE(RTAmbientOcclusion);
```

**Public API**
| Type | What it does |
|---|---|
| `Technique` | `setup(TechniqueBuilder&)` (declare), `execute(TechniqueContext&)` (record), `stage()`, `enabled`, `params()`, `id()` (class name), `folder()` |
| `TECHNIQUE_INFO(name, category)` | UI name + category (a `Techniques/` subfolder) |
| `REGISTER_TECHNIQUE(Class)` | adds the class to the `TechniqueRegistry` at static-initialization time |
| `Param<T>` | `bool`, `int`, `float`, `glm::vec3` (color). Shown in the panel, loaded from the scene; reads like a value |
| `TechniqueBuilder` | `read` (sampled) · `write` (storage; with a format = create) · `readWrite` · `history` (this frame + `previous`) · `buffer` (by address) · `computePipeline` · `rayPipeline` · `pass(name, type)` for multi-pass techniques |
| `TechniqueContext` | `sampled/storage/previous(name)` bindless indices · `extent`, `renderSize` · `buffer(name)` address · `frameConstants()` (the `FrameConstants*` for `import Scene;`) · `pushConstants(struct)` · `dispatch(pipeline, size)` · `traceRays(pipeline, size)` · `sceneChanged()`, `lightsChanged()` · `pass()` · `cmd()` |
| `TechniqueManager` | owned by the Renderer: instances, `addPasses(graph, scene, stage)`, `applySettings(json)` |

**Frame order (raster mode):** TLAS → GBuffer → *PreLighting* → Lighting (engine placeholder) → *Lighting* → *Denoise*
→ *Post* → Tonemap → UI. Techniques of one stage run in registration order. The reference path tracer mode (F5) runs
no techniques, so the reference stays pure.

**Names to know:** `gbuffer.depth/normal/albedo/material/emissive/motion/entityId` (layout: `ShaderLibrary/GBuffer.slang`),
`scene.color` (RGBA16F radiance in nits; the tonemapper reads it). Every texture is visible in the texture viewer.

**Scene files:** `"techniques": { "RTAmbientOcclusion": { "enabled": true, "params": { "radius": 0.5 } } }`. Param keys
are the labels in lowerCamelCase ("Rays per pixel" → `raysPerPixel`). Loading a scene first returns every technique
to its defaults (disabled), so a scene always looks the same.

**Rules (enforced by the build):** techniques include only this module and what it depends on: never World/EnTT,
Physics or Sandbox. Technique shaders import from `ShaderLibrary/` or their own folder; a shader must not be named
like a ShaderLibrary module. Push constant structs: 4-byte fields, `uint64` addresses and `float4` (no `float3`).

**Depends on:** Core, Graphics/Vulkan, ShaderCompiler, RenderGraph, GpuScene, RayTracing.
**Not responsible for:** UI widgets (UI/TechniquePanel draws the params), the techniques themselves (`Techniques/`).
