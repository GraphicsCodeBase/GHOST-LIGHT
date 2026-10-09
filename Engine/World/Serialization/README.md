# World/Serialization
**Purpose:** scene and prefab JSON → entities. Never throws, never crashes on bad input.

| File | Role |
|---|---|
| `SceneLoader` | Reads a scene (settings + entities); per entity applies prefab → `components` → `transform` → `overrides`. Collects every problem as `file(line): field.path: message`; a broken entity is skipped, an unreadable/unparsable file loads nothing |
| `ComponentReader` | One reader per component; unknown keys warn (typos), wrong types error, unknown components warn with the list of known ones. Sandbox components (`RigidBody`, ...) warn "arrives in M0b" |

Line numbers come from nlohmann/json's `JSON_DIAGNOSTIC_POSITIONS` (enabled for the whole build). File format: see
[Content/Scenes/README.md](../../../Content/Scenes/README.md). Saving (Ctrl+S) arrives in M0b.

**Depends on:** Core (`JsonFile`, `JsonReader`, `Paths`), Assets, World components.
