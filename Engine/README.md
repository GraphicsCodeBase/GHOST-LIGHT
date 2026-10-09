# Engine
**Purpose:** all engine code (Claude's side of the [[03 Division of Labor]]); one subfolder per module.
**Owns:** the module folders below. Each folder with a `CMakeLists.txt` is one static library (`Ghost_<Path>`).
**Public API:** see each module's README. Headers are included from this folder as root: `#include "Core/Log.h"`.
**Depends on:** third-party targets from `Scripts/CMake/Dependencies.cmake`.
**Not responsible for:** techniques (`Techniques/`), shared shader code (`ShaderLibrary/`), content (`Content/`).

## Modules (so far)
| Module | Purpose | Depends on |
|---|---|---|
| [Core](Core/README.md) | Logging, asserts, root-relative paths, JSON files, user settings, frame timing | glm, nlohmann/json |
| [Platform](Platform/README.md) | OS window and keyboard/mouse input | Core, GLFW (private) |
| [App](App/README.md) | `Engine` class, frame loop, `GhostLight.exe` | Core, Platform |

Graphics, World, Assets, UI and DebugTools arrive in later M0a steps. Physics and Sandbox arrive in M0b.

## Rules enforced by the build
- A module may only `#include` modules it lists in `DEPENDS` (directly or transitively). Breaking this fails
  the CMake configure step with the offending file and line (`ghost_check_module_includes`).
- `Internal/` headers are private to their module.
- `/W4 /WX`: warnings are errors in engine code.
