# Tests
**Purpose:** the smoke test behind `run.bat test`, the gate every engine change must pass.
**Owns:** `GhostLightSmokeTest.exe` and its log, `Build/SmokeTest.log`.

What it checks (grows with each M0a step):
- the engine initializes with default settings (it never reads or writes `User/settings.json`)
- it runs 60 frames and exits with code 0, resizing the window twice on the way (forces swapchain recreation)
- in Debug, the pinned Vulkan validation layer **is active** (with synchronization validation)
- **shader hot reload**: writes a compute shader to `Build/SmokeTest/`, breaks it (must be reported at line 4 while the
  last working pipeline keeps running), fixes it (must reload)
- **scenes**: the default scene loads its models; every `Content/Scenes/*.scene.json` loads with zero problems; the
  broken files in [`Data/`](Data/README.md) report exact locations and never crash or replace the running scene
- **frame capture + GPU timers**: saves `Build/SmokeTest/LastFrame.png`, checks per-pass timings arrive
- **zero errors logged**, which includes every Vulkan validation error (later: every scene in `Content/Scenes/` loads and renders)

**Depends on:** the `Ghost_App` library (same code as `GhostLight.exe`).
**Not responsible for:** performance measurements (DebugTools benchmark, M0b).
