# Engine modules (Claude's code). Every subfolder with a CMakeLists.txt is one module; see Engine/README.md.
add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Graphics/Vulkan)
add_subdirectory(Graphics/ShaderCompiler)
add_subdirectory(Graphics/RenderGraph)
add_subdirectory(Graphics/Passes)
add_subdirectory(Graphics/Renderer)
add_subdirectory(UI)
add_subdirectory(DebugTools)
add_subdirectory(App)

ghost_check_module_includes()
