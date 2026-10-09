# Graphics/Passes/Shaders
**Purpose:** Slang shaders of the engine's built-in passes. Compiled at runtime and hot-reloaded: edit, save, see the change.

| File | Used by |
|---|---|
| `Fullscreen.slang` | Vertex shader for every full-screen pass (one triangle from `SV_VertexID`) + the `FullscreenVaryings` struct |
| `Background.slang` | `BackgroundPass` compute shader (procedural HDR sky) |
| `Tonemap.slang` | `TonemapPass` fragment shader (exposure + ACES fit + sRGB) |

Shared helpers come from `ShaderLibrary/` (`import Math;`), never by copying code between shader files.
