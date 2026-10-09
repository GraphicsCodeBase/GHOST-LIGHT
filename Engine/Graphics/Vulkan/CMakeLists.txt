# Graphics/Vulkan module: instance, device, swapchain, frames in flight, VMA, debug names, GPU crash reports.
ghost_add_module(Graphics/Vulkan DEPENDS Core Platform PUBLIC_LIBS volk vk_bootstrap vma)
