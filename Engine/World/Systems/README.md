# World/Systems
**Purpose:** per-frame logic over components. Run order (see [[16 Sandbox, ECS and Scenes]]): Player → SandboxTools →
Physics → **Transform** → Lights → **GpuSceneExtraction**.

| System | What it does | Status |
|---|---|---|
| `TransformSystem` | `Transform` (relative to parent) → `WorldTransform.matrix`; keeps last frame's matrix in `previous` for motion vectors | ✅ |
| `GpuSceneExtractionSystem` | ECS → GPU scene (instances, materials, lights, previous transforms) | step 6 |
| Player, sandbox tools, physics, lights | | M0b |
