---
tags: [prompt, kickoff]
---

# 01 · Master Prompt

> [!info] How to use
> Open a **new** Claude Code session with `D:\Raytracing_Engine` as the folder, then copy everything inside the code block below and send it as your first message.
> `CLAUDE.md` in the project root is loaded automatically every session. This prompt is the one-time kickoff.

```markdown
You are building the engine for my ray tracing portfolio project.

## Context
I'm aiming for a graphics programmer role at a game studio. I want a C++20 / Vulkan
hardware ray tracing sandbox where I implement techniques from Ray Tracing Gems I & II
myself. You build the engine; I build the techniques. The engine also has an ECS
(EnTT), Jolt physics, and a Garry's Mod-style sandbox (spawn prefabs, physics gun,
fly/walk toggle) so I can demo techniques with dynamic, interactive scenes.

My non-negotiables:
1. One-click: double-clicking run.bat on a fresh clone builds and runs everything.
2. I focus on techniques. The engine must be reliable and never get in my way.
3. Scenes and prefabs are defined in JSON files.
4. Garry's Mod-style interaction with objects.
5. ECS architecture (EnTT).
6. Portable: any Windows PC meeting the README requirements, no config or path edits.
7. Clean structure: one responsibility per folder, clear names, no loose files.

## Read first (in this order). This is the spec
1. Design_Vault/02 Goals and Context.md
2. Design_Vault/03 Division of Labor.md
3. Design_Vault/14 Project Structure.md
4. Design_Vault/15 Setup and Portability.md
5. Design_Vault/04 Tech Stack.md
6. Design_Vault/05 Architecture.md
7. Design_Vault/16 Sandbox, ECS and Scenes.md
8. Design_Vault/06 Technique API.md
9. Design_Vault/07 Roadmap.md  (focus: M0a)
10. Design_Vault/09 Assets.md
11. Design_Vault/10 Coding Standards.md
12. Design_Vault/12 Decisions Log.md  (already decided, don't re-open)
13. Design_Vault/13 Open Questions.md

## Your task: Milestone M0a
Build everything listed under "M0a" in 07 Roadmap.md until every acceptance
criterion passes. M0b (sandbox) comes after, in a later session.

## How to work
1. First, check this machine against the README requirements (VS 2022 C++ workload,
   Git, RTX GPU + driver) and tell me what's missing. Don't install anything.
2. Then present a step-by-step implementation plan for M0a and wait for my approval
   before writing code. Flag anything in the spec that is ambiguous, contradictory,
   over-engineered, or a bad idea. Ask, don't guess.
3. Build in small, verifiable steps. After each step: `run.bat test` passes (builds,
   runs, zero validation errors). Commit to git after each working step.
4. Before YOU download anything during this session, list file, source, and size and
   wait for my yes. (run.bat fetching pinned dependencies is the designed behavior.)
5. Do NOT implement any technique from 08 RTG Technique Catalog.md. Leave
   `// RTG-TODO(RTG<1|2> ch.<N>): ...` stubs with naive placeholders.
6. Keep the structure from 14 Project Structure.md exactly: module READMEs, one class
   per file, purpose comment on every file.
7. When M0a is done: write Design_Vault/17 RT Plumbing Explainer.md (see roadmap),
   update the roadmap checkboxes and the Decisions Log.

Start with step 1.
```

---

## Why the prompt is shaped this way
- **Non-negotiables restated** so they're in focus even before the spec is read.
- **"Read first" list** makes the session load the spec instead of improvising.
- **Plan + approval before code** catches misunderstandings early.
- **`run.bat test` after every step** keeps the engine reliable (spec 2).
- **M0a only**: a smaller scope that's easier to verify. The sandbox (M0b) gets its own session.

See also: [[03 Division of Labor]], [[07 Roadmap]]
