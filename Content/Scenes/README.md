# Content/Scenes
**Purpose:** scene files (`*.scene.json`). Every file here must load with zero problems: `run.bat test` loads them all.
Edit a scene while the engine runs and it reloads automatically; mistakes show up on screen with file and line.

| Scene | Shows |
|---|---|
| `Sponza.scene.json` | Default scene: Crytek Sponza, sun, HDRI sky, Damaged Helmet |
| `CornellBox.scene.json` | Procedural Cornell box for path tracer validation |

## Format (version 1)
```jsonc
{
  "version": 1,
  "name": "Sponza",
  "environment": { "hdri": "Assets/HDRI/x.hdr", "intensity": 1.0 },   // optional; no HDRI = procedural sky
  "exposure": { "ev100": 14.0 },                                       // camera exposure (sunlit outdoor ~14-15)
  "player": { "mode": "Fly", "start": "Atrium" },                      // "Fly" | "Walk" (M0b); start = bookmark name
  "cameraBookmarks": [ { "name": "Atrium", "position": [-9, 1.7, -0.3], "yawPitch": [90, 3] } ], // yaw 0 = -Z, 90 = +X
  "entities": [
    { "name": "Sponza", "prefab": "Prefabs/Static/Sponza.prefab.json" },
    { "name": "Sun", "components": { "DirectionalLight": { "direction": [-0.25, -1, 0.18], "illuminance": 100000 } } },
    { "name": "Helmet", "prefab": "Prefabs/Props/DamagedHelmet.prefab.json",
      "transform": { "position": [-4, 1.4, -0.3], "rotationEuler": [0, -90, 0], "scale": 0.6 },
      "overrides": { "MaterialOverride": { "baseColor": [0.8, 0.1, 0.1] } } }
  ],
  "techniques": { "RayTracedShadows": { "enabled": true, "params": { "softness": 0.5 } } }
}
```
Units: meters, degrees, lux (sun), candela (point/spot). `//` comments are allowed. Components: see
[World/Components](../../Engine/World/Components/README.md).
