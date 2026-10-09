# World/Systems
**Purpose:** per-frame logic over components. Run order (see [[16 Sandbox, ECS and Scenes]]): Player → SandboxTools →
Physics → **Transform** → Lights → **GpuSceneExtraction**.

| System | What it does | Status |
|---|---|---|
| `TransformSystem` | `Transform` (relative to parent) → `WorldTransform.matrix`; keeps last frame's matrix in `previous` for motion vectors | ✅ |
| `GpuSceneExtractionSystem` | ECS → `GpuScene`: uploads a model the first time an entity uses it (then frees its CPU texels), sets the environment, and each frame builds instances (`WorldTransform` + `MeshRenderer` + `MaterialOverride`), the sun (first `DirectionalLight`) and point/spot lights | ✅ |
| Player, sandbox tools, physics, lights | | M0b |
