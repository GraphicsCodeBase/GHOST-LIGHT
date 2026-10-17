---
tags: [prompt, kickoff, character]
---

# 19 · Character Pipeline Prompt

> [!info] How to use
> Kickoff prompt for the **"Solo + AI vs Studio"** character pipeline project. Open a new Claude Code session in the
> repo root and paste the code block below. Drafted 2026-10-17 from a first prompt about Verlet hair + skin shading.

> [!success] Decided 2026-10-17 (see [[12 Decisions Log]])
> - An **optional feature track** in the same repo, separate from the RTG roadmap: branch **`character`**, code in
>   `Engine/Animation/` and `Techniques/Character/`, its own `CharacterShowcase` scene. Listed in [[07 Roadmap]].
> - Merged into `main` **in stages via pull requests**, each passing `run.bat test` with existing scenes unchanged and
>   unfinished parts off by default. Finish = delete the branch, tag `v1.0-character`.
> - Runs alongside M1 → M2 → M4 → M5 → M3; character shading can reuse M1 shadows, M2 GGX and M5 denoising.

```markdown
# Project: "Solo + AI vs Studio" — a playable hero character pipeline

## Context
I have a C++20 / Vulkan hardware ray tracing engine (GHOST LIGHT) built with AI assistance.
Read CLAUDE.md and Design_Vault/19 Character Pipeline Prompt.md first.
I want to study the production pipeline behind Eve (Stellar Blade, Shift Up, UE4) from concept to
playable character, then rebuild that pipeline as a solo developer with AI help, with an ORIGINAL
character (not Eve, no ripped assets, no real-person likeness).

## The question I'm answering
How close can one programmer + AI get to a studio-quality playable character, and which pipeline
stages can AI automate, assist, or not help with at all? Every stage is measured.

## Phase R: Research (no code)
Produce a pipeline map of Eve's creation, one section per stage:
concept → scan/sculpt → retopo/UV/LOD → texturing → hair cards → rigging → face/FACS →
mocap/animation → runtime animation → secondary motion → shading → integration/perf → controller.
For each stage: what the studio did (sources cited; mark confirmed vs inferred), typical studio
tools and team effort, the solo + AI equivalent (specific tools, license terms for portfolio use),
and whether the stage is "I write the math" or "AI-assisted content".

## Rules
- Runtime/graphics stages (skinning, physics, shading, animation runtime): derive the math first
  (Verlet from Taylor expansion, constraint projection, slerp, LBS, GGX, diffusion profiles).
  I write every function containing core math; AI writes Vulkan/engine boilerplate, reviews my
  code and quizzes me with 2–3 questions afterwards.
- Content stages: AI tools are allowed and are the experiment. Log them.
- Per stage, keep an experiment log: hours, tools, % AI vs manual, cost, license, quality
  vs the studio reference (side-by-side), what failed.
- Keep a running list of debug visualizations and before/after comparisons for my presentation.
- Engine rules from GHOST LIGHT's CLAUDE.md still apply (portability, RTX 20-series minimum,
  no Blackwell-only features, run.bat test passes).

## Scope tiers ("playable")
- Tier 1: idle/walk/run/jump with blending, Verlet hair + jiggle, skin shading, in-engine, third-person camera.
- Tier 2: small combat moveset, facial expressions, cloth on costume pieces.
- Tier 3: AAA polish (stiffness falloff, angle limits, inertia scaling, corrective blendshapes, LODs, perf budget).

## Runtime technique plan (from my original prompt, kept)
Skinning compute + BLAS refit → Verlet hair on spring-bone chains → jiggle bones → per-vertex
soft skin + normal recompute → dual-lobe GGX + transmission → RT SSS (Burley) + temporal denoise.

## Start
Work on the `character` branch (decisions in the vault note are already made). Do Phase R and present the
pipeline map. Then propose which stages are realistic for Tier 1, how they map onto GHOST LIGHT's
roadmap, and a concrete first build step.
```

> [!info] Phase R result: [[20 Eve Pipeline Research]]

## Notes carried over from the drafting session
- **Engine today:** no skinning/animation import (cgltf can parse it), hybrid renderer, one shared compacted BLAS per
  mesh with no refit, TLAS rebuilt every frame, motion vectors for rigid motion only. Characters need per-instance
  deformed + previous-position buffers and their own `ALLOW_UPDATE` BLAS.
- **Hair:** RT curve primitives are Blackwell-only, so hair stays alpha-tested cards (any-hit cost worth measuring).
- **Normal recompute:** prefer a gather over precomputed vertex→triangle adjacency to float atomics.
- **GPU soft-skin constraints** run Jacobi-style (averaged) or need graph colouring for Gauss-Seidel.
- **VRM spring bones** are inertia + stiffness + gravity + one length renormalisation; implement that baseline first,
  then upgrade to iterated PBD distance constraints (good before/after).
- **Assets:** Mixamo needs an Adobe login, ships FBX and can't be mirrored for `run.bat`. VRM models carry their own
  license field; VRM 0.x and 1.0 spring-bone extensions differ. Check current MetaHuman / CC4 export terms.

See also: [[02 Goals and Context]], [[07 Roadmap]], [[11 Portfolio Plan]], [[13 Open Questions]]
