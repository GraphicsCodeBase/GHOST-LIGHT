# App
**Purpose:** wires the modules together and runs the frame loop; builds `GhostLight.exe`.
**Owns:** startup order, the main loop, global shortcuts (F11 fullscreen), saving user settings on exit.
**Public API:**
- `Engine`: `initialize()` (paths → log → settings → window; returns false with a logged reason), `run()` (returns the exit code), `framesRun()`.
- `EngineOptions`: max frames, whether to use `User/settings.json`, scene, log file name, window title.
- `CommandLine::parse()`: `--scene <path>`, `--frames <n>`, `--help`.
- `Main.cpp`: the exe entry point (not part of the `Ghost_App` library, so the smoke test can link the library).

**Depends on:** Core, Platform (more modules join as M0a progresses).
**Not responsible for:** rendering details (Graphics), entities (World), the smoke test itself (`Tests/`).
