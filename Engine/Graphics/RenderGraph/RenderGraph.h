// Linear render graph: passes run in the order they were added. Each pass declares what it reads and writes; the graph
// allocates the textures, gives them bindless indices, inserts every barrier and layout transition, and times each pass.
// No pass reordering and no memory aliasing: simple to read, simple to explain (see 12 Decisions Log).
#pragma once

#include "Graphics/RenderGraph/GraphTypes.h"
#include "Graphics/RenderGraph/PassBuilder.h"
#include "Graphics/RenderGraph/PassContext.h"
#include "Graphics/Vulkan/GpuBuffer.h"
#include "Graphics/Vulkan/GpuImage.h"

#include <array>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
class BindlessDescriptors;
class DeletionQueue;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::rendergraph {

class GpuTimers;

class RenderGraph {
public:
    using SetupFn = std::function<void(PassBuilder&)>;
    using ExecuteFn = std::function<void(PassContext&)>;

    struct TextureInfo {
        std::string name;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent{};
        Lifetime lifetime = Lifetime::Transient;
        bool external = false;
    };

    RenderGraph() = default;
    ~RenderGraph();
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    void initialize(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless, vulkan::DeletionQueue& deletionQueue);
    void shutdown();

    // Forgets all passes; add them again afterwards. Textures with the same name and description are kept (no reallocation).
    void reset();
    // A texture owned outside the graph (the swapchain image); bind the actual image every frame with bindExternal().
    void declareExternal(const std::string& name, VkFormat format);
    void addPass(std::string name, PassKind kind, SetupFn setup, ExecuteFn execute);
    // Disabled passes are skipped entirely, including their barriers. Cheap: no recompile.
    void setPassEnabled(const std::string& name, bool enabled);

    // Runs pass setups (only after reset/addPass) and (re)allocates textures for this render size. Returns false if a
    // declaration is invalid (logged; the offending pass is skipped, the engine keeps running).
    bool compile(VkExtent2D renderSize, uint64_t lastSubmittedFrame);
    void bindExternal(const std::string& name, VkImage image, VkImageView view, VkExtent2D extent, VkImageLayout finalLayout);
    void execute(VkCommandBuffer cmd, uint64_t frameIndex, GpuTimers* timers);

    std::vector<TextureInfo> textures() const;
    const std::vector<std::string>& passNames() const { return m_passNames; }

private:
    friend class PassBuilder;
    friend class PassContext;

    enum class Access : uint8_t { Sampled, StorageRead, StorageWrite, StorageReadWrite, ColorAttachment, DepthAttachment, TransferSrc, TransferDst };

    // The layout, pipeline stages and memory access one kind of access needs (the input to every barrier).
    struct AccessInfo {
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;
        bool write = false;
    };
    static AccessInfo describe(Access access, PassKind kind);
    static VkPipelineStageFlags2 shaderStages(PassKind kind);

    // Synchronization state of one physical image (or buffer), carried from pass to pass and frame to frame.
    struct SyncState {
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags2 writeStages = VK_PIPELINE_STAGE_2_NONE;  // last write (or layout transition)
        VkAccessFlags2 writeAccess = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 readStages = VK_PIPELINE_STAGE_2_NONE;   // reads since that write (needed for write-after-read)
        VkPipelineStageFlags2 visibleStages = VK_PIPELINE_STAGE_2_NONE; // stages that already see that write
    };

    struct Texture {
        std::string name;
        TextureDesc desc;
        bool declared = false;
        bool referenced = false;
        bool external = false;
        VkImageUsageFlags usage = 0;
        // Physical images: [0] only, or [0] and [1] for History. `current` is the one written this frame.
        std::array<vulkan::GpuImage, 2> images;
        std::array<uint32_t, 2> sampledIndex{UINT32_MAX, UINT32_MAX};
        std::array<uint32_t, 2> storageIndex{UINT32_MAX, UINT32_MAX};
        std::array<SyncState, 2> state{};
        TextureDesc allocatedDesc;
        VkImageUsageFlags allocatedUsage = 0;
        VkExtent2D extent{};
        uint32_t current = 0;
        uint32_t framesSinceAllocation = 0;
        // External (swapchain) image for the current frame.
        VkImage externalImage = VK_NULL_HANDLE;
        VkImageView externalView = VK_NULL_HANDLE;
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct TextureAccess {
        uint32_t texture = 0;
        bool previous = false;
        Access access = Access::Sampled;
        LoadOp load = LoadOp::Clear;
        VkClearValue clear{};
    };

    struct Buffer {
        std::string name;
        BufferDesc desc;
        bool declared = false;
        bool referenced = false;
        vulkan::GpuBuffer buffer;
        VkDeviceSize allocatedSize = 0;
        SyncState state{};
    };

    struct BufferAccess {
        uint32_t buffer = 0;
        bool write = false;
    };

    struct Pass {
        std::string name;
        PassKind kind = PassKind::Compute;
        SetupFn setup;
        ExecuteFn execute;
        bool enabled = true;
        bool valid = true;
        std::vector<TextureAccess> textures;
        std::vector<BufferAccess> buffers;
        const char* label = ""; // stable pointer for GPU crash breadcrumbs
    };

    uint32_t findOrAddTexture(const std::string& name);
    uint32_t findOrAddBuffer(const std::string& name);
    TextureHandle addTextureAccess(uint32_t pass, const std::string& name, Access access, bool previous, LoadOp load = LoadOp::Clear,
                                   VkClearValue clear = {});
    BufferHandle addBufferAccess(uint32_t pass, const std::string& name, bool write);

    void runSetups();
    void allocateTexture(Texture& texture, VkExtent2D extent, uint64_t lastSubmittedFrame);
    void releaseTexture(Texture& texture, uint64_t lastSubmittedFrame);
    VkExtent2D extentFor(const TextureDesc& desc, VkExtent2D renderSize) const;
    uint32_t physicalIndex(const Texture& texture, bool previous) const;
    void transitionTexture(const TextureAccess& access, PassKind kind, std::vector<VkImageMemoryBarrier2>& barriers);
    void transitionBuffer(const BufferAccess& access, PassKind kind, std::vector<VkMemoryBarrier2>& barriers);
    void beginRendering(VkCommandBuffer cmd, const Pass& pass);

    const vulkan::Device* m_device = nullptr;
    vulkan::BindlessDescriptors* m_bindless = nullptr;
    vulkan::DeletionQueue* m_deletionQueue = nullptr;
    std::vector<Pass> m_passes;
    std::vector<std::string> m_passNames;
    std::vector<Texture> m_textures;
    std::vector<Buffer> m_buffers;
    std::unordered_map<std::string, uint32_t> m_textureLookup;
    std::unordered_map<std::string, uint32_t> m_bufferLookup;
    std::deque<std::string> m_labels; // only grows: command buffers in flight may still point at old labels
    VkExtent2D m_renderSize{};
    bool m_setupDirty = true;
};

} // namespace ghost::graphics::rendergraph
