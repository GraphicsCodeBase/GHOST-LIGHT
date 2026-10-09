# Platform module: the OS window and keyboard/mouse input (GLFW stays private to this module).
ghost_add_module(Platform DEPENDS Core PRIVATE_LIBS glfw)
