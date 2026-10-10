# Smoke test executable run by `run.bat test`: proves the engine builds, starts, runs frames and logs no errors.
add_executable(GhostLightSmokeTest SmokeTest.cpp)
target_link_libraries(GhostLightSmokeTest PRIVATE Ghost_App Ghost_Techniques Ghost_TechniqueTemplate)
target_include_directories(GhostLightSmokeTest PRIVATE "${CMAKE_SOURCE_DIR}/Engine")
target_compile_options(GhostLightSmokeTest PRIVATE /W4 /WX "${GHOST_DEBUG_FLAGS}")
ghost_copy_runtime_dlls(GhostLightSmokeTest)
