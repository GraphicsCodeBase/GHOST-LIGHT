// Device creation: required Vulkan 1.3 + KHR ray tracing features, optional diagnostics, VMA with buffer device address.
#include "Graphics/Vulkan/Device.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/GpuCrashReporter.h"
#include "Graphics/Vulkan/Instance.h"
#include "Graphics/Vulkan/VulkanCheck.h"

namespace ghost::graphics::vulkan {

namespace {

// NVIDIA packs the driver version as 10.8.8.6 bits (e.g. 591.86).
std::string driverVersionString(const VkPhysicalDeviceProperties& properties) {
    const uint32_t v = properties.driverVersion;
    if (properties.vendorID == 0x10DE) {
        return std::to_string((v >> 22) & 0x3FF) + "." + std::to_string((v >> 14) & 0xFF);
    }
    return std::to_string(VK_API_VERSION_MAJOR(v)) + "." + std::to_string(VK_API_VERSION_MINOR(v)) + "." + std::to_string(VK_API_VERSION_PATCH(v));
}

} // namespace

Device::~Device() {
    destroy();
}

bool Device::create(const Instance& instance, VkSurfaceKHR surface) {
    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.shaderInt64 = VK_TRUE;                          // 64-bit buffer device addresses in shaders
    features.fillModeNonSolid = VK_TRUE;                     // wireframe debug views
    features.multiDrawIndirect = VK_TRUE;
    features.drawIndirectFirstInstance = VK_TRUE;
    features.shaderStorageImageWriteWithoutFormat = VK_TRUE; // Slang storage images without explicit formats
    features.shaderStorageImageReadWithoutFormat = VK_TRUE;

    VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    features11.shaderDrawParameters = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
    features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    features12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    features12.timelineSemaphore = VK_TRUE;
    features12.scalarBlockLayout = VK_TRUE;
    features12.hostQueryReset = VK_TRUE;
    features12.vulkanMemoryModel = VK_TRUE;
    features12.vulkanMemoryModelDeviceScope = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;
    features13.maintenance4 = VK_TRUE;
    features13.shaderDemoteToHelperInvocation = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    accelerationStructure.accelerationStructure = VK_TRUE;
    accelerationStructure.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE; // TLAS lives in the bindless set
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracingPipeline.rayTracingPipeline = VK_TRUE;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    rayQuery.rayQuery = VK_TRUE;

    vkb::PhysicalDeviceSelector selector(instance.bootstrap(), surface);
    selector.set_minimum_version(1, 3)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .set_required_features(features)
        .set_required_features_11(features11)
        .set_required_features_12(features12)
        .set_required_features_13(features13)
        .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
        .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
        .add_required_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME)
        .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
        .add_required_extension_features(accelerationStructure)
        .add_required_extension_features(rayTracingPipeline)
        .add_required_extension_features(rayQuery);

    auto selected = selector.select();
    if (!selected) {
        core::Log::error("No GPU qualifies: GHOST LIGHT needs Vulkan 1.3 with hardware ray tracing "
                         "(an NVIDIA RTX 20-series or newer with a recent driver). Details: {}",
                         selected.error().message());
        auto names = vkb::PhysicalDeviceSelector(instance.bootstrap()).select_device_names();
        if (names) {
            for (const std::string& gpuName : names.value()) {
                core::Log::error("  found GPU: {}", gpuName);
            }
        }
        return false;
    }
    vkb::PhysicalDevice physical = selected.value();
    // Optional extras: GPU-crash breadcrumbs and accurate VRAM budget numbers.
    const bool checkpoints = physical.enable_extension_if_present(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
    const bool memoryBudget = physical.enable_extension_if_present(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

    auto built = vkb::DeviceBuilder(physical).build();
    if (!built) {
        core::Log::error("Could not create the Vulkan device on {}: {}", physical.name, built.error().message());
        return false;
    }
    m_device = built.value();
    m_created = true;
    volkLoadDevice(m_device.device);

    auto queue = m_device.get_queue(vkb::QueueType::graphics);
    auto family = m_device.get_queue_index(vkb::QueueType::graphics);
    if (!queue || !family) {
        core::Log::error("The GPU has no graphics queue");
        return false;
    }
    m_graphicsQueue = queue.value();
    m_graphicsQueueFamily = family.value();

    DebugUtils::initialize(m_device.device, instance.debugUtilsEnabled());
    DebugUtils::setName(m_graphicsQueue, "GraphicsQueue");
    GpuCrashReporter::initialize(m_device.device, m_graphicsQueue, checkpoints);
    queryProperties();

    const VkPhysicalDeviceProperties& p = properties();
    core::Log::info("GPU: {} (driver {}, Vulkan {}.{}.{}), crash breadcrumbs {}", p.deviceName, driverVersionString(p),
                    VK_API_VERSION_MAJOR(p.apiVersion), VK_API_VERSION_MINOR(p.apiVersion), VK_API_VERSION_PATCH(p.apiVersion),
                    checkpoints ? "on" : "unavailable");
    return createAllocator(instance, memoryBudget);
}

void Device::queryProperties() {
    m_accelerationStructureProperties.pNext = nullptr;
    m_rayTracingProperties.pNext = &m_accelerationStructureProperties;
    VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties2.pNext = &m_rayTracingProperties;
    vkGetPhysicalDeviceProperties2(physicalDevice(), &properties2);
    m_rayTracingProperties.pNext = nullptr;
}

bool Device::createAllocator(const Instance& instance, bool memoryBudget) {
    VmaAllocatorCreateInfo info{};
    info.instance = instance.handle();
    info.physicalDevice = physicalDevice();
    info.device = handle();
    info.vulkanApiVersion = VK_API_VERSION_1_3;
    info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (memoryBudget) {
        info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    }
    VmaVulkanFunctions functions{};
    VK_CHECK(vmaImportVulkanFunctionsFromVolk(&info, &functions));
    info.pVulkanFunctions = &functions;
    VK_CHECK(vmaCreateAllocator(&info, &m_allocator));
    return true;
}

void Device::waitIdle() const {
    if (m_created) {
        VK_CHECK(vkDeviceWaitIdle(m_device.device));
    }
}

void Device::destroy() {
    if (!m_created) {
        return;
    }
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }
    vkb::destroy_device(m_device);
    m_created = false;
}

} // namespace ghost::graphics::vulkan
