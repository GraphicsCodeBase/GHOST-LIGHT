# Smoke test executable run by `run.bat test`: proves the engine builds, starts, runs frames and logs no errors.
add_executable(GhostLightSmokeTest SmokeTest.cpp)
target_link_libraries(GhostLightSmokeTest PRIVATE Ghost_App)
target_compile_options(GhostLightSmokeTest PRIVATE /W4 /WX)
ghost_copy_runtime_dlls(GhostLightSmokeTest)
