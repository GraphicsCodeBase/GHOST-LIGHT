# Graphics/Vulkan/Internal
**Purpose:** implementation-only files of the Vulkan module. Other modules may not include anything from here (the build enforces it).

| File | Why it's here |
|---|---|
| `VmaImplementation.cpp` | The single translation unit that compiles Vulkan Memory Allocator (`VMA_IMPLEMENTATION`), with VMA's own warnings silenced |
