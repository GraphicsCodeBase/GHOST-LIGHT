// The global bindless descriptor arrays (set 0) every shader shares. Indices come from the render graph / GPU scene.
// Must match Engine/Graphics/Vulkan/BindlessDescriptors.h.
module Bindless;

[[vk::binding(0, 0)]] public Texture2D gTextures[];
[[vk::binding(1, 0)]] public SamplerState gSamplers[];
[[vk::binding(2, 0)]] public RWTexture2D<float4> gStorageImages[];

// Fixed sampler slots (BindlessDescriptors::Sampler).
public static const uint kSamplerLinearRepeat = 0;
public static const uint kSamplerLinearClamp = 1;
public static const uint kSamplerNearestClamp = 2;
public static const uint kSamplerAnisotropicRepeat = 3;

// Texture indices may differ between neighbouring pixels (per-material textures): mark them non-uniform.
public float4 sampleTexture(uint texture, uint samplerIndex, float2 uv)
{
    return gTextures[NonUniformResourceIndex(texture)].Sample(gSamplers[samplerIndex], uv);
}

public float4 sampleTextureLevel(uint texture, uint samplerIndex, float2 uv, float lod)
{
    return gTextures[NonUniformResourceIndex(texture)].SampleLevel(gSamplers[samplerIndex], uv, lod);
}

public float4 loadTexture(uint texture, int2 pixel)
{
    return gTextures[NonUniformResourceIndex(texture)].Load(int3(pixel, 0));
}
