# Scripts/CMake
**Purpose:** CMake helpers included by the root `CMakeLists.txt`.

| File | Role |
|---|---|
| `CompilerSettings.cmake` | C++20, MSVC flags (`/utf-8 /permissive-`), `/Z7` debug info, exes output to `Build/<Config>/` |
| `Dependencies.cmake` | Third-party targets built from `.tools/deps/` (CMake never downloads; `run.bat` does) |
| `GhostModule.cmake` | `ghost_add_module(<Path> DEPENDS … PUBLIC_LIBS … PRIVATE_LIBS … EXCLUDE …)` and the include-rule check |

**Not responsible for:** fetching anything (see `Scripts/Downloads.ps1`), per-module source lists (globbed automatically).
