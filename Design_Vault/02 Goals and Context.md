---
tags: [spec, goals]
---

# 02 · Goals and Context

## The goal
Get hired as a **graphics programmer** at a game studio, using a portfolio that demonstrates real-time ray tracing techniques from *Ray Tracing Gems I* (2019) and *Ray Tracing Gems II* (2021).

## What the portfolio must prove
1. **I understand the math and algorithms**: sampling, BRDFs, reservoirs, denoising, caching.
2. **I can implement them on real GPU hardware**: Vulkan RT, measured in milliseconds.
3. **I think like a game studio**: hybrid rendering, frame budgets, temporal stability, **dynamic interactive scenes**.
4. **I can communicate**: clear write-ups, before/after comparisons, honest tradeoffs.

## My core specs
| # | Spec | Detail in |
|---|---|---|
| 1 | **One-click setup**: double-click `run.bat` | [[15 Setup and Portability]] |
| 2 | **I focus on techniques**: the engine is reliable and stays out of my way | [[03 Division of Labor]] |
| 3 | **Scenes defined in JSON** | [[16 Sandbox, ECS and Scenes]] |
| 4 | **Garry's Mod-style sandbox**: spawn prefabs, physics gun, fly/walk toggle | [[16 Sandbox, ECS and Scenes]] |
| 5 | **ECS** (EnTT) | [[16 Sandbox, ECS and Scenes]] |
| 6 | **Portable**: clone on any RTX PC, no config | [[15 Setup and Portability]] |
| 7 | **Clean structure**: clear module folders, no loose files | [[14 Project Structure]] |

## Guiding principles
- **Techniques over plumbing.** My time goes into RTG techniques. Claude builds the engine.
- **But I can explain the plumbing.** BLAS/TLAS, the SBT, one `traceRays` call → `17 RT Plumbing Explainer` (written after M0a).
- **Hybrid, like real games.** Raster G-buffer, RT effects, denoise. The path tracer is a **reference**.
- **The sandbox serves the rendering.** Moving/spawned objects stress TLAS updates, motion vectors and denoisers, and make strong demo clips.
- **Everything measurable and toggleable.**

## Who I am (for Claude's context)
- Learned C++ at university. Comfortable with C++, still learning Vulkan.
- Prefers algorithms and math over low-level API work.
- Dev machine: **RTX 2070 Super** (8 GB VRAM), Windows 11. Any machine I use will have an **RTX GPU**.

## In scope
- Rendering (the focus), ECS, rigid-body physics (Jolt), sandbox tools, JSON scenes/prefabs, debug UI

## Out of scope (the sandbox fence)
- Gameplay rules, scripting languages, AI/NPCs, audio, networking
- Skeletal animation (revisit for M5 denoising demos, see [[13 Open Questions]])
- A full editor (ImGui panels + gizmos only)
- Non-Windows platforms, non-RTX GPUs
- Distributing prebuilt builds (later, see [[13 Open Questions]])

See also: [[03 Division of Labor]], [[11 Portfolio Plan]]
