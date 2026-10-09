---
tags: [spec, standards]
---

# 10 · Coding Standards

> Folder and module rules are in [[14 Project Structure]]. This note covers code style.

## C++
- **C++20**, MSVC `/W4`, warnings as errors in `Engine/`.
- Naming: `PascalCase` types and files, `camelCase` functions/variables, `m_` members, `k` constants.
- Namespaces mirror folders: `Engine/Graphics/RayTracing/` → `ghost::graphics::raytracing`.
- RAII for every Vulkan and Jolt object.
- `VK_CHECK` → log + abort with a clear message. No exceptions in hot paths.
- **Readable over clever.** I need to read and explain this code.
- Every file starts with a one-line purpose comment. Comments explain *why*.

## Shaders (Slang)
- File names in `PascalCase` (`ShadowRayGen.slang`).
- Shared code only through `ShaderLibrary/`.
- Entry points: `rayGen`, `closestHit`, `anyHit`, `miss`, `main` (compute).

## JSON (scenes, prefabs, configs)
- 2-space indent, `camelCase` keys, paths relative to `Content/`.
- Every file has a `"version"` field so formats can evolve.

## Stubs
```cpp
// RTG-TODO(RTG<1|2> ch.<N>): <what the real technique replaces>
```

## Vulkan hygiene
- Validation on in Debug; **zero errors** = done.
- Debug names on every object.
- GPU timer scope around every pass.

## Git
- I'm the only developer. **Claude commits freely** on the current milestone branch after each working step (`run.bat test` passes). No force-push, no history rewrites. Push only when I ask.
- Branch per milestone (`m0a-core`, `m1-shadows`), merged when acceptance passes.
- Never commit `Build/`, `.tools/`, `User/`, `Content/Assets/*` (except `CREDITS.md`), `Captures/`.

### Commit message format (feature log)
Every commit documents what was built and what it does, so `git log` reads like a changelog.
```text
<area>: <short summary in imperative, ≤ 72 chars>

What was built:
- <concrete pieces: classes, files, systems>

What it does:
- <behavior from the user's point of view; why it exists>

How to verify:
- <e.g. run.bat, open Sponza, press Q, spawn a crate>

Roadmap: <milestone + checklist item, e.g. M0b: Physics gun>
```
Example:
```text
sandbox: add physics gun with grab, throw and freeze

What was built:
- PhysicsGun tool (Engine/Sandbox/Tools/PhysicsGun.h/.cpp)
- Jolt distance constraint used to hold the grabbed body
- Freeze sets the Frozen tag and switches the body to static

What it does:
- LMB grabs any Grabbable entity under the crosshair, wheel changes distance,
  E + mouse rotates, RMB freezes in place, releasing throws with momentum.

How to verify:
- run.bat, press 1, grab a crate in Sponza, throw it, freeze another mid-air

Roadmap: M0b: Physics gun
```

See also: [[14 Project Structure]]
