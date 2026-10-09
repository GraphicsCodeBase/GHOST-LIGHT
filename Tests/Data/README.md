# Tests/Data
**Purpose:** deliberately broken input the smoke test feeds the engine, to prove bad files give clear errors and never crash.

| File | Expected result |
|---|---|
| `BrokenSyntax.scene.json` | Parse error reported at line 4; nothing loaded; the running scene stays |
| `BrokenField.scene.json` | Loads the valid entity; reports `BrokenField.scene.json(6): entities[1].transform.position: ...`, the missing model (line 8) and a warning for the misspelled `DirectionalLite` component |

Never "fix" these files: they are broken on purpose.
