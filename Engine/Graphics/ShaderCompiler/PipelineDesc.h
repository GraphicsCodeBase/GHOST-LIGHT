// Descriptions of graphics and compute pipelines built from Slang entry points (all use the shared bindless pipeline layout).
#pragma once

#include "Graphics/ShaderCompiler/ShaderEntryPoint.h"

#include <string>
#include <vector>

#include <volk.h>

namespace ghost::graphics::shader {

struct GraphicsPipelineDesc {
    std::string name;
    ShaderEntryPoint vertex;
    ShaderEntryPoint fragment;
    std::vector<VkFormat> colorFormats;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    bool depthTest = false;
    bool depthWrite = false;
    // Reverse-Z: depth 1 is near, 0 is far, so "closer" means GREATER (better float precision far away).
    VkCompareOp depthCompare = VK_COMPARE_OP_GREATER_OR_EQUAL;
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool alphaBlend = false;
};

struct ComputePipelineDesc {
    std::string name;
    ShaderEntryPoint compute;
};

} // namespace ghost::graphics::shader
