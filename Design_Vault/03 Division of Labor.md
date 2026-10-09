---
tags: [spec, roles]
---

# 03 · Division of Labor

> [!important] The one rule
> **Claude builds the engine. I build the techniques.**
> If something is listed in [[08 RTG Technique Catalog]], it's mine. Everything else is Claude's.

## 🤖 Claude builds (everything under `Engine/`, plus tooling)
| Area | Includes |
|---|---|
| Setup | `run.bat`, `new_technique.bat`, dependency/tool bootstrap, smoke tests ([[15 Setup and Portability]]) |
| Platform | Window, input, frame timing, main loop |
| Vulkan core | Device, swapchain, memory (VMA), sync, bindless, debug names |
| Ray tracing plumbing | BLAS/TLAS build and update, shader binding table, RT pipelines, ray query |
| Render graph | Passes, transient/persistent/history resources, automatic barriers |
| ECS & scenes | EnTT world, components, systems, JSON scene + prefab loading and saving ([[16 Sandbox, ECS and Scenes]]) |
| Physics | Jolt integration, physics ↔ ECS sync, character controller |
| Sandbox | Spawn menu, physics gun, gizmos, picking, inspector, undo, fly/walk toggle |
| GPU scene | ECS → GPU buffers, TLAS instances, previous-frame transforms (for motion vectors) |
| Base passes | Raster **G-buffer** (incl. motion vectors), tonemap, present |
| Reference harness | Progressive accumulation with a **naive** path tracer that I upgrade |
| Debug tools | Shader hot-reload + error overlay, texture viewer, GPU timers, A/B view, metrics, benchmark, screenshots, camera paths |
| Shader library basics | Scene access, RNG, blue noise lookup, packing, math helpers |
| Docs | Module READMEs, `17 RT Plumbing Explainer` |

## 🧑‍💻 I build (everything under `Techniques/`)
- Everything in [[08 RTG Technique Catalog]]
- The **shader code** where the math lives
- The **technique logic**: passes, order, data
- **Upgrading the naive baselines** (`RTG-TODO` stubs)
- Write-ups, comparisons, videos ([[11 Portfolio Plan]])
- Scene JSON files for my demos (Claude can help)

## 🛡️ "The engine never gets in my way" guarantees
1. **Shader errors never crash the engine.** A compile error shows an on-screen overlay with file/line, and the last working shader keeps running.
2. **GPU hangs are reported**, not silent crashes: which pass was running and what to check.
3. **`run.bat test`** (load every scene, render frames, zero validation errors) passes after every engine change.
4. **`new_technique.bat <Category> <Name>`** generates a ready-to-edit technique folder from a template.
5. **Bad JSON gives a clear error** (file, line, field) and doesn't crash.

## 🧑‍🏫 Tutor mode (when I ask for help with a technique)
Default. I can override per technique by saying **"write it for me"**.
1. Explain the concept and math, pointing to the RTG chapter.
2. Give hints and pseudocode, not the full implementation.
3. Review my code and explain *why* something is a bug.
4. Help debug with engine tools (texture viewer, Nsight).
5. Claude **may** add infrastructure a technique needs (new buffer types, debug views).

## Naive baselines
```cpp
// RTG-TODO(RTG1 ch.6): replace epsilon offset with robust self-intersection avoidance
float3 origin = hitPos + normal * 1e-3;
```
Search for `RTG-TODO` to find them all.

See also: [[06 Technique API]], [[07 Roadmap]]
