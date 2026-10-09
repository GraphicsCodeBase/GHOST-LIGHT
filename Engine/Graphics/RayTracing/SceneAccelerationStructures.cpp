// SceneAccelerationStructures: BLAS build -> compacted-size query -> compacting copy (load time); TLAS build per frame.
#include "Graphics/RayTracing/SceneAccelerationStructures.h"

#include "Core/Log.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <memory>
#include <string>

namespace ghost::graphics::raytracing {

namespace {

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void memoryBarrier(VkCommandBuffer cmd, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
                   VkAccessFlags2 dstAccess) {
    VkMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

constexpr VkPipelineStageFlags2 kBuildStage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;

} // namespace

SceneAccelerationStructures::~SceneAccelerationStructures() {
    shutdown();
}

void SceneAccelerationStructures::initialize(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless,
                                             vulkan::DeletionQueue& deletionQueue) {
    m_device = &device;
    m_bindless = &bindless;
    m_deletionQueue = &deletionQueue;
    m_submit.create(device);
}

void SceneAccelerationStructures::shutdown() {
    if (!m_device) {
        return;
    }
    // The caller waited for the GPU.
    m_blases.clear();
    for (TlasSlot& slot : m_slots) {
        slot = TlasSlot{};
    }
    m_submit.destroy();
    m_device = nullptr;
}

VkDeviceSize SceneAccelerationStructures::blasBytes() const {
    VkDeviceSize total = 0;
    for (const AccelerationStructure& blas : m_blases) {
        total += blas.size();
    }
    return total;
}

VkDeviceAddress SceneAccelerationStructures::alignedScratchAddress(const vulkan::GpuBuffer& scratch) const {
    return alignUp(scratch.address(), m_device->accelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
}

void SceneAccelerationStructures::updateBlases(const scene::GpuScene& scene, uint64_t lastSubmittedFrame) {
    if (scene.geometryRevision() == m_geometryRevision) {
        return;
    }
    m_geometryRevision = scene.geometryRevision();
    const auto start = std::chrono::steady_clock::now();

    // TLASes of frames still in flight reference the old BLASes.
    for (AccelerationStructure& blas : m_blases) {
        if (blas.valid()) {
            auto old = std::make_shared<AccelerationStructure>(std::move(blas));
            m_deletionQueue->push(lastSubmittedFrame, [old] { old->destroy(); });
        }
    }
    const auto& meshes = scene.meshes();
    m_blases.clear();
    m_blases.resize(meshes.size());
    if (meshes.empty()) {
        return;
    }

    const auto& primitives = scene.primitives();
    const auto& vertexCounts = scene.primitiveVertexCounts();
    const auto& materials = scene.materials();
    const VkDevice device = m_device->handle();

    // One BLAS per mesh, one geometry per primitive (GeometryIndex() in shaders = primitive within the mesh).
    struct Build {
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
        VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        AccelerationStructure uncompacted;
    };
    std::vector<Build> builds(meshes.size());
    VkDeviceSize scratchSize = 0;
    VkDeviceSize uncompactedBytes = 0;
    for (size_t m = 0; m < meshes.size(); ++m) {
        Build& build = builds[m];
        std::vector<uint32_t> triangleCounts;
        for (uint32_t p = 0; p < meshes[m].primitiveCount; ++p) {
            const uint32_t index = meshes[m].firstPrimitive + p;
            const scene::GpuPrimitive& primitive = primitives[index];
            VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            // Alpha-masked geometry runs any-hit shaders (alpha test); everything else is opaque (faster traversal).
            geometry.flags = (materials[primitive.material].flags & scene::kMaterialAlphaMask) ? VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR
                                                                                             : VK_GEOMETRY_OPAQUE_BIT_KHR;
            VkAccelerationStructureGeometryTrianglesDataKHR& triangles = geometry.geometry.triangles;
            triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            triangles.vertexData.deviceAddress = scene.vertexBufferAddress() + primitive.firstVertex * scene::GpuScene::kVertexStride;
            triangles.vertexStride = scene::GpuScene::kVertexStride;
            triangles.maxVertex = std::max(vertexCounts[index], 1u) - 1;
            triangles.indexType = VK_INDEX_TYPE_UINT32;
            triangles.indexData.deviceAddress = scene.indexBufferAddress() + primitive.firstIndex * sizeof(uint32_t);
            build.geometries.push_back(geometry);
            build.ranges.push_back({primitive.indexCount / 3, 0, 0, 0});
            triangleCounts.push_back(primitive.indexCount / 3);
        }
        build.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        build.info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build.info.geometryCount = static_cast<uint32_t>(build.geometries.size());
        build.info.pGeometries = build.geometries.data();
        if (build.geometries.empty()) {
            continue;
        }
        VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build.info, triangleCounts.data(), &sizes);
        build.uncompacted.create(*m_device, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizes.accelerationStructureSize,
                                 "BLAS.Build" + std::to_string(m));
        scratchSize = std::max(scratchSize, sizes.buildScratchSize);
        uncompactedBytes += sizes.accelerationStructureSize;
    }

    // Builds run one after another and share one scratch buffer.
    const VkDeviceSize scratchAlignment = m_device->accelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;
    vulkan::GpuBuffer scratch;
    scratch.create(*m_device, {scratchSize + scratchAlignment, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vulkan::GpuBuffer::Memory::GpuOnly, "BLAS.Scratch"});
    const VkDeviceAddress scratchAddress = alignedScratchAddress(scratch);

    VkQueryPoolCreateInfo queryInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    queryInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    queryInfo.queryCount = static_cast<uint32_t>(builds.size());
    VkQueryPool queryPool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateQueryPool(device, &queryInfo, nullptr, &queryPool));
    vulkan::DebugUtils::setName(queryPool, "BLAS.CompactedSizes");
    vkResetQueryPool(device, queryPool, 0, queryInfo.queryCount);

    m_submit.run([&](VkCommandBuffer cmd) {
        for (size_t m = 0; m < builds.size(); ++m) {
            Build& build = builds[m];
            if (!build.uncompacted.valid()) {
                continue;
            }
            build.info.dstAccelerationStructure = build.uncompacted.handle();
            build.info.scratchData.deviceAddress = scratchAddress;
            const VkAccelerationStructureBuildRangeInfoKHR* ranges = build.ranges.data();
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build.info, &ranges);
            // The next build reuses the scratch memory; the size query reads the finished structure.
            memoryBarrier(cmd, kBuildStage, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, kBuildStage,
                          VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
            const VkAccelerationStructureKHR handle = build.uncompacted.handle();
            vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, 1, &handle, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool,
                                                          static_cast<uint32_t>(m));
        }
    });

    std::vector<uint64_t> compactedSizes(builds.size(), 0);
    for (size_t m = 0; m < builds.size(); ++m) {
        if (builds[m].uncompacted.valid()) {
            VK_CHECK(vkGetQueryPoolResults(device, queryPool, static_cast<uint32_t>(m), 1, sizeof(uint64_t), &compactedSizes[m], sizeof(uint64_t),
                                           VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
        }
    }
    vkDestroyQueryPool(device, queryPool, nullptr);

    m_submit.run([&](VkCommandBuffer cmd) {
        // The copies read the structures built by the previous submit.
        memoryBarrier(cmd, kBuildStage, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                      VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR);
        for (size_t m = 0; m < builds.size(); ++m) {
            if (!builds[m].uncompacted.valid()) {
                continue;
            }
            m_blases[m].create(*m_device, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, compactedSizes[m], "BLAS.Mesh" + std::to_string(m));
            VkCopyAccelerationStructureInfoKHR copy{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
            copy.src = builds[m].uncompacted.handle();
            copy.dst = m_blases[m].handle();
            copy.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
            vkCmdCopyAccelerationStructureKHR(cmd, &copy);
        }
        // Every later TLAS build and ray traversal reads them.
        memoryBarrier(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                      VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR);
    });
    // `builds` (uncompacted structures) and `scratch` are released here; the GPU is done with them (run() waits).

    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    core::Log::info("Ray tracing: built {} BLAS ({:.1f} MB, {:.1f} MB after compaction) in {:.0f} ms", meshes.size(),
                    static_cast<double>(uncompactedBytes) / (1024.0 * 1024.0), static_cast<double>(blasBytes()) / (1024.0 * 1024.0), ms);
}

void SceneAccelerationStructures::ensureTlasCapacity(uint32_t slotIndex, uint32_t instanceCount) {
    TlasSlot& slot = m_slots[slotIndex];
    if (slot.tlas.valid() && slot.capacity >= instanceCount) {
        return;
    }
    const uint32_t capacity = std::max(64u, std::bit_ceil(instanceCount));

    VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    info.geometryCount = 1;
    info.pGeometries = &geometry;
    VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(m_device->handle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, &capacity, &sizes);

    // This slot's previous frame has finished (the frame scheduler waited), so its old TLAS can go right away.
    const std::string suffix = std::to_string(slotIndex);
    slot.tlas.create(*m_device, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizes.accelerationStructureSize, "TLAS.Slot" + suffix);
    const VkDeviceSize scratchAlignment = m_device->accelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;
    slot.scratch.create(*m_device, {sizes.buildScratchSize + scratchAlignment, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    vulkan::GpuBuffer::Memory::GpuOnly, "TLAS.Scratch" + suffix});
    slot.instances.create(*m_device, {capacity * sizeof(VkAccelerationStructureInstanceKHR),
                                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, vulkan::GpuBuffer::Memory::Upload,
                                      "TLAS.Instances" + suffix});
    slot.capacity = capacity;
    m_bindless->setAccelerationStructure(slotIndex, slot.tlas.handle());
}

void SceneAccelerationStructures::recordTlasBuild(VkCommandBuffer cmd, uint32_t slotIndex, const scene::GpuScene& scene) {
    slotIndex %= kFrameSlots;
    const auto& instances = scene.instances();
    const auto& instanceMeshes = scene.instanceMeshes();
    ensureTlasCapacity(slotIndex, static_cast<uint32_t>(instances.size()));
    TlasSlot& slot = m_slots[slotIndex];

    auto* out = static_cast<VkAccelerationStructureInstanceKHR*>(slot.instances.mapped());
    uint32_t count = 0;
    for (size_t i = 0; i < instances.size(); ++i) {
        const uint32_t mesh = instanceMeshes[i];
        if (mesh >= m_blases.size() || !m_blases[mesh].valid()) {
            continue;
        }
        VkAccelerationStructureInstanceKHR& instance = out[count++];
        const glm::mat4& world = instances[i].world;
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 4; ++column) {
                instance.transform.matrix[row][column] = world[column][row]; // glm is column-major, Vulkan wants rows
            }
        }
        instance.instanceCustomIndex = static_cast<uint32_t>(i);
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        // Facing is resolved in shaders (double-sided materials), so traversal never culls.
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = m_blases[mesh].address();
    }
    m_lastInstanceCount = count;

    VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = slot.instances.address();
    VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    info.dstAccelerationStructure = slot.tlas.handle();
    info.geometryCount = 1;
    info.pGeometries = &geometry;
    info.scratchData.deviceAddress = alignedScratchAddress(slot.scratch);
    const VkAccelerationStructureBuildRangeInfoKHR range{count, 0, 0, 0};
    const VkAccelerationStructureBuildRangeInfoKHR* ranges = &range;

    // Readers of this slot's TLAS two frames ago are done (CPU waited); the barrier also orders scratch reuse.
    memoryBarrier(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, kBuildStage,
                  VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &info, &ranges);
    memoryBarrier(cmd, kBuildStage, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                  VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}

} // namespace ghost::graphics::raytracing
