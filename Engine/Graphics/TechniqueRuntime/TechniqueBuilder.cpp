// TechniqueBuilder: records declarations; TechniqueManager turns them into render graph passes and pipelines.
#include "Graphics/TechniqueRuntime/TechniqueBuilder.h"

namespace ghost::graphics::techniques {

TechniqueBuilder::Pass& TechniqueBuilder::current() {
    if (m_passes.empty()) {
        m_passes.push_back({m_defaultPassName, PassType::Compute, /*implicit*/ true, {}});
    }
    return m_passes.back();
}

TechniqueBuilder& TechniqueBuilder::pass(const std::string& name, PassType type) {
    // Declarations made before the first pass() call belong to that first pass.
    if (m_passes.size() == 1 && m_passes.front().implicit) {
        m_passes.front().name = name;
        m_passes.front().type = type;
        m_passes.front().implicit = false;
    } else {
        m_passes.push_back({name, type, false, {}});
    }
    return *this;
}

TechniqueBuilder& TechniqueBuilder::read(const std::string& texture) {
    current().accesses.push_back({AccessKind::Read, texture, {}});
    return *this;
}

TechniqueBuilder& TechniqueBuilder::write(const std::string& texture) {
    current().accesses.push_back({AccessKind::Write, texture, {}});
    return *this;
}

TechniqueBuilder& TechniqueBuilder::write(const std::string& texture, VkFormat format, Scale scale) {
    current().accesses.push_back({AccessKind::Create, texture, {format, scale, Lifetime::Transient}});
    return *this;
}

TechniqueBuilder& TechniqueBuilder::readWrite(const std::string& texture) {
    current().accesses.push_back({AccessKind::ReadWrite, texture, {}});
    return *this;
}

TechniqueBuilder& TechniqueBuilder::history(const std::string& texture, VkFormat format, Scale scale) {
    current().accesses.push_back({AccessKind::History, texture, {format, scale, Lifetime::History}});
    return *this;
}

TechniqueBuilder& TechniqueBuilder::buffer(const std::string& name, VkDeviceSize bytes, Lifetime lifetime) {
    Access access{AccessKind::Buffer, name, {}};
    access.bytes = bytes;
    access.lifetime = lifetime;
    current().accesses.push_back(access);
    return *this;
}

void TechniqueBuilder::computePipeline(const std::string& name, const std::string& file, const std::string& entry) {
    m_compute.push_back({name, {file, entry}});
}

void TechniqueBuilder::rayPipeline(const std::string& name, const RayPipelineDesc& desc) {
    m_ray.push_back({name, desc});
}

void TechniqueBuilder::finish() {
    current(); // a technique that declared nothing still gets its one pass
    if (m_passes.size() == 1 && m_passes.front().implicit && !m_ray.empty()) {
        m_passes.front().type = PassType::RayTracing;
    }
}

} // namespace ghost::graphics::techniques
