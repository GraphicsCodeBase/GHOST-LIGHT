# Platform
**Purpose:** the operating-system window and raw keyboard/mouse input.
**Owns:** the GLFW window and GLFW's global state (initialized with the first window, terminated with the last).
**Public API:**
- `Window`: create from `Window::Desc` (fits the size to the monitor, maximizes when it doesn't fit), `pollEvents()`, `framebufferSize()`, `isMinimized()`, `setCursorCaptured()`, `toggleFullscreen()` (borderless), `handle()` for Vulkan surface creation.
- `Input` (from `window.input()`): `isDown / wasPressed / wasReleased` for `Key` and `MouseButton`, `mouseDelta()`, `wheelDelta()`, `setUiCapture()` so ImGui can take the keyboard or mouse.

**Depends on:** Core. GLFW is a private dependency: no GLFW header leaks out of this module.
**Not responsible for:** what keys *do* (App, Sandbox), the Vulkan surface and swapchain (Graphics), UI (UI).
