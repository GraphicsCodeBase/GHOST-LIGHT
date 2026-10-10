# Tests
**Purpose:** the smoke test behind `run.bat test`, the gate every engine change must pass.
**Owns:** `GhostLightSmokeTest.exe` and its log, `Build/SmokeTest.log`.

What it checks (grows with each M0a step):
- the engine initializes with default settings (it never reads or writes `User/settings.json`)
- it runs 240 frames and exits with code 0, resizing the window twice on the way (forces swapchain recreation)
- in Debug, the pinned Vulkan validation layer **is active** (with synchronization validation)
- **shader hot reload**: writes a compute shader to `Build/SmokeTest/`, breaks it (must be reported at line 4 while the
  last working pipeline keeps running), fixes it (must reload)
- **scenes**: the default scene loads its models; every `Content/Scenes/*.scene.json` loads with zero problems; the
  broken files in [`Data/`](Data/README.md) report exact locations and never crash or replace the running scene
- **GPU scene**: after the first frame the default scene's models are on the GPU with instances to draw
- **player**: holding W (injected like a real key event) flies the camera forward
- **techniques**: `RayQueryNormals` and the `_Template` technique are registered; scene-style settings reach their
  params; enabled, their passes run (GPU timings appear); saves `Build/SmokeTest/RayQueryNormals.png`
- **ray tracing**: BLASes and TLAS instances exist for Sponza; the reference path tracer accumulates 50 samples in 50
  still frames of the Cornell box (no spurious restarts) and saves `Build/SmokeTest/PathTracedCornellBox.png`
- **frame capture + GPU timers**: saves `Build/SmokeTest/LastFrame.png` (look at it: Sponza from the Atrium bookmark),
  checks per-pass timings arrive for TLAS, GBuffer, Lighting, Tonemap and UI
- **zero errors logged**, which includes every Vulkan validation error (later: every scene in `Content/Scenes/` loads and renders)

**Depends on:** the `Ghost_App` library (same code as `GhostLight.exe`).
**Not responsible for:** performance measurements (DebugTools benchmark, M0b).
