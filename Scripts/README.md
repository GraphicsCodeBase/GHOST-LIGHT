# Scripts
**Purpose:** everything `run.bat` runs, plus build helpers. PowerShell 5.1 (ships with Windows), ASCII only.
**Owns:** `.tools/` (downloaded tools and dependency sources), `Build/run.log`.

| File | Role |
|---|---|
| `Run.ps1` | The 6 steps behind `run.bat`: requirements → tools → configure → build → assets → launch / test |
| `Common.ps1` | Step banners, `Build/run.log`, `Invoke-Logged` (runs a program, streams output to console + log) |
| `Downloads.ps1` | Pinned downloads: fetch once into `.tools/downloads/`, verify SHA-256, extract, stamp. Used for dependencies and assets |
| `VisualStudio.ps1` | Finds VS 2022+ (17.5 or newer) with the C++ workload, loads `vcvars64` into this process only, strips any Vulkan SDK variables |
| `Dependencies.json` | Every third-party library/tool: version, URL, SHA-256, size, license, destination |
| [`CMake/`](CMake/README.md) | CMake helpers included by the root `CMakeLists.txt` |

**Rules:** nothing here installs system-wide, asks for admin, or needs anything beyond the README requirements.
Environment changes live only in the `run.bat` process.

**Updating a dependency:** change `version` + `url` in `Dependencies.json`, clear `sha256`, run `run.bat`
(it prints the new hash as a warning), paste the hash back. `run.bat test` refuses unpinned entries.

**Depends on:** Windows' own `curl.exe` and `tar.exe` (Windows 10 1803+), Visual Studio's bundled CMake + Ninja.
**Not responsible for:** C++ build logic (that's CMake), downloading assets that aren't in `Content/AssetManifest.json`.
