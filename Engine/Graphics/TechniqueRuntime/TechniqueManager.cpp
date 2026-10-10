// TechniqueManager: registry -> instances; builder declarations -> render graph passes + pipelines; scene settings.
#include "Graphics/TechniqueRuntime/TechniqueManager.h"

#include "Core/Log.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/RenderGraph/RenderGraph.h"
#include "Graphics/TechniqueRuntime/Param.h"
#include "Graphics/TechniqueRuntime/TechniqueBuilder.h"
#include "Graphics/TechniqueRuntime/TechniqueContext.h"
#include "Graphics/TechniqueRuntime/TechniqueRegistry.h"

#include <memory>

#include <nlohmann/json.hpp>

namespace ghost::graphics::techniques {

namespace {

shader::ShaderEntryPoint entryPoint(const Technique& technique, const TechniqueBuilder::ShaderRef& shader, shader::ShaderStage stage) {
    return {technique.folder() / shader.file, shader.entry, stage};
}

} // namespace

TechniqueManager::~TechniqueManager() {
    shutdown();
}

void TechniqueManager::initialize(shader::PipelineLibrary& pipelines, const vulkan::Device& device, vulkan::DeletionQueue& deletionQueue) {
    m_pipelines = &pipelines;
    m_device = &device;
    m_deletionQueue = &deletionQueue;
    for (const TechniqueRegistry::Entry& entry : TechniqueRegistry::entries()) {
        auto instance = std::make_unique<TechniqueInstance>();
        instance->technique = TechniqueRegistry::create(entry);
        m_instances.push_back(std::move(instance));
    }
    core::Log::info("Techniques: {} registered", m_instances.size());
}

void TechniqueManager::shutdown() {
    m_instances.clear(); // the caller waited for the GPU; ray pipelines release their shader binding tables
}

std::vector<Technique*> TechniqueManager::techniques() const {
    std::vector<Technique*> result;
    for (const auto& instance : m_instances) {
        result.push_back(instance->technique.get());
    }
    return result;
}

Technique* TechniqueManager::find(const std::string& id) const {
    for (const auto& instance : m_instances) {
        if (instance->technique->id() == id) {
            return instance->technique.get();
        }
    }
    return nullptr;
}

bool TechniqueManager::enabledStateChanged() {
    bool changed = false;
    for (const auto& instance : m_instances) {
        if (instance->technique->enabled != instance->wasEnabled) {
            instance->wasEnabled = instance->technique->enabled;
            changed = true;
        }
    }
    return changed;
}

void TechniqueManager::createPipelines(TechniqueInstance& instance, const TechniqueBuilder& builder) {
    const Technique& technique = *instance.technique;
    for (const auto& compute : builder.m_compute) {
        if (instance.computePipelines.contains(compute.name)) {
            continue; // created on an earlier graph build; hot reload keeps it current
        }
        const std::string name = technique.id() + "." + compute.name;
        instance.computePipelines[compute.name] =
            m_pipelines->addCompute({name, entryPoint(technique, compute.shader, shader::ShaderStage::Compute)});
    }
    for (const auto& ray : builder.m_ray) {
        if (instance.rayPipelines.contains(ray.name)) {
            continue;
        }
        raytracing::RayTracingPipelineDesc desc;
        desc.name = technique.id() + "." + ray.name;
        desc.rayGeneration = entryPoint(technique, ray.desc.rayGeneration, shader::ShaderStage::RayGeneration);
        for (const auto& miss : ray.desc.misses) {
            desc.misses.push_back(entryPoint(technique, miss, shader::ShaderStage::Miss));
        }
        for (const auto& group : ray.desc.hitGroups) {
            raytracing::RayTracingHitGroup hitGroup;
            if (!group.closestHit.file.empty()) {
                hitGroup.closestHit = entryPoint(technique, group.closestHit, shader::ShaderStage::ClosestHit);
            }
            if (!group.anyHit.file.empty()) {
                hitGroup.anyHit = entryPoint(technique, group.anyHit, shader::ShaderStage::AnyHit);
            }
            desc.hitGroups.push_back(hitGroup);
        }
        auto pipeline = std::make_unique<raytracing::RayTracingPipeline>();
        pipeline->initialize(*m_pipelines, *m_device, *m_deletionQueue, std::move(desc));
        instance.rayPipelines[ray.name] = std::move(pipeline);
    }
}

void TechniqueManager::addPasses(rendergraph::RenderGraph& graph, const scene::GpuScene& scene, TechniqueStage stage) {
    for (const auto& instance : m_instances) {
        if (instance->technique->enabled && instance->technique->stage() == stage) {
            addTechniquePasses(graph, scene, *instance);
        }
    }
}

void TechniqueManager::addTechniquePasses(rendergraph::RenderGraph& graph, const scene::GpuScene& scene, TechniqueInstance& instance) {
    TechniqueBuilder builder(instance.technique->id());
    instance.technique->setup(builder);
    builder.finish();
    createPipelines(instance, builder);

    const bool single = builder.m_passes.size() == 1;
    for (uint32_t passIndex = 0; passIndex < builder.m_passes.size(); ++passIndex) {
        const TechniqueBuilder::Pass& pass = builder.m_passes[passIndex];
        const rendergraph::PassKind kind =
            pass.type == TechniqueBuilder::PassType::RayTracing ? rendergraph::PassKind::RayTracing : rendergraph::PassKind::Compute;
        const std::string passName = single ? instance.technique->id() : instance.technique->id() + "." + pass.name;
        auto resources = std::make_shared<TechniqueContext::Resources>();
        TechniqueInstance* target = &instance;

        graph.addPass(
            passName, kind,
            [pass, resources](rendergraph::PassBuilder& graphBuilder) {
                using Kind = TechniqueBuilder::AccessKind;
                for (const TechniqueBuilder::Access& access : pass.accesses) {
                    switch (access.kind) {
                    case Kind::Read: resources->sampled[access.name] = graphBuilder.sample(access.name); break;
                    case Kind::Write: resources->storage[access.name] = graphBuilder.writeStorage(access.name); break;
                    case Kind::Create: resources->storage[access.name] = graphBuilder.writeStorage(access.name, access.desc); break;
                    case Kind::ReadWrite: resources->storage[access.name] = graphBuilder.readWriteStorage(access.name); break;
                    case Kind::History:
                        graphBuilder.create(access.name, access.desc);
                        resources->storage[access.name] = graphBuilder.writeStorage(access.name);
                        resources->previous[access.name] = graphBuilder.samplePrevious(access.name);
                        break;
                    case Kind::Buffer:
                        graphBuilder.createBuffer(access.name, {access.bytes, access.lifetime});
                        resources->buffers[access.name] = graphBuilder.writeBuffer(access.name);
                        break;
                    }
                }
            },
            [this, target, passIndex, kind, resources, &scene](rendergraph::PassContext& passContext) {
                // Change flags are computed once per frame, at the technique's first pass.
                if (passIndex == 0) {
                    target->sceneChanged = scene.sceneRevision() != target->lastSceneRevision;
                    target->lightsChanged = scene.lightRevision() != target->lastLightRevision;
                    target->lastSceneRevision = scene.sceneRevision();
                    target->lastLightRevision = scene.lightRevision();
                }
                TechniqueContext context(passContext, *target, *m_pipelines, passIndex, kind, *resources, scene);
                target->technique->execute(context);
            });
    }
}

void TechniqueManager::applySettings(const nlohmann::json& settings, const std::string& source) {
    for (const auto& instance : m_instances) {
        instance->technique->enabled = false;
        for (ParamBase* param : instance->technique->params()) {
            param->reset();
        }
    }
    if (!settings.is_object()) {
        return;
    }
    for (const auto& [id, block] : settings.items()) {
        Technique* technique = find(id);
        if (!technique) {
            core::Log::warning("{}: techniques.{}: no technique with that class name is compiled in", source, id);
            continue;
        }
        if (!block.is_object()) {
            core::Log::warning("{}: techniques.{}: expected an object like {{ \"enabled\": true, \"params\": {{}} }}", source, id);
            continue;
        }
        if (block.contains("enabled")) {
            technique->enabled = block["enabled"].is_boolean() && block["enabled"].get<bool>();
        }
        if (!block.contains("params") || !block["params"].is_object()) {
            continue;
        }
        for (const auto& [key, value] : block["params"].items()) {
            ParamBase* match = nullptr;
            for (ParamBase* param : technique->params()) {
                if (param->key() == key) {
                    match = param;
                }
            }
            if (!match) {
                core::Log::warning("{}: techniques.{}.params.{}: unknown parameter", source, id, key);
            } else if (!match->fromJson(value)) {
                core::Log::warning("{}: techniques.{}.params.{}: wrong value type", source, id, key);
            }
        }
    }
}

} // namespace ghost::graphics::techniques
