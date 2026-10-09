---
tags: [spec, architecture]
---

# 05 · Architecture

> Folder layout and module rules are in [[14 Project Structure]]. ECS, scenes and sandbox detail is in [[16 Sandbox, ECS and Scenes]].

## One frame, end to end

```
 Input ─▶ Sandbox tools / Player ─▶ ECS World (EnTT) ◀─▶ Physics (Jolt)
                                          │
                                          ▼  extraction (Transform + MeshRenderer + Lights)
                                     GPU Scene ── instance buffer, materials, lights,
                                          │       prev transforms, bindless textures
                                          ▼
                                    TLAS update (BLAS reused for rigid bodies)
                                          │
 ┌──────────────┐   ┌────────────────────────────┐   ┌──────────────┐   ┌─────────┐
 │ Raster       │──▶│ Techniques (MY code)       │──▶│ Denoise /    │──▶│ Tonemap │──▶ swapchain
 │ G-buffer     │   │ shadows, reflections, GI…  │   │ temporal     │   │ + UI    │
 └──────────────┘   └────────────────────────────┘   └──────────────┘   └─────────┘

 Alternative mode: [Reference Path Tracer] → accumulation → tonemap
```

### Frame order
1. **Input** → player controller (fly/walk) and sandbox tools
2. **Physics step** (fixed timestep) → writes `Transform`s of dynamic bodies
3. **ECS systems** (lights, selection, etc.)
4. **GPU scene extraction**: ECS → GPU buffers; stores **previous transforms** for motion vectors
5. **TLAS update**: instance transforms change, BLAS untouched for rigid bodies
6. **Render graph**: G-buffer → techniques → denoise → tonemap → UI → present

## Key boundary
> [!important] Techniques never touch the ECS or physics
> Techniques only see the **GPU scene** (buffers, TLAS, lights, G-buffer) through the [[06 Technique API]]. They work identically whether objects are static, spawned, or flying through the air. That keeps technique code clean and close to the RTG chapters.

## Render modes
1. **Real-time (hybrid):** game-style pipeline from enabled techniques
2. **Reference:** progressive path tracer, accumulates while nothing moves
3. **A/B view:** split screen between any two configurations + error metrics

## Key graphics systems

### Render graph
- Passes declare reads/writes by name (`"gbuffer.normal"`); barriers are automatic.
- Resource kinds: **transient**, **persistent**, **history** (auto ping-pong `prev`/`curr`).
- Per-resource resolution scale (full / half / quarter).
- Every resource is viewable in the texture viewer.

### GPU scene
- Global vertex/index buffers, material buffer, instance buffer via **buffer device address**.
- **Bindless** texture array.
- Light buffer + **emissive triangle list** (for many-lights/ReSTIR), rebuilt when lights are spawned or edited.
- One BLAS per mesh asset (shared by all instances), one TLAS rebuilt/updated per frame.
- Instance masks: opaque, alpha-tested, emissive, sandbox-helper (gizmos excluded from RT).

### G-buffer
| Target | Format |
|---|---|
| Depth | D32 |
| Normals (geometric + shading) | RG16 octahedral ×2 |
| Albedo | RGBA8 sRGB |
| Metal / roughness | RG8 |
| Emissive | RGBA16F |
| Motion vectors (camera **and** object motion) | RG16F |
| Entity ID | R32 (picking, debugging) |

### Reference path tracer harness
- Naive version (cosine diffuse, epsilon offset, no MIS) marked `RTG-TODO`. Upgraded by me in M2.
- Resets accumulation whenever anything moves (camera or physics).
- Can save a converged reference EXR for metrics.

### Tooling
Shader hot-reload with error overlay, auto ImGui technique panels, GPU timers per pass, camera bookmarks + paths, benchmark → CSV, RMSE/relMSE/FLIP metrics, VRAM display, screenshots PNG + EXR.

See also: [[14 Project Structure]], [[16 Sandbox, ECS and Scenes]], [[06 Technique API]]
