# Animation
**Purpose:** pose characters on the CPU: skeletons, clip sampling, the transform hierarchy, skinning matrices.
Character feature track ([[21 Character Implementation Plan]]). No ECS and no Vulkan here: World drives it, the GPU
scene will receive its skinning matrices.
**Owns:** `Skeleton` (per model), `Pose` data, and the user's math in `PoseMath`.

**Public API**
| Type | What it does |
|---|---|
| `Skeleton` | `build(model)`: parents, a parents-first `evaluationOrder()`, rest pose, the model's first skin (a copy), and `referenceRestGlobals()` (cgltf's rest matrices, the test reference for `PoseMath`). `makeRestPose()` |
| `Pose` | `local` (TRS per node), `global` (node → model per node), `skinning` (bind → model per joint) |
| `JointTransform` | translation / rotation (quaternion) / scale |
| `ClipSampler` | `apply(clip, time, pose)`: each channel → `PoseMath::sampleVec3/sampleQuat` → `pose.local`; `wrapTime` (loop / clamp) |
| `PoseMath` | **The user's code** (`// CHAR-TODO` stubs): `sampleVec3`, `sampleQuat`, `localMatrix`, `computeGlobalPose`, `computeSkinningMatrices`. Placeholders hold the rest pose |

**Frame order** (run by `World/Systems/AnimationSystem`): rest pose → `ClipSampler::apply` → `computeGlobalPose` →
`computeSkinningMatrices`.

**Depends on:** Core, Assets.
**Not responsible for:** advancing clip time and reading scene settings (World's `Animator` + `AnimationSystem`), GPU
skinning and acceleration-structure refits (Graphics, later in Stage 1), secondary motion (Stage 2).
