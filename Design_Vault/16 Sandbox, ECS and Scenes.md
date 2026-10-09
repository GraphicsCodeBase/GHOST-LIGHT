---
tags: [spec, ecs, sandbox, scenes, physics]
---

# 16 · Sandbox, ECS and Scenes

> [!goal] Specs 3, 4, 5
> Scenes in JSON files. Garry's Mod-style interaction. Everything is an entity in an EnTT ECS.

## ECS (EnTT)

### Components
| Component | Data |
|---|---|
| `Name` | display name |
| `Transform` | position, rotation (quat), scale; optional parent |
| `MeshRenderer` | model asset handle, visible flag |
| `MaterialOverride` | base color, roughness, metallic, emissive, transmission |
| `DirectionalLight` | direction, illuminance, color, angular size |
| `PointLight` / `SpotLight` | intensity, color, radius, cone angles |
| `RigidBody` | motion type (Static / Dynamic / Kinematic), mass, friction, restitution |
| `Collider` | shape: Box / Sphere / Capsule / ConvexHull / Mesh (static only) |
| `Grabbable` | can the physics gun pick it up |
| `Frozen` (tag) | physics-gun frozen |
| `PrefabInstance` | source prefab path (for saving) |
| `Camera` | fov, near/far |
| `PlayerController` | mode (Fly / Walk), speed |
| `Selected` (tag) | currently selected in the sandbox |

### Systems (run order each frame)
1. `PlayerSystem`: fly/walk movement
2. `SandboxToolSystem`: physics gun, gizmo, spawn, delete
3. `PhysicsSystem`: Jolt fixed step → writes `Transform` for dynamic bodies
4. `TransformSystem`: world matrices, hierarchy
5. `LightSystem`: flags light changes for the GPU scene
6. `GpuSceneExtractionSystem`: ECS → GPU scene (see [[05 Architecture]])

## Scene files: `Content/Scenes/*.scene.json`
```json
{
  "version": 1,
  "name": "Sponza Shadows Demo",
  "environment": { "hdri": "Assets/HDRI/kloofendal_48d_partly_cloudy.hdr", "intensity": 1.0 },
  "player": { "mode": "Fly", "start": "Entrance" },
  "cameraBookmarks": [
    { "name": "Entrance", "position": [-10, 2, 0], "yawPitch": [90, -5] }
  ],
  "entities": [
    { "name": "Sponza", "prefab": "Prefabs/Static/Sponza.prefab.json" },
    { "name": "Sun", "components": {
        "DirectionalLight": { "direction": [-0.3, -1, 0.2], "illuminance": 100000, "angularSizeDeg": 0.53 } } },
    { "name": "Crate_01", "prefab": "Prefabs/Props/Crate.prefab.json",
      "transform": { "position": [0, 5, 0], "rotationEuler": [0, 45, 0] },
      "overrides": { "MaterialOverride": { "baseColor": [0.8, 0.1, 0.1] } } }
  ],
  "techniques": {
    "RayTracedShadows": { "enabled": true, "params": { "softness": 0.5 } }
  }
}
```
- **Hot reload:** editing the file updates the running scene.
- **Save (Ctrl+S)** writes the current world back, including spawned objects and technique settings.
- Clear error with file/line/field on bad JSON. Never crashes.

## Prefabs: `Content/Prefabs/<Group>/*.prefab.json`
```json
{
  "version": 1,
  "name": "Crate",
  "category": "Props",
  "components": {
    "MeshRenderer": { "model": "Assets/Models/Crate/Crate.glb" },
    "RigidBody":    { "motion": "Dynamic", "mass": 20, "friction": 0.6, "restitution": 0.1 },
    "Collider":     { "shape": "Box", "fitToMesh": true },
    "Grabbable":    {}
  }
}
```
Starter prefabs (M0b): Crate, Barrel, Sphere, **GlassSphere**, **MetalSphere**, **LightOrb** (emissive + point light), PointLight, SpotLight, and static scene prefabs. Each is chosen to show off a technique.

## Sandbox tools (Garry's Mod-style)
| Input | Action |
|---|---|
| **Q** (hold) | Spawn menu: prefabs by category, click to spawn at crosshair |
| **1** | **Physics gun**: LMB grab · mouse wheel = distance · **E** + mouse = rotate · RMB while holding = **freeze** · release = throw with momentum |
| **2** | **Gizmo tool**: click to select · W/E/R = translate/rotate/scale (ImGuizmo) |
| **3** | **Delete tool** |
| **Ctrl+D** | Duplicate selected |
| **Z** | Undo last spawn/delete/transform |
| **Tab** | Toggle mouse cursor ↔ camera control |
| **F1** | Inspector panel: transform, material, light, physics properties of the selected entity, edited live |
| **P** / **O** | Pause / single-step physics |
| **Ctrl+R** | Reset scene to file state |
| **Ctrl+S** | Save scene |

**Picking** uses a ray query against the TLAS (entity ID), so it's render-accurate and itself a small RT feature.

## Player modes (toggle with **V**)
| Mode | Behavior |
|---|---|
| **Fly (noclip)** | Free camera, passes through geometry, adjustable speed. Default |
| **Walk** | Jolt `CharacterVirtual`: gravity, collision, jump (Space), sprint (Shift) |

## Physics (Jolt)
- Fixed timestep (60 Hz), interpolated transforms for rendering.
- Static scene geometry → mesh colliders. Dynamic objects → simple or convex shapes.
- Physics **never** blocks rendering, and physics errors never crash the engine.
- Moving objects → TLAS instance updates + previous transforms → correct **motion vectors**, which is critical for M5 denoising.

## The sandbox fence (out of scope)
No gameplay rules, scripting, NPCs, weapons, ropes/constraints, destruction, audio, or networking. **The sandbox exists to show off rendering.**

See also: [[05 Architecture]], [[07 Roadmap#M0b — Sandbox & Measurement Tools (Claude)]]
