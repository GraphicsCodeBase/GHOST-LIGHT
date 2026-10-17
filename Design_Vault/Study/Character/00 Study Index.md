---
tags: [study, character, index]
---

# 00 · Character Study Index

> [!info] What these notes are
> Written in the **study session** (branch `character-study`), one note per concept, before the code for it exists.
> The **build session** reads the relevant notes before writing stubs, so the code matches what I learned.
> Plan: [[21 Character Implementation Plan]].

## Note template
```markdown
# <NN> · <Concept>
**Stage:** <1–6>   **Status:** studying | understood | quizzed ✅
## The idea in one paragraph (my words)
## Derivation
## What it becomes in code (function signature I'll write, inputs/outputs, edge cases)
## Pitfalls and debug visualisations
## Quiz questions (and my answers)
## Sources
```

## Stage 1: skinning
- [ ] 01 Transform hierarchy (local → global, parent-before-child order)
- [ ] 02 Inverse bind matrices
- [ ] 03 Keyframe sampling: lerp, slerp vs nlerp, shortest path
- [ ] 04 Linear blend skinning + the candy-wrapper artifact
- [ ] 05 Transforming normals and tangents (inverse-transpose)
- [ ] 06 BLAS refit vs rebuild (concept level)

## Stage 2–3: secondary motion
- [ ] 07 Verlet integration from a Taylor expansion
- [ ] 08 Position-based constraints (distance), iterations vs stiffness
- [ ] 09 Sphere / capsule collision
- [ ] 10 Fixed timestep with an accumulator
- [ ] 11 Damped springs, critical damping

## Stage 5: animation runtime
- [ ] 12 Rotation blending, 1D blend spaces, crossfades, state machines

## Stage 6: shading
- [ ] 13 GGX microfacet BRDF
- [ ] 14 Dual-lobe specular for skin
- [ ] 15 Wrap diffuse + thickness transmission
- [ ] 16 Sheen / gloss for suit materials
