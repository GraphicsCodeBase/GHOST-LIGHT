# Graphics/RenderGraph
**Purpose:** passes declare what they read and write; the graph does the bookkeeping: allocation, bindless indices,
barriers, layout transitions, dynamic rendering, GPU timing. Pass code never writes a barrier.
**Owns:** every render-graph texture and buffer (GPU memory + bindless slots), their synchronization state, the GPU timestamp pools.

**How a pass looks**
```cpp
graph.addPass("Background", PassKind::Compute,
    [&](PassBuilder& b) { out = b.writeStorage("scene.color", {VK_FORMAT_R16G16B16A16_SFLOAT, Scale::Full}); },
    [&](PassContext& ctx) {
        ctx.bindPipeline(pipeline);                     // + the global bindless set
        ctx.pushConstants(Constants{ctx.storageIndex(out), time});
        ctx.dispatchForSize(ctx.extent(out));
    });
```

**Public API**
| Type | What it does |
|---|---|
| `RenderGraph` | `reset()`, `declareExternal("swapchain", fmt)`, `addPass(name, kind, setup, execute)`, `setPassEnabled()`, `compile(size, frame)`, `bindExternal(...)`, `execute(cmd, frame, timers)`, `textures()` |
| `PassBuilder` | `create`, `sample`, `samplePrevious`, `readStorage`, `writeStorage`, `readWriteStorage`, `colorAttachment`, `depthAttachment`, `copySource/Destination`, `createBuffer`, `readBuffer`, `writeBuffer` |
| `PassContext` | `cmd()`, `sampledIndex()`, `storageIndex()`, `extent()`, `historyValid()`, `bufferAddress()`, `bindPipeline()`, `pushConstants()`, `dispatchForSize()`, `drawFullscreenTriangle()` |
| `GraphTypes` | `PassKind` (Raster/Compute/RayTracing/Transfer), `Lifetime` (Transient/Persistent/History), `Scale` (Full/Half/Quarter/Fixed), `TextureDesc`, handles |
| `GpuTimers` | Timestamps around every pass, read back without stalling (a couple of frames late) |

**Design (deliberately simple)**
- **Linear:** passes run in the order added. No reordering, no memory aliasing ([[12 Decisions Log]]).
- **Barriers:** per physical image the graph tracks layout, last write and the reads since. A write or layout change
  waits for all of them; a read only waits for the last write, once per shader stage. Read→read in the same layout: no barrier.
- **Lifetimes:** Transient contents are discarded each frame; Persistent survive; History has two copies swapped every
  frame (`samplePrevious()` reads last frame's; `historyValid()` is false right after (re)allocation).
- Textures are allocated at `compile()` and **kept across `reset()`** when name + description are unchanged (toggling a
  technique doesn't reallocate everything). Replaced textures retire through the `DeletionQueue`.
- Every texture gets `SAMPLED` usage, so any of them can be shown in the texture viewer.
- A declaration error (reading a texture nobody creates, `samplePrevious` on a non-History texture, ...) is logged and
  that pass is skipped: the engine keeps running.

**Depends on:** Core, Graphics/Vulkan.
**Not responsible for:** which passes exist and in what order (Renderer, techniques), pipelines (ShaderCompiler).
