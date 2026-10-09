# GHOST LIGHT — Project Rules for Claude

A C++20 / Vulkan hardware ray tracing sandbox for implementing techniques from
*Ray Tracing Gems* I & II, built as a portfolio for a **graphics programmer** role
at a game studio. Includes an ECS (EnTT), Jolt physics, and a Garry's Mod-style
sandbox for spawning and interacting with objects.

**The full spec lives in the Obsidian vault at `Design_Vault/`.**

> **Start of every session:** read the top entry of `Design_Vault/18 Session Log.md`
> to see where we left off, then tell the user in 2–3 lines what you understood and
> what you propose to do next.
> **End of every session** (or when the user says they're wrapping up): add a new entry
> at the top of the Session Log using its template, then commit.

Before planning or writing code, read at least:

- `Design_Vault/01 Master Prompt.md` — the kickoff brief
- `Design_Vault/03 Division of Labor.md` — what you build vs. what the user builds
- `Design_Vault/05 Architecture.md`, `06 Technique API.md`, `14 Project Structure.md`
- `Design_Vault/15 Setup and Portability.md` and `16 Sandbox, ECS and Scenes.md`
- `Design_Vault/07 Roadmap.md` — current milestone and acceptance criteria
- `Design_Vault/12 Decisions Log.md` — decisions already made; do not re-litigate

## Hard rules

1. **Do not implement Ray Tracing Gems techniques** listed in
   `Design_Vault/08 RTG Technique Catalog.md`. Those are the user's work. Build the
   infrastructure they plug into and leave `// RTG-TODO(RTG<1|2> ch.<N>): ...` stubs.
   Only write technique code if the user explicitly says "write it for me".
2. **Portability is non-negotiable.** A fresh `git clone` on any Windows machine that
   meets the README requirements must run by double-clicking `run.bat`, with no
   config edits. No absolute paths anywhere (code, CMake, JSON). Everything resolves
   from the repo root. Machine-specific state goes only in gitignored `User/`.
3. **Respect the module structure** in `Design_Vault/14 Project Structure.md`: one
   responsibility per folder, one class per file, file name = class name, a one-line
   purpose comment at the top of every file, no loose files, no dependency cycles.
   Every module folder keeps an up-to-date `README.md`.
4. **During development sessions, ask before downloading or installing anything**
   yourself (list file, source, size). `run.bat` auto-fetching pinned dependencies and
   free assets is fine. That's the product working as designed. Never accept license
   terms or sign in on the user's behalf.
5. **Target hardware: NVIDIA RTX GPU** (minimum RTX 20-series; dev machine is an
   RTX 2070 Super, 8 GB). No Ada/Blackwell-only features (SER, opacity micromaps).
6. **Every change must pass `run.bat test`** (build + smoke test, zero Vulkan
   validation errors) before you call it done.
7. Record new decisions in `Design_Vault/12 Decisions Log.md`; tick roadmap items in
   `Design_Vault/07 Roadmap.md` as they complete. Use Obsidian `[[wikilinks]]`.
8. **Git: you may commit without asking**, on the current milestone branch, after each
   working step (`run.bat test` passes). Every commit must use the feature-log format
   in `Design_Vault/10 Coding Standards.md#Git`, describing what was built and what it
   does. Never force-push or rewrite history. Push only when the user asks.
9. **Commit source only.** Code, shaders, CMake, scripts, JSON scenes/prefabs, the
   asset manifest + `CREDITS.md`, docs and the vault. Never logs, build output,
   binaries, caches, downloaded assets, captures or `User/`. Check `git status` before
   every commit; if a generated file shows up, add it to `.gitignore` instead of
   committing it.
