// Vulkan instance: loads the Vulkan loader (volk), enables validation in Debug, and routes validation messages into the engine log.
#pragma once

#include <volk.h>

#include <VkBootstrap.h>

namespace ghost::graphics::vulkan {

class Instance {
public:
    Instance() = default;
    ~Instance();
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    // Returns false after logging a clear reason (no Vulkan driver, Vulkan older than 1.3, ...).
    bool create(bool wantValidation);
    void destroy();

    VkInstance handle() const { return m_instance.instance; }
    const vkb::Instance& bootstrap() const { return m_instance; }
    bool validationEnabled() const { return m_validationEnabled; }
    bool debugUtilsEnabled() const { return m_debugUtilsEnabled; }

private:
    vkb::Instance m_instance;
    bool m_created = false;
    bool m_validationEnabled = false;
    bool m_debugUtilsEnabled = false;
};

} // namespace ghost::graphics::vulkan
