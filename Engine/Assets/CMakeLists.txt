# Assets module: CPU-side asset loading (glTF, images, procedural meshes) and the asset registry. No Vulkan here.
# Optimized even in Debug: importing Sponza unoptimized takes ~9 s instead of well under one.
ghost_add_module(Assets DEPENDS Core PRIVATE_LIBS cgltf stb EXCLUDE ThirdPartyImplementations.cpp OPTIMIZE_IN_DEBUG)

# cgltf + stb_image implementations, also optimized in Debug.
add_library(Ghost_Assets_Codecs STATIC Internal/ThirdPartyImplementations.cpp)
target_link_libraries(Ghost_Assets_Codecs PRIVATE cgltf stb)
target_compile_options(Ghost_Assets_Codecs PRIVATE "${GHOST_OPTIMIZED_DEBUG_FLAGS}")
target_link_libraries(Ghost_Assets PRIVATE Ghost_Assets_Codecs)
