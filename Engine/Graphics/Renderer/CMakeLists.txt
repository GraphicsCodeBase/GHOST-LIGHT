# Graphics/Renderer module: per-frame orchestration on top of the Vulkan core (and, in later steps, the render graph and passes).
ghost_add_module(Graphics/Renderer DEPENDS Core Platform Graphics/Vulkan Graphics/ShaderCompiler Graphics/Passes)
