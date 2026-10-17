---
tags: [plan, character]
---

# 21 · Character Implementation Plan

> [!summary] How the character feature track gets built
> Scope and target: [[19 Character Pipeline Prompt]] · research: [[20 Eve Pipeline Research]] · branch and merge rules:
> [[12 Decisions Log]] (2026-10-17). Concepts are studied first in [[Study/Character/00 Study Index|the study notes]].

## Work split
**Rule:** I (the user) write every function containing the math. Claude builds everything around it so the math can
run, be seen and be measured. Content is mine; AI tools there are part of the experiment.

| | Me | Claude |
|---|---|---|
| Math (runtime) | Sampling, hierarchy, skinning, Verlet, constraints, collisions, springs, blending, state machine, shading | Derive with me, review, quiz, tests and comparison tools |
| Engine plumbing | Understand and explain it | Import, `Engine/Animation`, skinning pass, deformed buffers, BLAS refit, motion vectors, overlays, panels, smoke tests |
| Content | Style sheet, concept, base mesh, rig, hair cards, textures, animation choice | Tool/licence research, export conventions, import + debugging |
| Experiment | Keep the log (hours, tools, AI share, failures) | Define what to record, build comparison/screenshot tools |
| Write-ups | Write-ups and presentation | Vault, roadmap, decisions log |

**Loop per stage:** study session (concepts → my summary → quiz) → Claude builds scaffold + `// CHAR-TODO` stubs →
I write the math → review + quiz → verify in engine → PR into `main`.

## Frame flow (target)
```
World (CPU, ECS)                                   Graphics (GPU)
1 AnimationSystem    sample clips → local pose      5 Skinning pass   rest verts → deformed verts
2 PoseSystem         local → global → skin mats     6 BLAS refit      per character, every frame
3 SecondaryMotion    hair/jiggle (fixed timestep)   7 TLAS build      (exists)
4 Extraction         skin matrices → GpuScene       8 G-buffer / RT   read the DEFORMED vertices
```
New modules: `Engine/Animation/` (CPU only, Core + Assets), `Graphics/Deformation/` (skinning pass, deformed
buffers), `Techniques/Character/` (skin, suit, hair shading).

## Engine changes (Claude)
| Gap today | Change |
|---|---|
| `NodeData` keeps only mesh nodes, flattened to model space | Full node tree: parent + local TRS |
| No skin/animation data | `SkinData` (joints + inverse binds), `AnimationClip` (channels, times, values) from cgltf |
| `Vertex` fixed 48 B | Separate skin stream (`uvec4 joints, vec4 weights`) |
| All instances read the global vertex buffer | Per-instance vertex address + previous-positions address (`GpuInstance` 240 → 256 B, `Scene.slang` too) |
| One shared compacted BLAS per mesh | Per-character BLAS with `ALLOW_UPDATE`: refit per frame, rebuild every N frames |
| Rigid-only motion vectors | Per-vertex motion vectors from previous deformed positions |
| No debug lines | `DebugDraw` line pass (skeleton, chains, colliders) |
| No skinned smoke test | Skinned test scene, N animated frames, 0 validation errors |

## Stages
### Stage 1: skinning + BLAS refit (CesiumMan sample) → PR 1
- [ ] Study: hierarchy, inverse bind, LBS, normals, slerp/nlerp, refit vs rebuild
- [x] Claude: CesiumMan in manifest, node tree + skin + animation import, `Engine/Animation`, `Animator`, skeleton overlay (F4), stubs in `Engine/Animation/PoseMath.cpp`, smoke-test check (2026-10-17)
- [ ] Me: `sampleChannel`, `localMatrix`, `computeGlobalPose`, skinning matrices, `skinVertex` (Slang)
- [ ] Claude: skin stream, per-instance vertex addresses, skinning pass, BLAS refit + rebuild, per-vertex motion vectors, smoke test
- [ ] Done: CesiumMan walks in Sponza (raster + path traced), F3 motion vectors correct, `run.bat test` passes

### Stage 2: bone chains + Verlet hair → PR 2
- [ ] Study: Verlet from Taylor, PBD constraints, collisions, fixed timestep
- [ ] Decide chain format (proposed: `VRMC_springBone` authored with the VRM add-on for Blender; licences to confirm)
- [ ] Claude: chain/collider import, fixed-timestep loop, overlays, tuning panel
- [ ] Me: Verlet, distance constraints, sphere/capsule collision, chain → bone rotations
- [ ] Done: hair follows motion, collides, same behaviour at 30 and 144 fps

### Stage 3: jiggle bones → PR 3
- [ ] Study: damped harmonic oscillator, critical damping, frame-rate independence
- [ ] Me: damped spring toward the animated pose (same solver framework as stage 2)

### Stage 4: my character in the engine
- [ ] Blender export conventions: metres, Y-up, ≤ 4 influences, triangulated, tangents
- [ ] Rig to the same skeleton as the animation source (no retargeting in Tier 1)
- [ ] Host my own character on GitHub Releases + manifest (one-click stays intact)

### Stage 5: playable (needs the Jolt part of M0b first) → PR
- [ ] Study: rotation blending, 1D blend spaces, crossfades, state machines
- [ ] Claude: Jolt world + `CharacterVirtual`, third-person camera with collision probe
- [ ] Me: walk/run blend, crossfade, idle/walk/run/jump state machine

### Stage 6: skin + suit shading → PR
- [ ] Study: GGX, dual lobe, wrap diffuse, transmission, sheen
- [ ] Claude: shading-model ID in `GpuMaterial` + G-buffer, thickness map slot, `Techniques/Character/` scaffold
- [ ] Me: dual-lobe GGX, wrap diffuse, transmission, suit gloss/sheen (GGX shared with M2)

### Stage 7: bedroom showcase
- [ ] Props (CC0 / AI-generated), window light, bookmarks, style comparison shots (best after M1, M5, M6)

## Reading list
| Stage | Read |
|---|---|
| 1 | glTF 2.0 spec (*Skins*, *Animations*); Gregory, *Game Engine Architecture* (Animation Systems); Jacobson et al., *Skinning: Real-time Shape Deformation* (SIGGRAPH 2014 course); NVIDIA RT best practices (refit vs rebuild) |
| 2–3 | Jakobsen, *Advanced Character Physics* (GDC 2001); Müller et al., *Position Based Dynamics* (2007); Macklin et al., *XPBD* (2016); Holden, *Spring-It-On*; `VRMC_springBone` spec |
| 5 | Gregory (blending, state machines) |
| 6 | Jimenez et al., *Next-Generation Character Rendering* (GDC 2013); Christensen & Burley, *Approximate Reflectance Profiles* (2015); d'Eon & Luebke, *GPU Gems 3* ch. 14; Kajiya-Kay / Marschner (hair) |
| Later | Kavan et al., *Dual Quaternion Skinning* (2007) |

## Presentation material (collect as we go)
Bind pose vs animated · bone-weight heatmap · skeleton overlay · per-vertex motion vectors · refit vs rebuild cost over
time · Verlet with 1/4/10 iterations · hair at 30 vs 144 fps · LBS candy-wrapper · single vs dual-lobe skin · with vs
without transmission · Eve-style trait comparison sheet.

See also: [[07 Roadmap]], [[03 Division of Labor]]
