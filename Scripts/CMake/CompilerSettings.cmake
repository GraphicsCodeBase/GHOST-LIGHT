# Global C++20 / MSVC settings shared by every GHOST LIGHT target (warning levels are set per target).

if(NOT MSVC)
  message(FATAL_ERROR "GHOST LIGHT builds with MSVC. Use run.bat, which loads the Visual Studio environment.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug info goes inside the .obj files (/Z7): no PDB contention between parallel Ninja jobs.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:Embedded>")

# Executables and the DLLs copied next to them land directly in Build/<Config>/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE $<$<CONFIG:Debug>:GHOST_DEBUG=1>)
add_compile_options(/utf-8 $<$<COMPILE_LANGUAGE:CXX>:/permissive-> $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
