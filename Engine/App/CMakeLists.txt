# App module: the Engine class and frame loop (library), plus GhostLight.exe built from Main.cpp.
ghost_add_module(App DEPENDS Core Platform Assets World Graphics/Renderer UI DebugTools Sandbox/Player EXCLUDE Main.cpp)

add_executable(GhostLight Main.cpp)
target_link_libraries(GhostLight PRIVATE Ghost_App)
target_compile_options(GhostLight PRIVATE /W4 /WX "${GHOST_DEBUG_FLAGS}")
ghost_copy_runtime_dlls(GhostLight)
