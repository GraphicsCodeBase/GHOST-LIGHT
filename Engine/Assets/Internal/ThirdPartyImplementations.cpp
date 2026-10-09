// The one translation unit that compiles cgltf and stb_image. Built as its own target with optimizations on even in
// Debug, so decoding Sponza's 69 textures doesn't take ages in Debug builds.
#pragma warning(push, 0)
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_GIF
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#include <stb_image.h>
#pragma warning(pop)
