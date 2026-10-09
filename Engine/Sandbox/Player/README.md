# Sandbox/Player
**Purpose:** how the user moves through the world.
**Owns:** the camera pose (position, yaw, pitch) and its controls.

**Public API**
| Class | What it does |
|---|---|
| `FlyController` | `teleport(position, yaw, pitch)` (scene bookmarks use the same convention: yaw 0 looks down -Z, 90 down +X); `update(window, dt, speed)`: hold **RMB** to look (cursor captured, 0.12°/px), **WASD** move, **Q/E** down/up, **Shift** ×4, **Ctrl** ×¼, **wheel** changes the base speed (saved as `cameraSpeed` in `User/settings.json`); `position()`, `forward()`, `fovYDeg`, `nearPlane` |

**Depends on:** Core, Platform (window + input).
**Not responsible for:** projection matrices (GpuScene), walking/collision (M0b, with Physics), scene bookmarks (World).
