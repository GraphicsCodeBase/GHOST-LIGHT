// Frame-ordered deferred destruction (entries are pushed in non-decreasing frame order, so the front retires first).
#include "Graphics/Vulkan/DeletionQueue.h"

#include <utility>

namespace ghost::graphics::vulkan {

DeletionQueue::~DeletionQueue() {
    flushAll();
}

void DeletionQueue::push(uint64_t lastFrameUsingIt, std::function<void()> destroy) {
    m_entries.push_back({lastFrameUsingIt, std::move(destroy)});
}

void DeletionQueue::flush(uint64_t completedFrame) {
    while (!m_entries.empty() && m_entries.front().frame <= completedFrame) {
        m_entries.front().destroy();
        m_entries.pop_front();
    }
}

void DeletionQueue::flushAll() {
    for (Entry& entry : m_entries) {
        entry.destroy();
    }
    m_entries.clear();
}

} // namespace ghost::graphics::vulkan
