# Engine modules (Claude's code). Every subfolder with a CMakeLists.txt is one module; see Engine/README.md.
add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Graphics/Vulkan)
add_subdirectory(Graphics/Renderer)
add_subdirectory(App)

ghost_check_module_includes()
