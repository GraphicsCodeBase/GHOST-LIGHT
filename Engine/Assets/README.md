# Assets
**Purpose:** turn files into CPU-side data: glTF models, images, HDR environments, procedural shapes. **No Vulkan here**
(the GPU scene uploads what this module produces).
**Owns:** loaded `ModelData` / `ImageData`, cached by the reference scenes use.

**Public API**
| Type | What it does |
|---|---|
| `AssetRegistry` | `loadModel("Assets/Models/X/X.gltf" or "procedural:CornellBox")` → `ModelHandle` (cached; failures not cached), `loadEnvironment("Assets/HDRI/x.hdr")`, `revision()` |
| `GltfImporter` | glTF 2.0 (`.gltf` + `.bin` + images, or `.glb`) via cgltf: one vertex/index array, primitives, meshes, nodes, metallic-roughness materials (+ emissive strength, transmission), images decoded in parallel. Missing normals/tangents are generated |
| `ImageLoader` | PNG/JPG → RGBA8 (sRGB or linear), `.hdr` → RGBA32F (stb_image) |
| `ProceduralMeshes` | `procedural:CornellBox` (2 m box, classic colors, 17/12/4 ceiling light), `Cube`, `Sphere`, `Plane` |
| `ModelData`, `ImageData` | The data layout. `Vertex` (48 bytes: position, normal, tangent+sign, uv) is shared with shaders |

**Performance:** built optimized even in Debug (`OPTIMIZE_IN_DEBUG`); Sponza (262k triangles, 69 textures) imports in ~0.3 s.
Files are read in one block per file (reading through stream iterators made the parallel decode ~25× slower).

**Depends on:** Core. cgltf and stb_image are private (implementations in `Internal/`).
**Not responsible for:** GPU upload and mipmaps (GpuScene), downloading files (`run.bat` + `Content/AssetManifest.json`),
BC7 compression (deferred, see [[12 Decisions Log]]).
