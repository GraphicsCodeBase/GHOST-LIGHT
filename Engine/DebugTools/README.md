# DebugTools
**Purpose:** tools for inspecting and documenting what the renderer does.
**Owns:** files written to `Captures/` (gitignored).

| Class | What it does | Status |
|---|---|---|
| `FrameCapture` | **F12** saves the next frame (UI included) to `Captures/GhostLight_<date>_<time>.png`; `requestPng(renderer, path)` for tools/tests | ✅ |
| `PngWriter` | Dependency-free PNG encoder (8-bit RGBA, stored deflate) | ✅ |
| Texture viewer, GPU timer panel | Any render-graph texture on screen, per-pass timings | step 9 (timings already in the corner overlay) |
| A/B view, RMSE/FLIP metrics, benchmark CSV, camera paths, EXR | Write-up tooling | M0b |

**Depends on:** Core, Graphics/Renderer.
**Not responsible for:** recording GPU work (Renderer and the render graph do the capture copy), ImGui setup (UI).
