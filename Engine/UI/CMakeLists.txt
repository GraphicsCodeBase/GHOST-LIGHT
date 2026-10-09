# UI module: Dear ImGui setup and the engine's overlays and panels (ImGui and GLFW stay private to this module).
ghost_add_module(UI DEPENDS Core Platform Graphics/Vulkan Graphics/ShaderCompiler Graphics/Renderer PRIVATE_LIBS imgui glfw)
