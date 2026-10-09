---
tags: [log, handoff]
---

# 18 · Session Log

> [!important] Rule for every Claude session
> **At the start:** read the latest entry below to know where we left off.
> **At the end** (or when the user says they're wrapping up): add a new entry at the **top**, then commit.
> Keep entries short. Details belong in the commits and the other notes.

### Entry template
```markdown
## YYYY-MM-DD: <one-line summary>
**Milestone:** <e.g. M0a>   **Branch:** <branch>   **Last commit:** <short hash + title>
**Done this session:**
- …
**Where we left off:**
- <exact state: what works, what's half-done, any failing test>
**Next steps:**
1. …
**Open issues / blockers:**
- …
**Decisions made:** (also added to [[12 Decisions Log]])
- …
```

---

## 2026-10-10: Design phase complete, spec written
**Milestone:** pre-M0a (design)   **Branch:** main (repo being created by the user)   **Last commit:** none yet
**Done this session:**
- Chose the direction: custom C++20/Vulkan RT engine, Ray Tracing Gems I & II techniques, graphics programmer portfolio
- Researched RTG I & II chapters → [[08 RTG Technique Catalog]]
- Researched asset sources → [[09 Assets]]
- Defined the 7 core specs: one-click `run.bat`, focus on techniques, JSON scenes, Garry's Mod sandbox (Jolt), ECS (EnTT), portability, clean module structure
- Named the engine **GHOST LIGHT**
- Wrote the full vault (notes 00–16), root `README.md`, `CLAUDE.md`, `.gitignore`
- Set the commit policy: Claude commits freely with feature-log messages ([[10 Coding Standards#Git]])

**Where we left off:**
- The user is creating the GitHub repo (suggested name `GhostLight`, with no README, .gitignore or license on GitHub's side).
- No code exists yet. The first commit will be the spec itself.

**Next steps:**
1. User creates the repo → Claude makes the first commit (vault + README + CLAUDE.md + .gitignore)
2. User answers the "Before M0a" items in [[13 Open Questions]] (public/private repo, tutor mode strictness)
3. New session → paste [[01 Master Prompt]] → Claude checks prerequisites, then plans M0a for approval

**Open issues / blockers:**
- None

**Decisions made:** all recorded in [[12 Decisions Log]]
