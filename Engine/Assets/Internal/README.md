# Assets/Internal
**Purpose:** implementation-only files of the Assets module (other modules may not include them).

| File | Why it's here |
|---|---|
| `ThirdPartyImplementations.cpp` | Compiles cgltf and stb_image (`*_IMPLEMENTATION`) once, as the separate target `Ghost_Assets_Codecs`, optimized even in Debug |
