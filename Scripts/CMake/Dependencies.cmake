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

# --- GLFW: window and input --------------------------------------------------------------------------
ghost_require_dependency("${GHOST_DEPS_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GHOST_DEPS_DIR}/glfw" "${CMAKE_BINARY_DIR}/_deps/glfw" EXCLUDE_FROM_ALL SYSTEM)
