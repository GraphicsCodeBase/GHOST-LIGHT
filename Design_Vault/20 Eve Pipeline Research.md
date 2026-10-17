---
tags: [research, character, pipeline]
---

# 20 · Eve Pipeline Research (Phase R)

> [!summary] Findings in five lines
> 1. Shift Up has published **very little technical detail** on Eve. There's no GDC/NDC/Unreal Summit pipeline talk that
>    could be found. What's public comes from interviews (Famitsu, Push Square, PlayStation Blog) and a behind-the-scenes video.
> 2. **Confirmed:** UE4; body = **3D scan of a real model**; face **sculpted in-house**; real clothes bought and
>    **scanned**; monsters **sculpted in clay then scanned**; animation style drawn from **ballet and gymnastics**;
>    the long ponytail cost an estimated **~1 year** of development.
> 3. Everything else (rig, hair method, jiggle, shading) is **inferred** from standard UE4 practice.
> 4. Stellar Blade is a **raster** game. GHOST LIGHT's version is ray traced, a deliberate difference and the
>    graphics-programmer angle of this project.
> 5. Solo + AI is strongest at **runtime/graphics** stages (code + math, AI reviews) and **weakest at deformation-ready
>    content** (topology, weights, hair cards), where AI generators exist but output needs manual cleanup.

> [!important] Target revised 2026-10-17: **style match**, not fidelity match
> The question is now **"How close can one dev + AI get to Eve's *art style*?"**, with an original character
> (never Eve's design or the scanned model's body). See [[#Style target]] below. The pipeline map stays valid; the base
> character moves from VRoid (anime) to a **semi-realistic human base mesh**.

Legend: ✅ confirmed by a cited source · 🔶 inferred (standard UE4/AAA practice, not stated by Shift Up)
Kind: **Math** = I write the core math · **Content** = AI-assisted content, measured · **Engine** = Claude builds plumbing

---

## The pipeline, stage by stage

| # | Stage | What Shift Up did | Solo + AI equivalent | Kind |
|---|---|---|---|---|
| 1 | **Concept & design** | ✅ Kim Hyung-tae designed Eve (2019, "Eve 107"), in his illustration style, meant to stand out against the bleak world [W1][F1]. ✅ Extra modelling care on her **back**, since the third-person camera always shows it [G1] | Image models for mood boards and turnarounds (front/side/back sheet); I make the final design calls. Log the prompts | Content |
| 2 | **Body / likeness** | ✅ Body from a **3D scan of model Shin Jae-eun** (2020); later outfits used a second scanned model [P1][E1][D1]. ✅ Shift Up later built an in-house rig reportedly with **hundreds of cameras** (second-hand report) [P2] | **No scanning** (no rig, and real-person likeness is out of bounds). Options: **VRoid Studio** (parametric, gives a VRM with spring bones) or AI image→3D (TRELLIS.2, cloud generators). Measure how far each is from deformation-ready | Content |
| 3 | **Face** | ✅ Created **in-house** by artists, not scanned [P1][W1] | VRoid face sliders, or an AI mesh + manual sculpt touch-up. Expect the biggest quality gap here | Content |
| 4 | **Retopo, UVs, LODs** | 🔶 Scan → clean quad retopology with deformation-friendly edge loops; UV layout; LOD chain | AI meshes are triangle soups: auto-retopo (Instant Meshes, free; Quad Remesher, paid) + Blender. VRoid output is already game topology. **Key measurement:** hours of cleanup per path | Content |
| 5 | **Clothing** | ✅ Bought real clothes and **scanned them on a model** for realism [G2] | Model in Blender or VRoid's outfit editor. AI texture generation for fabric detail | Content |
| 6 | **Texturing & materials** | 🔶 Scan-derived albedo + hand-painted/Substance PBR maps; skin detail maps | AI texture tools + bake **thickness / curvature / AO** in UV space (needed by the SSS shading in stage 12) | Content → feeds Math |
| 7 | **Hair** | ✅ Long ponytail was a "big challenge", **~1 year** extra development by the director's estimate; tech director asked for shorter hair; a shorter-ponytail option shipped [S1][A1]. 🔶 Likely **hair cards** + bone chains (UE4 strand hair wasn't production-ready on console then) | **Hair cards** (RT curve primitives are Blackwell-only, ruled out). VRoid generates card-style hair with spring bones. Ray-traced alpha-tested cards = an any-hit cost to measure | Content + Math |
| 8 | **Rigging & skinning** | 🔶 Humanoid skeleton + physics/secondary bones (hair, cloth, chest), weight painting, corrective shapes | VRoid ships a rig + weights; AI meshes: **AccuRIG** (free, reportedly commercial-OK; check the EULA) [R1]. Runtime **linear blend skinning** is mine | Content + **Math** |
| 9 | **Face animation** | 🔶 Facial rig / blendshapes for cutscenes | VRM ships expression blendshapes. Tier 2 | Content |
| 10 | **Body animation** | ✅ Movement style built on **ballet and gymnastics** [W1]. 🔶 Mocap + hand-keyed combat polish | Animation libraries (Mixamo: local `User/` only; check CC0 packs such as Quaternius'), video→mocap (**GVHMR is non-commercial research + needs gated SMPL**, so experiment only, not shippable) [M1]. **Retargeting** onto my skeleton is mine | Content + **Math** |
| 11 | **Secondary motion** | ✅ Costume and hair movement were a showcase goal for PS5 [S1]. 🔶 UE4 AnimDynamics/RigidBody-style bone physics for hair, cloth and jiggle | **Verlet spring-bone chains** (VRM baseline → PBD constraints), jiggle bones → per-vertex soft skin, colliders | **Math** |
| 12 | **Shading** | 🔶 UE4 subsurface skin profile, raster lighting. No evidence of RT use (the PC feature list names DLSS 4/FSR 3, no RT) [T1] | **Ray-traced**: dual-lobe GGX, wrap diffuse + thickness transmission → RT SSS (Burley), RT shadows, temporal denoise | **Math** (+ Engine plumbing) |
| 13 | **Runtime & integration** | ✅ UE4 with in-house modifications [T1]. 🔶 Anim blueprints, state machines, LODs, performance budget | GHOST LIGHT: animation sampling + blending (Math), GPU skinning, per-instance deformed buffers, **BLAS refit**, per-vertex motion vectors (Engine) | **Math** + Engine |
| 14 | **Playable controller** | 🔶 Third-person camera, locomotion state machine, root motion | Jolt `CharacterVirtual` (planned in M0b walk mode) + third-person camera + a small locomotion state machine | Math + Engine |

**Monsters (context):** ✅ Naytiba were sculpted in clay by Hee-cheol Jang (*The Host*, *Okja*) then 3D scanned; physical
sculpts made team communication easier [W2][G3]. Physical-first pipelines are a Shift Up habit.

---

## Solo + AI tool licences (for a public portfolio)

| Tool | Use | Licence status |
|---|---|---|
| **VRoid Studio** | Base character + hair + rig + spring bones (anime style: dropped for the style target) | Free. The exporter **sets the VRM's licence fields** (commercial use, redistribution, alteration) [V1]. A model I build myself = I choose. Check the current official terms page |
| **TRELLIS.2** (Microsoft) | Image→3D mesh + PBR | **MIT** [T2]. Needs ~16–24 GB VRAM and Linux per reports, so it **won't run on an 8 GB laptop** (cloud GPU needed) |
| **Hunyuan3D 2.x** (Tencent) | Image→3D | Community licence **excludes the EU, UK and South Korea** [H1]. Check where you are before using it |
| Meshy / Tripo (cloud) | Text/image→3D | Paid tiers grant commercial rights; free tiers usually don't. Check each plan's terms |
| **AccuRIG** (Reallusion) | Auto-rig | Free; staff say commercial use is OK; 2.0 limits retargeting to ActorCore motions [R1] |
| **GVHMR** + SMPL | Video→mocap | **Non-commercial research**; SMPL is registration-gated [M1]. Fine for a measured experiment, not for shipped assets |
| Mixamo | Animations, auto-rig | Adobe login; files **not redistributable**, so `User/` only (see [[12 Decisions Log]] 2026-10-17) |

---

## Experiment log (one per stage, copy into the stage's write-up)
```markdown
### Stage <n>: <name>
- Path tried: <tool / manual>          - Hours: <me> / <waiting on AI>
- AI share: <0–100 %> of the output    - Cost: <$ / credits>
- Licence of the output: <...>
- Quality vs Eve reference: <side-by-side image + one-line verdict>
- What failed / what I had to fix by hand:
- Verdict: automate · assist · no help
```

---

## Style target
Eve's style as **my reading** (🔶 analysis, no published style guide). These are the traits we try to reproduce:

| Trait | What it means | Predicted reach (solo + AI) | Main limit |
|---|---|---|---|
| **Materials & finish** | Warm soft skin vs glossy latex/fabric suit vs hard sci-fi metal | **75–85 %** | Shading = my graphics work; RT helps. **Flagship** |
| **Lighting / presentation** | Cinematic, strong rim light, polished gloss | **~75 %** | Needs M1 shadows + M2 GGX first |
| **Costume & silhouette** | Slim elegant shape, long ponytail, suit readable **from behind** | **60–70 %** | Mechanical detail costs modelling time |
| **Hair** | Long flowing ponytail, look + motion | **~50 %** | Grooming good cards is art skill |
| **Motion** | Graceful, dance-like (ballet / gymnastics) | **40–50 %** | Library anims look generic; feel = performers + polish |
| **Face** | Idealised but realistic, the style's signature | **35–45 %** | Skilled sculpting; AI least reliable. **Key experiment** |

**Prediction: ~55–65 % "recognisably the same style"** (a few months part-time, Tier 1, controlled showcase).
Third-person / mid-shot camera ≈ 65–70 %; face close-ups ≈ 45 %. A photoreal *fidelity* match would sit nearer 30–35 %.

**Hypothesis to test:** *AI collapses engineering time far more than content time.* Expect: close on materials, lighting
and silhouette; partial on hair and motion; furthest on the face.

**Rules for style work:** describe the traits in AI prompts ("semi-realistic, idealised face, glossy sci-fi bodysuit,
long ponytail"), never the artist's or character's name; the design must be original.

**Showcase scene:** a small **bedroom** (warm window light, character + props). Interiors are RT's best case once M1, M2,
M5 and M6 exist; props are where AI 3D generation works best (no deformation). Path traced mode runs no techniques, so
the reference path tracer will eventually need the skin/hair materials for a fair comparison.

## Proposed Tier 1 (for approval)
**Character:** an original character on a **semi-realistic human base mesh**, stylised toward the target. Candidates
(licences to verify before choosing): Blender Studio human base meshes, MakeHuman / MPFB (output reportedly CC0),
Character Creator 4 (paid, check export terms). Comparison path: one **AI image→3D + AccuRIG** attempt at the same design.
VRoid drops out (anime style); spring-bone chains are authored by us instead of coming from VRM.

**Runtime (GHOST LIGHT):** idle / walk / run / jump with blending · Verlet hair on authored bone chains · jiggle bones ·
dual-lobe GGX + wrap diffuse/thickness transmission · suit materials (gloss/sheen) · RT shadows · third-person camera
on the M0b walking character · bedroom showcase scene.

**Out of Tier 1:** per-vertex soft skin, RT SSS, facial animation, cloth, combat (Tier 2–3).

## Proposed first build step: merge stage 1 "Skinning + BLAS refit"
Start with a **small redistributable rigged glTF sample** (e.g. Khronos `CesiumMan`, CC BY 4.0; verify when adding to
the manifest) so the engine work doesn't wait for the character.
1. **Math session first (no code):** transform hierarchy (local → global), inverse bind matrices, quaternion slerp vs
   nlerp, linear blend skinning and the candy-wrapper artifact, inverse-transpose for normals, BLAS refit vs rebuild.
2. **Claude:** cgltf skin + animation import into `Assets`, `Engine/Animation/` module skeleton, ECS components,
   skeleton debug overlay, skinning compute pass scaffold with `// CHAR-TODO` stubs where my math goes.
3. **Me:** keyframe sampling, hierarchy composition, the LBS function.
4. **Claude:** per-instance deformed + previous-position buffers, `ALLOW_UPDATE` BLAS + refit, per-vertex motion vectors.
5. **Done when:** the sample walks in Sponza in raster and path traced modes, motion vectors correct in F3, 0
   validation errors, smoke test loads a skinned scene. Then the first PR → `main`.

---

## Sources
- [W1] Wikipedia, *Eve (Stellar Blade)*: https://en.wikipedia.org/wiki/Eve_(Stellar_Blade) (cites Famitsu, ThisIsGame, Gematsu)
- [W2] Wikipedia, *Stellar Blade* (UE4, timeline, Naytiba clay sculpts): https://en.wikipedia.org/wiki/Stellar_Blade
- [P1] Push Square, interview with Kim Hyung-tae (Feb 2024): https://www.pushsquare.com/news/2024/02/stellar-blades-protagonists-body-is-based-on-this-real-world-korean-model
- [P2] Push Square, scanning-rig upgrade (Jun 2025, second-hand): https://www.pushsquare.com/news/2025/06/stellar-blade-dev-massively-upgrades-3d-scanning-tech-ahead-of-sequel
- [E1] esports.gg, Shin Jae-eun scan: https://esports.gg/news/gaming/stellar-blades-eve-based-on-irl-korean-model-shin-jae-eun/
- [D1] Dexerto, second scan model: https://www.dexerto.com/gaming/stellar-blade-model-actress-2529809/
- [F1] Automaton, Eve's body design (via Push Square/Famitsu): https://automaton-media.com/en/news/20240207-26822/
- [S1] Siliconera, Famitsu interview on Eve's hair: https://siliconera.com/eves-long-hair-extended-stellar-blade-development-time/
- [A1] Automaton, Famitsu interview follow-up: https://automaton-media.com/en/news/stellar-blade-dev-talks-about-fetishistic-elements-in-the-game/
- [G1] TheGamer, attention to Eve's back: https://www.thegamer.com/stellar-blade-devs-paid-special-attention-to-main-character-eve-back/
- [G2] GamingBolt, behind-the-scenes video (scanned clothing): https://gamingbolt.com/stellar-blade-behind-the-scenes-video-discusses-inspiration-challenges-and-naytiba-design
- [G3] GamesHedge, Naytiba clay models: https://www.gameshedge.com/stellar-blade-shift-up-details-in-game-monster-creation-process/
- [T1] Icy Veins / GameStar, PC port features (UE4, DLSS 4, FSR 3): https://www.gamestar.de/artikel/stellar-blade-pc-systemanforderungen,3433023.html
- [T2] TRELLIS.2 model card (MIT): https://huggingface.co/microsoft/TRELLIS.2-4B
- [H1] Hunyuan3D 2.0 licence text: https://huggingface.co/tencent/Hunyuan3D-2/blob/main/LICENSE
- [V1] VRoid / VRM conditions of use: https://developer.vroid.com/en/guidelines/conditions_of_use.html
- [R1] Reallusion forum, AccuRIG licence: https://forum.reallusion.com/551972/reallusion-Accu-Rig-Free ; AccuRIG 2.0: https://80.lv/articles/reallusion-launches-free-rigging-tool-accurig-2-0
- [M1] GVHMR weights (non-commercial): https://huggingface.co/ryanrudes/gvhmr

> [!note] Gaps worth a follow-up
> The original Famitsu and PlayStation Blog interviews (primary sources for hair and ballet), and any Korean-language
> talks by Shift Up staff. Push Square pages returned 403 to automated fetches, so their quotes are via other outlets.

See also: [[19 Character Pipeline Prompt]], [[12 Decisions Log]], [[07 Roadmap]]
