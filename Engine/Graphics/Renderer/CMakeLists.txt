# Graphics/Renderer module: per-frame orchestration on top of the Vulkan core, the GPU scene, ray tracing, the render graph and the passes.
ghost_add_module(Graphics/Renderer DEPENDS Core Platform Graphics/Vulkan Graphics/ShaderCompiler Graphics/RenderGraph Graphics/GpuScene Graphics/RayTracing Graphics/Passes)
