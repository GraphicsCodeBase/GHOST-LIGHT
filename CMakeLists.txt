# GHOST LIGHT root build: compiler settings, pinned third-party targets, engine modules and tests.
# Configure through run.bat, which downloads the dependencies into .tools/ and loads the VS environment first.
cmake_minimum_required(VERSION 3.25)
project(GhostLight VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Scripts/CMake")
include(CompilerSettings)
include(Dependencies)
include(GhostModule)

add_subdirectory(Engine)
add_subdirectory(Tests)
