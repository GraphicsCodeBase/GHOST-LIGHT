# Techniques
**Purpose:** the ray tracing techniques from *Ray Tracing Gems* I & II: the user's side of the [[03 Division of Labor]].
One folder per technique, grouped by category ([[08 RTG Technique Catalog]]).

| Folder | What it is |
|---|---|
| `_Template/` | Copied by `new_technique.bat`. Compiled into the smoke test only, so it always builds and runs |
| `Examples/RayQueryNormals/` | Engine example (not RTG): camera rays via an inline ray query, normals as colors |
| `Shadows/`, `Sampling/`, `Reflections/`, `ManyLights/`, `Denoising/`, `GlobalIllumination/` | Created as techniques arrive |

Start a technique: `new_technique.bat Shadows RayTracedShadows`, then `run.bat`, then enable it in the Techniques panel
(F2). A technique folder holds `<Name>.cpp` (passes and params), `Shaders/` and `README.md` (the write-up).

**API:** [Graphics/TechniqueRuntime](../Engine/Graphics/TechniqueRuntime/README.md) and [[06 Technique API]].
**Build:** every `.cpp` here (except `_Template/`) goes into the `Ghost_Techniques` object library, linked into
`GhostLight.exe` and the smoke test. Includes are checked at configure time: techniques may use the technique runtime
(and the Graphics modules below it) and `ShaderLibrary/`, never World/EnTT, Physics or Sandbox.
