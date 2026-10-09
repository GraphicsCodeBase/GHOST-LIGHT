# Graphics/Passes/Shaders
**Purpose:** Slang shaders of the engine's built-in passes. Compiled at runtime and hot-reloaded: edit, save, see the change.

| File | Used by |
|---|---|
| `Fullscreen.slang` | Vertex shader for every full-screen pass (one triangle from `SV_VertexID`) + the `FullscreenVaryings` struct |
| `GBufferFill.slang` | `GBufferPass`: vertex pulling (`SV_VulkanVertexID` / `SV_VulkanInstanceID`), normal mapping, material overrides, motion vectors, entity IDs |
| `DeferredLighting.slang` | `LightingPass` compute shader (placeholder Lambert lighting) |
| `Tonemap.slang` | `TonemapPass` fragment shader (exposure + ACES fit + sRGB) |

Shared helpers come from `ShaderLibrary/` (`import Scene;`), never by copying code between shader files.

> [!warning] File names must not match a ShaderLibrary module
> `import X;` looks next to the importing file first. A pass shader called `GBuffer.slang` would shadow
> `ShaderLibrary/GBuffer.slang` for every shader in this folder (that's why the G-buffer shader is `GBufferFill.slang`).
