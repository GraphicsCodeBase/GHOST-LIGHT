// Tonemap: HDR scene.color -> exposure -> ACES filmic curve (Narkowicz fit) -> sRGB, written to the UNORM swapchain.
// A simple, standard operator; better exposure / tonemapping is an M7 topic.
import Fullscreen;
import Bindless;
import Math;

struct TonemapConstants
{
    uint input;      // sampled index of scene.color
    float exposure;  // linear multiplier: 1 / (1.2 * 2^EV100)
};

[[vk::push_constant]] ConstantBuffer<TonemapConstants> gConstants;

float3 acesFitted(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("fragment")]
float4 fragmentMain(FullscreenVaryings input) : SV_Target
{
    // Radiance beyond the RGBA16F range (the sun in an HDRI) is stored as +inf; clamp it so the curve stays finite.
    float3 hdr = min(sampleTextureLevel(gConstants.input, kSamplerLinearClamp, input.uv, 0.0).rgb, 65504.0);
    float3 ldr = acesFitted(hdr * gConstants.exposure);
    return float4(linearToSrgb(ldr), 1.0);
}
