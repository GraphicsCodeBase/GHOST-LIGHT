# Graphics/ShaderCompiler module: Slang -> SPIR-V, pipelines built from it, hot reload and error data for the overlay.
ghost_add_module(Graphics/ShaderCompiler DEPENDS Core Graphics/Vulkan PRIVATE_LIBS slang)
