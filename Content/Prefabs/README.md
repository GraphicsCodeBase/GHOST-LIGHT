# Content/Prefabs
**Purpose:** reusable entity templates (`*.prefab.json`): a set of components a scene entity can start from
(`"prefab": "Prefabs/Props/X.prefab.json"`), then adjust with `components`, `transform` and `overrides`.

| Folder | Contents |
|---|---|
| [`Static/`](Static/README.md) | Whole environments (Sponza) |
| [`Props/`](Props/README.md) | Objects (Damaged Helmet; crates, barrels, spheres arrive with the sandbox in M0b) |
| `Lights/`, `Shapes/` | M0b |

Format: `{ "version": 1, "name": "...", "category": "Props", "components": { "MeshRenderer": { "model": "..." } } }`.
