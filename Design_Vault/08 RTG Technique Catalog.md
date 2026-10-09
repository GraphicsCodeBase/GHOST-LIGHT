---
tags: [catalog, rtg, reference]
---

# 08 · RTG Technique Catalog

> [!important] Everything in this note is MY work
> Claude does not implement these (see [[03 Division of Labor]]). Both books are free PDFs:
> - RTG I: https://www.realtimerendering.com/raytracinggems/rtg/
> - RTG II: https://www.realtimerendering.com/raytracinggems/rtg2/

Legend: 🔴 must · 🟡 high value · 🟢 optional · 📖 read only

---

## 1. Ray basics & robustness → [[07 Roadmap#M1 — Ray Traced Shadows (me)|M1]]
| Ch | Technique | Pri |
|---|---|---|
| RTG1 6 | Fast & robust self-intersection avoidance | 🔴 |
| RTG2 2, 34 | Ray/AABB intersection, numerical precision | 📖 |
| RTG2 3 | Essential ray generation shaders | 🟡 |
| RTG2 15–17 | SBT demystified, Vulkan RT intro, bindless | 📖 (engine plumbing, so understand it) |

**In games:** needed by every shipped RT title. Bindless matters because rays can hit any material.

## 2. Shadows → M1
| Ch | Technique | Pri |
|---|---|---|
| RTG1 13 | RT shadows at real-time frame rates | 🔴 |
| RTG2 24 | Blue noise for RT soft shadows | 🔴 |
| RTG1 12, RTG2 4 | Shadow terminator fixes | 🟡 |

**In games:** among the first RT features to ship (*Shadow of the Tomb Raider*, *CoD: Modern Warfare 2019*).

## 3. Reflections, refractions, texture LOD → M3
| Ch | Technique | Pri |
|---|---|---|
| RTG1 20 | Texture LOD strategies (ray cones / differentials) | 🔴 |
| RTG1 21 | Env-map filtering with ray cones | 🟡 |
| RTG2 5, 6, 7, 10 | Missing derivatives, differential barycentrics, cone gradients, refraction cones | 🟡 |
| RTG2 8, 9 | Reflection/refraction formulas, Schlick Fresnel | 🔴 |
| RTG2 11, 29 | Translucency, hybrid RT + screen-space refractions | 🟢 |
| RTG1 32 | Specular reflections with radiance caching | 🟢 |

**In games:** the most visible RT feature (*Battlefield V*, *Control*, *Spider-Man: Miles Morales*, *Cyberpunk 2077*). Without ray cones, textures in reflections alias.

## 4. Sampling & many lights → M2, M4 ⭐
| Ch | Technique | Pri |
|---|---|---|
| RTG1 15 | On the importance of sampling | 🔴 |
| RTG1 16 | Sample transformations zoo | 🔴 |
| RTG1 17 | Ignoring the inconvenient (firefly clamping, path cutoffs) | 🟡 |
| RTG2 20 | Multiple importance sampling 101 | 🔴 |
| RTG2 21 | Alias method | 🔴 |
| RTG2 22 | Weighted reservoir sampling | 🔴 |
| RTG2 23 | Many lights with grid-based reservoirs | 🔴 |
| RTG1 18 | Importance sampling many lights on the GPU (light BVH) | 🟡 |
| RTG2 47 | Light sampling in *Quake 2 RTX* | 📖🟡 |

**In games:** currently the hottest area. ReSTIR (NVIDIA RTXDI) is what lets *Cyberpunk 2077 RT Overdrive* and *Alan Wake 2* path trace scenes with many lights.

## 5. Denoising & temporal → M5 ⭐
| Ch | Technique | Pri |
|---|---|---|
| RTG2 25 | Temporally reliable motion vectors | 🔴 |
| RTG2 49 | ReBLUR hierarchical recurrent denoiser | 🟡 |
| RTG1 19 | Cinematic RT + denoising in UE4 | 📖 |
| RTG1 22 | TAA with adaptive ray tracing | 🟡 (M7) |
| (external) | SVGF, Schied et al. 2017 | 🔴 |

**In games:** games trace ≤1 sample per pixel, so denoisers make it shippable. NRD (ReBLUR/ReLAX) shipped in *Cyberpunk 2077*, *Dying Light 2*. Newer titles add ML denoising (DLSS Ray Reconstruction).

## 6. Hybrid rendering → M3
| Ch | Technique | Pri |
|---|---|---|
| RTG1 25 | Hybrid rendering for real-time RT (SEED / PICA PICA) | 🔴 **engine blueprint** |
| RTG1 26 | Deferred hybrid path tracing | 🟡 |
| RTG2 26–28 | RT LOD cross-fades, RT decals, billboards/impostors | 🟢 |

## 7. Global illumination & caching → M6
| Ch | Technique | Pri |
|---|---|---|
| RTG1 24 | Real-time GI with photon mapping | 🟡 |
| RTG2 41 | Practical spatial hash map updates | 🔴 (hash-grid radiance cache) |
| RTG1 31 | Variance reduction with path reuse | 🟢 |
| RTG1 23 | Light map / irradiance volume preview in Frostbite | 📖 (RT in tools) |

**In games:** *Metro Exodus Enhanced Edition* RTGI, UE5 Lumen hardware RT mode, hash-grid radiance caches (e.g. NVIDIA SHaRC).

## 8. Volumes, caustics & effects → optional
| Ch | Technique | Pri |
|---|---|---|
| RTG1 14, 30, RTG2 30 | Water caustics, screen-space photon mapping, RT caustics | 🟢 |
| RTG1 28, RTG2 43 | Inhomogeneous volumes, unbiased volume path tracing | 🟢 |
| RTG1 11 | Nested volumes | 🟢 |
| RTG2 31 | Tilt-shift / thin lens | 🟢 |

## 9. Production case studies → read before interviews 📖
- RTG2 ch.46: *Ray Tracing in Control*
- RTG2 ch.48: *Ray Tracing in Fortnite*
- RTG2 ch.50: RT content compatibility in UE4
- RTG2 ch.38: CPU performance in DXR

## Skipped (not game-relevant)
Scientific visualization (RTG1 27, 29; RTG2 44, 45) · spectral rendering (RTG2 13, 42) · fractals/blobbies (RTG2 33, 35) · WebRays (RTG2 18) · planetarium camera (RTG1 4)

## External references worth adding later
- ReSTIR DI paper (Bitterli et al. 2020), ReSTIR GI (Ouyang et al. 2021)
- SVGF (Schied et al. 2017)
- *Physically Based Rendering* (PBRT 4th ed., free online)

See also: [[07 Roadmap]], [[11 Portfolio Plan]]
