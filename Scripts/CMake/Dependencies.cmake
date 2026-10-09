# Third-party targets built from the pinned sources run.bat downloads into .tools/ (CMake itself never downloads).

set(GHOST_TOOLS_DIR "${CMAKE_SOURCE_DIR}/.tools")
set(GHOST_DEPS_DIR "${GHOST_TOOLS_DIR}/deps")

# Re-run CMake whenever a pin changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Scripts/Dependencies.json")

function(ghost_require_dependency folder)
  if(NOT EXISTS "${folder}")
    file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${folder}")
    message(FATAL_ERROR "Missing dependency folder ${rel}. Build through run.bat, which downloads the pinned dependencies first.")
  endif()
endfunction()

# --- glm: header-only math, configured for Vulkan (depth 0..1, radians) -----------------------------
ghost_require_dependency("${GHOST_DEPS_DIR}/glm")
add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE "${GHOST_DEPS_DIR}/glm")
target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_ENABLE_EXPERIMENTAL)

# --- nlohmann/json: header-only JSON for scenes, prefabs and settings -------------------------------
ghost_require_dependency("${GHOST_DEPS_DIR}/nlohmann-json")
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json SYSTEM INTERFACE "${GHOST_DEPS_DIR}/nlohmann-json/include")

# --- Vulkan headers + volk: the Vulkan API without the SDK (the loader ships with the GPU driver) ---------
ghost_require_dependency("${GHOST_DEPS_DIR}/vulkan-headers")
add_library(vulkan_headers INTERFACE)
target_include_directories(vulkan_headers SYSTEM INTERFACE "${GHOST_DEPS_DIR}/vulkan-headers/include")
target_compile_definitions(vulkan_headers INTERFACE VK_NO_PROTOTYPES)

ghost_require_dependency("${GHOST_DEPS_DIR}/volk")
add_library(volk STATIC "${GHOST_DEPS_DIR}/volk/volk.c")
target_include_directories(volk SYSTEM PUBLIC "${GHOST_DEPS_DIR}/volk")
target_link_libraries(volk PUBLIC vulkan_headers)

# --- vk-bootstrap: instance, GPU selection, device and swapchain setup ---------------------------------
ghost_require_dependency("${GHOST_DEPS_DIR}/vk-bootstrap")
add_library(vk_bootstrap STATIC "${GHOST_DEPS_DIR}/vk-bootstrap/src/VkBootstrap.cpp")
target_include_directories(vk_bootstrap SYSTEM PUBLIC "${GHOST_DEPS_DIR}/vk-bootstrap/src")
target_link_libraries(vk_bootstrap PUBLIC vulkan_headers)

# --- Vulkan Memory Allocator: header-only; the implementation is compiled in Graphics/Vulkan ------------
ghost_require_dependency("${GHOST_DEPS_DIR}/vma")
add_library(vma INTERFACE)
target_include_directories(vma SYSTEM INTERFACE "${GHOST_DEPS_DIR}/vma/include")
# Function pointers come from volk (vmaImportVulkanFunctionsFromVolk), not from static or dynamic lookup.
target_compile_definitions(vma INTERFACE VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=0)
target_link_libraries(vma INTERFACE volk)

# --- Slang: runtime shader compiler (prebuilt release); its DLLs are copied next to each executable ------
set(GHOST_SLANG_DIR "${GHOST_TOOLS_DIR}/slang")
ghost_require_dependency("${GHOST_SLANG_DIR}")
add_library(slang SHARED IMPORTED GLOBAL)
set_target_properties(slang PROPERTIES
  IMPORTED_LOCATION "${GHOST_SLANG_DIR}/bin/slang.dll"
  IMPORTED_IMPLIB "${GHOST_SLANG_DIR}/lib/slang.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${GHOST_SLANG_DIR}/include")
# slang.dll is a thin loader for slang-compiler.dll; the glslang/glsl modules help with SPIR-V. slang-llvm is CPU-only: skipped.
set(GHOST_RUNTIME_DLLS
  "${GHOST_SLANG_DIR}/bin/slang.dll"
  "${GHOST_SLANG_DIR}/bin/slang-compiler.dll"
  "${GHOST_SLANG_DIR}/bin/slang-glslang.dll"
  "${GHOST_SLANG_DIR}/bin/slang-glsl-module.dll")

function(ghost_copy_runtime_dlls target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${GHOST_RUNTIME_DLLS} "$<TARGET_FILE_DIR:${target}>"
    VERBATIM)
endfunction()

# --- GLFW: window and input --------------------------------------------------------------------------
ghost_require_dependency("${GHOST_DEPS_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GHOST_DEPS_DIR}/glfw" "${CMAKE_BINARY_DIR}/_deps/glfw" EXCLUDE_FROM_ALL SYSTEM)

# --- Dear ImGui (docking branch) with the GLFW and Vulkan (volk) backends ------------------------------
set(GHOST_IMGUI_DIR "${GHOST_DEPS_DIR}/imgui")
ghost_require_dependency("${GHOST_IMGUI_DIR}")
add_library(imgui STATIC
  "${GHOST_IMGUI_DIR}/imgui.cpp"
  "${GHOST_IMGUI_DIR}/imgui_draw.cpp"
  "${GHOST_IMGUI_DIR}/imgui_tables.cpp"
  "${GHOST_IMGUI_DIR}/imgui_widgets.cpp"
  "${GHOST_IMGUI_DIR}/imgui_demo.cpp"
  "${GHOST_IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${GHOST_IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")
target_include_directories(imgui SYSTEM PUBLIC "${GHOST_IMGUI_DIR}" "${GHOST_IMGUI_DIR}/backends")
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(imgui PUBLIC volk glfw)
