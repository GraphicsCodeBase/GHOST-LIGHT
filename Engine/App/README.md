# App
**Purpose:** wires the modules together and runs the frame loop; builds `GhostLight.exe`.
**Owns:** startup order, the main loop, global shortcuts (F11 fullscreen, F12 screenshot), saving user settings on exit.

**Public API**
- `Engine`: `initialize()` (paths → log → settings → window → renderer → UI → assets/world → scene; returns false with
  a logged reason), `run()` (returns the exit code), `loadScene(path)` (also puts the player at the scene's start
  bookmark), accessors `window()`, `renderer()`, `world()`, `assets()`, `player()`.
- `EngineOptions`: max frames, whether to use `User/settings.json`, scene, log file name, window title, validation,
  and an `onFrame` hook (the smoke test drives the engine through it).
- `CommandLine::parse()`: `--scene <path>`, `--frames <n>`, `--validation`, `--no-validation`, `--help`.
- `Main.cpp`: the exe entry point (not part of the `Ghost_App` library, so the smoke test can link the library).

Frame loop: poll events → scene hot reload → `World::update()` (transforms) → `FlyController::update()` →
`GpuSceneExtractionSystem::update()` + camera + exposure (`1 / (1.2 · 2^EV100)`) → UI → `Renderer::renderFrame()`.

**Depends on:** Core, Platform, Assets, World, Graphics/Renderer, UI, DebugTools, Sandbox/Player.
**Not responsible for:** rendering details (Graphics), entities (World), the smoke test itself (`Tests/`).
