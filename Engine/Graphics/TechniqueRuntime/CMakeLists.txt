# Graphics/TechniqueRuntime module: the Technique base class, Param<T>, the registry and the runtime that turns
# techniques into render graph passes. Techniques/ builds against this module (and its dependencies) only.
ghost_add_module(Graphics/TechniqueRuntime DEPENDS Core Graphics/Vulkan Graphics/ShaderCompiler Graphics/RenderGraph Graphics/GpuScene Graphics/RayTracing)
