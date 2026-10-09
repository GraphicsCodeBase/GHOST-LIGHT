# Content
**Purpose:** everything the engine loads at runtime. Paths inside scene and prefab files are relative to this folder.

| Path | Contents | In git? |
|---|---|---|
| [`Scenes/`](Scenes/README.md) | `*.scene.json` | yes |
| [`Prefabs/`](Prefabs/README.md) | `*.prefab.json`, grouped by category | yes |
| `AssetManifest.json` | Every downloadable asset: URL(s), SHA-256, size, license, attribution, which scenes need it | yes |
| `Assets/` | Downloaded models, textures, HDRIs (`run.bat` fetches them) | **no**, except [`Assets/CREDITS.md`](Assets/CREDITS.md) |

Adding an asset: add an entry to `AssetManifest.json` (single `url` + `sha256`, or `baseUrl` + `files` for a glTF with
many textures), list it in `Assets/CREDITS.md`, run `run.bat`.
