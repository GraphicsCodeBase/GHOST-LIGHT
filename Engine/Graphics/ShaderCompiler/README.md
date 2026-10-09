# Graphics/ShaderCompiler
**Purpose:** turn Slang source into running pipelines, and keep them running while you edit shaders.
**Owns:** the Slang global session, every `VkPipeline` built from Slang, the list of current shader errors.

**Public API**
| Class | What it does |
|---|---|
| `ShaderCompiler` | `compile(entryPoints)` → SPIR-V 1.6 per entry point, the list of files read (imports included), and the first error as file + line + message. A fresh Slang session per compile, so edited files are always re-read |
| `PipelineLibrary` | `addGraphics(desc)`, `addCompute(desc)`, `addCustom(name, entryPoints, builder)` (ray tracing pipelines later) → a `PipelineHandle`. `pipeline(handle)` gives the current `VkPipeline`. `update(frame)` polls source files 4×/s and rebuilds what changed; **a failed rebuild keeps the last working pipeline** and records a `ShaderError` for the on-screen overlay |
| `GraphicsPipelineDesc` / `ComputePipelineDesc` | Entry points + fixed-function state (formats, depth, cull, blend). Dynamic rendering, dynamic viewport/scissor, no vertex input (vertex pulling) |
| `ShaderEntryPoint` | File + function + stage |

**Shader conventions (Slang)**
- Entry points carry `[shader("vertex" | "fragment" | "compute" | "raygeneration" | ...)]`; names `vertexMain`, `fragmentMain`, `main` (compute), `rayGen`, `miss`, `closestHit`, `anyHit`.
- Column-major matrices (same memory layout as glm), scalar buffer layout (C++ structs match shader structs), debug info in Debug builds.
- `import X;` finds `ShaderLibrary/X.slang` or a file next to the shader.
- Every pipeline uses the one bindless pipeline layout (`vulkan::BindlessDescriptors`): set 0 + 256 bytes of push constants.
- A shader broken **at startup** is logged as an error (fails `run.bat test`); broken **after an edit** is a warning plus the overlay.

**Depends on:** Core, Graphics/Vulkan. Slang (`slang.dll` + `slang-compiler.dll`, copied next to the exe) is private.
**Not responsible for:** drawing the error overlay (UI), deciding which pipelines exist (passes, techniques).
