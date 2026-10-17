---
tags: [roadmap]
---

# 07 · Roadmap

> [!summary] Shape
> **M0a + M0b** = Claude builds the engine and sandbox. **M1–M7** = I implement techniques; Claude adds infrastructure each milestone needs.

---

## M0a — Renderer Core, ECS & One-Click Setup (Claude)
**Goal:** a fresh clone → `run.bat` → Sponza on screen with ray tracing working end-to-end, so I could already write a technique.

### Checklist
- [x] Folder structure exactly per [[14 Project Structure]], module READMEs, per-module CMake targets
- [x] `run.bat` (`release`, `test`, `clean`), bootstrap into `.tools/`, clear error messages ([[15 Setup and Portability]])
- [x] `new_technique.bat` + `Techniques/_Template/`
- [x] Root-relative path system (`Core`), `User/settings.json` auto-created
- [x] Window, input, fly camera
- [x] Vulkan 1.3 via volk, RT extensions, validation layers from `.tools/`, debug names
- [x] VMA, frames in flight, timeline sync
- [x] Render graph (transient / persistent / history, auto barriers)
- [x] EnTT world + core components (`Transform`, `Name`, `MeshRenderer`, lights, `Camera`)
- [x] JSON **scene loading** (entities, prefabs, lights, environment, technique settings)
- [x] glTF loading → asset registry → GPU scene extraction from ECS
- [x] BLAS per mesh, TLAS per frame, SBT builder, ray query
- [x] G-buffer incl. motion vectors and entity ID
- [x] Placeholder lighting (sun, no shadows) + tonemap + present
- [x] Naive reference path tracer with accumulation (`RTG-TODO` stubs)
- [x] Technique API, auto-registration, auto ImGui params
- [x] Example technique (not RTG): "visualize normals via ray query"
- [x] Slang hot-reload + **error overlay** (last good shader keeps running)
- [x] GPU timers per pass, texture viewer
- [x] Asset manifest + starter assets ([[09 Assets#Starter set]])
- [x] Blue noise textures in `ShaderLibrary`
- [x] Smoke test (`run.bat test`)
- [x] Root README requirements kept accurate, `.gitignore`, git initialized

### Acceptance criteria
- [x] **Portability test:** fresh clone into a path **with spaces**, on a machine **without the Vulkan SDK** → double-click `run.bat` → engine runs. No edits.
- [x] Sponza scene loads from `Content/Scenes/Sponza.scene.json`, 1080p, smooth camera, **zero validation errors**
- [x] Reference mode converges on Cornell box and Sponza
- [x] Breaking a shader shows the error overlay, fixing it recovers, with no restart
- [x] `new_technique.bat Shadows Test` creates a technique that builds and appears in the UI with no other edits
- [x] Every folder has a README, every file a purpose comment, no loose files
- [x] `run.bat test` passes

### Deliverable: `17 RT Plumbing Explainer.md`
≤2 pages on **this engine's** RT plumbing: BLAS vs TLAS (rebuild vs refit, links to code), SBT layout diagram, the lifecycle of one `traceRays`, ray query vs RT pipeline, and 10 likely interview questions with answers.

---

## M0b — Sandbox & Measurement Tools (Claude)
**Goal:** Garry's Mod-style interaction, plus the tools I need for write-ups. Details in [[16 Sandbox, ECS and Scenes]].
- [ ] Jolt physics world, fixed timestep, physics ↔ ECS sync
- [ ] **Prefab** JSON files + prefab instancing (with overrides)
- [ ] Starter prefab set: crate, sphere, barrel, glass sphere, metal sphere, emissive light orb, point/spot light
- [ ] **Spawn menu** (Q)
- [ ] **Physics gun**: grab, move, rotate, throw, freeze
- [ ] **Gizmo tool** (translate / rotate / scale)
- [ ] Picking via TLAS ray query (entity ID)
- [ ] **Inspector** panel: transform, material, light, physics properties live
- [ ] Delete, duplicate, **undo** (Z)
- [ ] **Player toggle (V):** noclip fly ↔ walking character (Jolt CharacterVirtual)
- [ ] Physics pause / step / reset scene
- [ ] **Save scene** (Ctrl+S) back to JSON
- [ ] Dynamic TLAS updates + previous transforms → correct motion vectors for moving objects
- [ ] A/B split view, RMSE/relMSE/FLIP metrics, camera bookmarks + paths, benchmark → CSV, screenshots PNG + EXR

### Acceptance criteria
- [ ] Spawn 100 physics crates in Sponza, still interactive, zero validation errors
- [ ] Grab, throw and freeze objects; motion vectors correct in the texture viewer
- [ ] Spawn a light orb and the lighting updates immediately
- [ ] Save → restart → reload gives an identical scene
- [ ] Walk mode collides with the scene; V toggles back to fly
- [x] `run.bat test` passes

---

> [!important] Technique order (agreed 2026-10-11)
> **M1 → M2 → M4 → M5 → M3** (reflections take the last slot; M6 radiance cache is a stretch goal). Claude builds M0b
> during overnight runs while I work on M1; M0b must land before the M4 demo (live light orbs).
> **Method per technique:** 1) read the chapter, 2) break the algorithm down together (Claude explains, I summarize back),
> 3) I write the code, 4) Claude reviews it and builds tests/comparison tooling, 5) verify in GHOST LIGHT against the
> reference path tracer, 6) write-up + debugging log.

## M1 — Ray Traced Shadows (me)
**Read:** RTG1 ch.6, ch.13 · RTG2 ch.24 (+ ch.4)
- [ ] Robust self-intersection offset (replace the `RTG-TODO`)
- [ ] Hard shadows → soft shadows with blue noise
- [ ] RTG1 ch.13 adaptive tricks
- [ ] Demo: throw objects around and watch shadows update
- [ ] Write-up

## M2 — Reference Path Tracer (me)
**Read:** RTG2 ch.14, ch.20, ch.21 · RTG1 ch.15, ch.16, ch.17
**Claude adds:** Veach MIS scene
- [ ] GGX BRDF + importance sampling, MIS, Russian roulette, firefly handling
- [ ] Validated against Veach MIS scene
- [ ] Write-up

## M3 — Hybrid Pipeline & Reflections (me)
**Read:** RTG1 ch.25, ch.20, ch.21 · RTG2 ch.5–10
- [ ] RT reflections (half-res), ray cones, rough reflections, environment fallback
- [ ] Optional refractions (glass sphere prefab)
- [ ] Write-up

## M4 — Many Lights & ReSTIR (me) ⭐
**Read:** RTG2 ch.21–23 · RTG1 ch.18 · RTG2 ch.47
**Claude adds:** many-light scene, reservoir buffer helpers if needed
- [ ] Uniform → power (alias table) → light BVH sampling
- [ ] RIS → ReSTIR DI (temporal + spatial)
- [ ] Demo: spawn dozens of light orbs live
- [ ] Write-up

## M5 — Denoising (me) ⭐
**Read:** RTG2 ch.25, ch.49 · RTG1 ch.19
- [ ] Temporal reprojection + disocclusion (moving physics objects = stress test)
- [ ] SVGF, applied to shadows / reflections / ReSTIR
- [ ] Write-up

## M6 — Global Illumination & Caching (me)
**Read:** RTG1 ch.24, ch.32, ch.31 · RTG2 ch.41
- [ ] 1-bounce GI → hash-grid radiance cache → multi-bounce
- [ ] Write-up

## M7 — Polish & Portfolio (me + Claude)
**Read:** RTG1 ch.22 · RTG2 ch.46, ch.48
- [ ] TAA, tonemapping/exposure
- [ ] Videos, portfolio page, README hero image

## Optional extras
Caustics · volumes · RT decals · thin lens (see [[08 RTG Technique Catalog]])

## Feature track: playable character (optional, branch `character`)
Separate from the milestones above. Plan and kickoff prompt: [[19 Character Pipeline Prompt]]; branch and merge rules in
[[12 Decisions Log]] (2026-10-17).
- [ ] Phase R: research the Eve (Stellar Blade) pipeline, pick the Tier 1 scope
- [ ] Skinning + BLAS refit (merge stage 1)
- [ ] Verlet hair on spring-bone chains (merge stage 2)
- [ ] Jiggle bones → soft skin → normal recompute (merge stage 3)
- [ ] Skin shading: dual-lobe GGX, transmission → RT SSS + denoise (merge stage 4)
- [ ] Release tag `v1.0-character`

See also: [[08 RTG Technique Catalog]], [[11 Portfolio Plan]]
