---
tags: [writeup]
technique: RayQueryNormals
category: Examples
chapters: none (engine example)
milestone: M0a
status: done
---

# Ray query normals

> **One-sentence pitch:** the smallest possible ray tracing technique: proves the TLAS, inline ray queries and the
> technique API work, and doubles as a debug view of the geometry the rays actually see.

## Source
- Not an RTG technique. Vulkan `VK_KHR_ray_query` / HLSL-style `RayQuery` (DXR 1.1 inline ray tracing).

## The idea (plain language)
For every pixel, shoot a ray from the camera through the pixel into the scene's top-level acceleration structure and
paint the normal of whatever it hits (x, y, z mapped to red, green, blue). Unlike the G-buffer normals, these come from
ray traversal, so mismatches between the raster and ray traced scene show up immediately.

## Implementation in this engine
- Passes: one compute pass, stage `Post` (runs after lighting, overwrites `scene.color` by `Blend`).
- Resources: `scene.color` (read-write storage). No textures of its own.
- Shader: `Shaders/RayQueryNormals.slang`. `RayQuery<RAY_FLAG_NONE>` + a `Proceed()` loop that commits alpha-masked
  candidates only where `alphaTestFails()` says the texel is solid; `loadHitSurface()` rebuilds the hit from the
  scene buffers.
- Params: **Blend** (0 = lit image, 1 = normals), **Geometric normals** (triangle vs interpolated vertex normals),
  **Alpha test** (off = leaves and fences are solid quads).

## Results
Enable "Ray query normals" in the Techniques panel (F2). The smoke test saves `Build/SmokeTest/RayQueryNormals.png`.

## Limitations & what I'd do next
- Debug colors are divided by the exposure so the ACES curve shows them roughly as authored; not color exact.
