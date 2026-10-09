---
tags: [home, index]
---

# 🏠 GHOST LIGHT — Design Vault

A C++/Vulkan ray tracing sandbox for implementing techniques from **Ray Tracing Gems I & II**, with an **ECS (EnTT)**, **Jolt physics**, and a **Garry's Mod-style sandbox**. Built as a portfolio for a **graphics programmer** role in game studios.

> [!tip] How to use this vault
> 1. Read the notes in order the first time.
> 2. Edit anything that doesn't match what you want. **This vault is the spec**: the next Claude session builds from it.
> 3. When ready, open a new Claude Code session in `D:\Raytracing_Engine` and paste the prompt from [[01 Master Prompt]].

## 🎯 My core specs
1. **One-click setup**: double-click `run.bat` → [[15 Setup and Portability]]
2. **Focus on techniques, not engine code**: the engine just works → [[03 Division of Labor]]
3. **Proper scene definition** in JSON files → [[16 Sandbox, ECS and Scenes]]
4. **Garry's Mod-style sandbox**: spawn prefabs, physics gun → [[16 Sandbox, ECS and Scenes]]
5. **ECS** (EnTT) → [[16 Sandbox, ECS and Scenes]]
6. **Portable**: clone on any RTX PC and run, no config → [[15 Setup and Portability]]
7. **Clean, navigable structure**: one responsibility per folder → [[14 Project Structure]]

## 📋 The spec
| # | Note | What it answers |
|---|---|---|
| 01 | [[01 Master Prompt]] | The exact prompt to paste into the next session |
| 02 | [[02 Goals and Context]] | Why this project exists, scope |
| 03 | [[03 Division of Labor]] | What Claude builds vs. what **I** build |
| 04 | [[04 Tech Stack]] | Language, API, libraries, hardware |
| 05 | [[05 Architecture]] | How the systems fit together |
| 06 | [[06 Technique API]] | What writing a technique looks like |
| 07 | [[07 Roadmap]] | Milestones M0a → M7 with checklists |
| 08 | [[08 RTG Technique Catalog]] | Every relevant RTG chapter + how games use it |
| 09 | [[09 Assets]] | Scenes, HDRIs, licenses, hosting |
| 10 | [[10 Coding Standards]] | Code style |
| 11 | [[11 Portfolio Plan]] | How techniques become portfolio pieces |
| 12 | [[12 Decisions Log]] | Settled decisions (and why) |
| 13 | [[13 Open Questions]] | Still to decide |
| 14 | [[14 Project Structure]] | Folder layout, module rules, dependency direction |
| 15 | [[15 Setup and Portability]] | `run.bat`, requirements, path rules |
| 16 | [[16 Sandbox, ECS and Scenes]] | ECS components, scene/prefab JSON, physics, sandbox tools, player modes |
| 17 | `17 RT Plumbing Explainer` | *Written by Claude after M0a* |
| 18 | [[18 Session Log]] | **Where we left off.** Updated at the end of every session |

## 🧩 Templates
- [[Technique Writeup]]: one per finished technique

## 📍 Current status
- **Phase:** Design (spec complete, pending my final review)
- **Next milestone:** [[07 Roadmap#M0a — Renderer Core, ECS & One-Click Setup (Claude)|M0a]]
