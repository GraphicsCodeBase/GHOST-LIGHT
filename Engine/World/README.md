# World
**Purpose:** the ECS: every object in a scene is an EnTT entity with components. Loads scenes and prefabs from JSON.
**Owns:** the `entt::registry` of the current scene and its `SceneSettings`.

**Public API**
| Type | What it does |
|---|---|
| `World` | `loadScene("Scenes/X.scene.json", assets)` (into a fresh registry, swapped in only if the file was readable), `reloadIfChanged()` (scene or prefab edited on disk → reload, twice a second), `update()` (systems), `registry()`, `settings()`, `sceneRevision()` |
| `SceneSettings` | name, environment HDRI + intensity, exposure (EV100), player mode/start, camera bookmarks, raw `techniques` block |
| [`Components/`](Components/README.md) | `Name`, `Transform`, `WorldTransform`, `MeshRenderer`, `MaterialOverride`, lights, `Camera`, `PrefabInstance` |
| [`Systems/`](Systems/README.md) | `TransformSystem`, `GpuSceneExtractionSystem` (more in M0b) |
| [`Serialization/`](Serialization/README.md) | `SceneLoader`, `ComponentReader` |

Namespace: everything in this module is `ghost::world` (subfolders organize files; they are not separate modules).

**Depends on:** Core, Assets, Graphics/GpuScene (the extraction system pushes plain data into it), EnTT (public).
**Not responsible for:** rendering (Graphics), physics (Physics, M0b), saving scenes (M0b), the asset files themselves (Assets).
