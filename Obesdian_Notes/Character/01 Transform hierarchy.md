---
tags: [character, stage1, lesson]
---

# 01 · Transform hierarchy
**Stage:** 1   **Status:** studying   **Back to:** [[00 Home]]   **Next:** 02 Inverse bind matrices

## Intuition
Every node stores its transform **relative to its parent**, never relative to the world. An animator thinks "bend the
elbow 90°", not "move the wrist to (0.3, 1.75, 0)". The hierarchy turns those local edits into global positions by
composing transforms down the tree.

> [!key] A child's local translation is expressed in its **parent's frame**, after the parent's rotation and scale.
> That's why rotating a joint moves its whole subtree without changing the joint's own position.

## Derivation
glTF uses column vectors, stored column-major (the same as glm). Local matrix from TRS, in the order the glTF spec
fixes:

$$L_i = T(t_i)\,R(q_i)\,S(s_i)$$

Applied to a point, this scales first, then rotates, then translates.

Composition down the tree:

$$G_i = \begin{cases} L_i & i \text{ is a root}\\ G_{p(i)}\,L_i & \text{otherwise}\end{cases}$$

Unrolled for a chain: $G_\text{wrist} = L_\text{shoulder}\,L_\text{elbow}\,L_\text{wrist}$. A point in wrist space goes
through the wrist's own transform first, then the elbow's, then the shoulder's.

A joint's position is the translation column of its global matrix: $G_i[3].xyz$.

### Numeric example
- Shoulder: $t=(0,1.5,0)$, no rotation
- Elbow: $t=(0.3,0,0)$, rotated 90° about Z
- Wrist: $t=(0.25,0,0)$, no rotation

| Node | Global position | Why |
|---|---|---|
| Shoulder | (0, 1.5, 0) | root |
| Elbow | (0.3, 1.5, 0) | shoulder has no rotation, so the offset is added as-is |
| Wrist | (0.3, **1.75**, 0) | the offset (0.25,0,0) is **rotated by the elbow's 90° first**, becoming (0,0.25,0) |

The elbow's rotation didn't move the elbow, but it swung the wrist from +X to +Y.

## The details that cause bugs
1. **Evaluation order.** $G_i$ needs $G_{p(i)}$ first. glTF does **not** guarantee that the node array lists parents
   before children. Reorder the joints once at load so that `parent[i] < i` (the layout Gregory describes). Then one
   flat forward loop works, with no recursion and good cache behaviour.
2. **Never store global transforms as TRS.** A non-uniform parent scale combined with a child rotation that isn't a
   multiple of 90° produces **shear**, and you can't decompose that back into T·R·S (Unity's `lossyScale` exists
   because of this). Keep globals as `mat4`. This comes back in 05 for normals.
3. **Matrix vs TRS nodes.** A glTF node has either a `matrix` or TRS fields, and animated nodes must use TRS.
   Animation channels overwrite single TRS components. A component with no channel keeps the node's default value,
   so don't reset it to identity.
4. **Unit quaternions.** Normalise before converting to a matrix, because interpolated quaternions often aren't unit
   length (see 03). glTF stores them as (x, y, z, w), but glm's constructor takes (w, x, y, z).
5. **Negative or zero scale.** A negative determinant mirrors the geometry, which flips triangle winding and the
   direction of normals. Zero scale is fine for $G$ but breaks every inverse you take later (02, 05).
6. **Skinned meshes ignore their own node transform.** The joint globals place the vertices. This is the bridge to 02.

## What it becomes in code (I implement it)
```cpp
// Local TRS -> 4x4, glTF order T*R*S. Expects unit quaternion (normalise or assert).
glm::mat4 localMatrix(const glm::vec3& t, const glm::quat& r, const glm::vec3& s);

// Global pose from locals. Precondition: parents[i] < i, or -1 for roots.
void computeGlobalPose(std::span<const int32_t>   parents,
                       std::span<const glm::mat4> local,
                       std::span<glm::mat4>       global);   // out, same size
```
- **Inputs/outputs:** N joints, from about 20 (CesiumMan) to 100 or more later. The output overwrites `global`.
- **Edge cases:**
  - several roots
  - sizes that don't match (assert)
  - a parent index that's out of order (assert in debug builds, since a silent wrong pose is worse than a crash)
  - a non-unit quaternion
  - negative or zero scale
- **Split of work:** the import code (Claude) does the parent-before-child reordering. My function asserts the
  precondition and doesn't sort.

## Pitfalls and debug visualisations
- **Skeleton overlay:** a line from $G_{p(i)}[3]$ to $G_i[3]$ for every joint.
- **Axis tripods per joint:** draw columns 0–2 of $G_i$. They show rotations, and non-orthogonal axes reveal shear.
- **Colour by depth:** catches a wrong parent index.
- **Joint poke panel:** rotate one joint by hand and check that only its subtree moves.
- **Bind-pose check:** compare joint positions against Blender at frame 0.

## Quiz (answer below)
1. Using the shoulder/elbow/wrist example, someone writes `G_i = L_i * G_parent` instead of `G_parent * L_i`. Where
   does the wrist end up, and what does that look like in motion?
2. A parent has scale (2,1,1). Its child is rotated **45°** about Z. Can you store the child's global transform as TRS?
   What happens visually to a cube attached to the child?
3. `parents = [-1, 3, 0, 0]`. What goes wrong with a single forward loop, and what are two ways to fix it?

### My summary
-

### My answers
1.
2.
3.

## My notes
-

## Sources
- glTF 2.0 specification: *Nodes and Hierarchy*, *Transformations*, *Animations*
- Jason Gregory, *Game Engine Architecture*: Animation Systems chapter (skeletons, poses, joint ordering)
- Jacobson et al., *Skinning: Real-time Shape Deformation*, SIGGRAPH 2014 course (background for 02–04)
