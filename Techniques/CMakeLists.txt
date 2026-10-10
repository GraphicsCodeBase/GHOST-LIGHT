# Techniques (the user's code): every .cpp under Techniques/ except _Template/ is compiled into Ghost_Techniques, an
# OBJECT library linked straight into the executables (so each REGISTER_TECHNIQUE static runs; a static library would
# let the linker drop them). New technique folders are picked up on the next build: no CMake edits.
file(GLOB_RECURSE GHOST_TECHNIQUE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
list(FILTER GHOST_TECHNIQUE_SOURCES EXCLUDE REGEX "/_Template/")

add_library(Ghost_Techniques OBJECT ${GHOST_TECHNIQUE_SOURCES})
target_link_libraries(Ghost_Techniques PUBLIC Ghost_Graphics_TechniqueRuntime)
target_compile_options(Ghost_Techniques PRIVATE /W4 /WX "${GHOST_DEBUG_FLAGS}")

# The template is compiled into the smoke test only, so it can never rot.
file(GLOB_RECURSE GHOST_TEMPLATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/_Template/*.cpp")
add_library(Ghost_TechniqueTemplate OBJECT ${GHOST_TEMPLATE_SOURCES})
target_link_libraries(Ghost_TechniqueTemplate PUBLIC Ghost_Graphics_TechniqueRuntime)
target_compile_options(Ghost_TechniqueTemplate PRIVATE /W4 /WX "${GHOST_DEBUG_FLAGS}")

# Techniques may only include the technique runtime and what it depends on (never World/EnTT, Physics or Sandbox).
ghost_allowed_modules(GHOST_TECHNIQUE_ALLOWED "Graphics/TechniqueRuntime")
ghost_check_includes("techniques" "" "${GHOST_TECHNIQUE_ALLOWED}" "${GHOST_TECHNIQUE_SOURCES};${GHOST_TEMPLATE_SOURCES}")
