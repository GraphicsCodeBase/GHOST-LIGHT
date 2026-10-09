// The one translation unit that compiles Vulkan Memory Allocator's implementation (functions imported from volk).
#include <volk.h>

#pragma warning(push, 0)
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma warning(pop)
