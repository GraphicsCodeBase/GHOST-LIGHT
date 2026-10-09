# Sandbox
**Purpose:** the Garry's Mod-style layer: how the user moves around and interacts with the world.
Not a module itself: each subfolder is one module.

| Submodule | Purpose | Status |
|---|---|---|
| [Player](Player/README.md) | `FlyController` (free camera); walk mode + mode toggle with physics | fly ✅ (M0a), walk M0b |
| Tools | Physics gun, gizmo, spawn, delete | M0b |
| Selection | Picking (TLAS ray query / `gbuffer.entityId`), selection state | M0b |
| Undo | Undo stack | M0b |

See [[16 Sandbox, ECS and Scenes]].
