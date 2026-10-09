# World/Components
**Purpose:** the data entities are made of. Plain structs, no logic; one per file. Namespace `ghost::world`.

| Component | Data | Scene JSON key |
|---|---|---|
| `Name` | display name | entity `"name"` |
| `Transform` | position (m), rotation (quat), scale, parent | entity `"transform"`: `position`, `rotationEuler` (degrees, pitch/yaw/roll) or `rotation` (quat xyzw), `scale` (number or [x,y,z]) |
| `WorldTransform` | this frame's and last frame's local-to-world matrix | (computed by `TransformSystem`) |
| `MeshRenderer` | model reference + loaded handle, visible | `"MeshRenderer": { "model": "Assets/Models/X/X.gltf" \| "procedural:CornellBox", "visible": true }` |
| `MaterialOverride` | optional base color, roughness, metallic, emissive, transmission | `"MaterialOverride": { "baseColor": [r,g,b], ... }` or entity `"overrides"` |
| `DirectionalLight` | direction, illuminance (lux), color, angular size | `"DirectionalLight": { "direction": [x,y,z], "illuminance": 100000, ... }` |
| `PointLight` | color, intensity (candela), radius, range | `"PointLight": { ... }` |
| `SpotLight` | color, intensity, radius, inner/outer cone (deg), range | `"SpotLight": { ... }` |
| `Camera` | vertical FOV, near plane (far = infinity) | `"Camera": { "fovYDeg": 60 }` |
| `PrefabInstance` | source prefab path | entity `"prefab"` |

Physics components (`RigidBody`, `Collider`, `Grabbable`, `Frozen`) arrive in M0b; until then the loader warns and ignores them.
