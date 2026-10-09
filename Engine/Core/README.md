# Core
**Purpose:** the small utilities every other module uses.
**Owns:** the engine log file (`Build/GhostLight.log`), `User/settings.json`, the repo-root location.
**Public API:**
- `Log`: `Log::info/warning/error("x = {}", x)` to console, debugger output and the log file. Counts errors (the smoke test fails on any).
- `GHOST_ASSERT(cond, "message")` (`Assert.h`): always-on check; logs file/line and stops without a blocking dialog.
- `Paths`: finds the repo root (nearest parent of the exe with `run.bat`) and builds every path from it: `content()`, `user()`, `build()`, `tools()`, `shaderLibrary()`, `techniques()`, `resolveContent("Scenes/X.scene.json")`, `display(path)` for messages, UTF-8 conversions.
- `JsonFile`: `load(path)` returns the value (and the text) or an error with file, line and column (comments allowed); `save(path, json)` writes atomically with 2-space indent.
- `JsonReader`: typed field access (`string`, `number`, `vec3`, ...) that never throws; problems become `file(line): field.path: message` (line from nlohmann's diagnostic positions), unknown keys warn.
- `UserSettings`: window size, last scene, camera speed. Created with defaults when missing; a corrupt file is kept as `.bad` and replaced.
- `FrameTimer`: delta time (clamped), smoothed FPS, frame index.

**Depends on:** glm, nlohmann/json (public, since headers expose them).
**Not responsible for:** windows or input (Platform), anything GPU (Graphics), scene files (World/Serialization).
