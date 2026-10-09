// GPU timestamps around every render graph pass. Results arrive a couple of frames late (when that frame's GPU work is done).
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
}

namespace ghost::graphics::rendergraph {

class GpuTimers {
public:
    struct Timing {
        std::string name;
        double milliseconds = 0.0;
    };
    static constexpr uint32_t kMaxScopes = 64;
    static constexpr uint32_t kMaxFrames = 3;

    GpuTimers() = default;
    ~GpuTimers();
    GpuTimers(const GpuTimers&) = delete;
    GpuTimers& operator=(const GpuTimers&) = delete;

    void create(const vulkan::Device& device, uint32_t framesInFlight);
    void destroy();

    // Call after the frame slot's previous GPU work is known to be done: collects its results, then resets its queries.
    void beginFrame(uint32_t slot);
    uint32_t begin(VkCommandBuffer cmd, const std::string& name);
    void end(VkCommandBuffer cmd, uint32_t scope);

    const std::vector<Timing>& results() const { return m_results; }
    double totalMilliseconds() const { return m_totalMilliseconds; }

private:
    struct Slot {
        VkQueryPool pool = VK_NULL_HANDLE;
        std::vector<std::string> names;
    };

    const vulkan::Device* m_device = nullptr;
    std::array<Slot, kMaxFrames> m_slots{};
    uint32_t m_slotCount = 0;
    uint32_t m_current = 0;
    double m_nanosecondsPerTick = 1.0;
    bool m_supported = false;
    std::vector<Timing> m_results;
    double m_totalMilliseconds = 0.0;
};

} // namespace ghost::graphics::rendergraph
