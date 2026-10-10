// TechniqueName: <what this shader computes>. Imports come from ShaderLibrary/ or this folder only.
import Bindless;
import Scene;

struct Constants
{
    FrameConstants* frame;
    uint color;      // storage index of scene.color
    float strength;
    float4 tint;     // rgb used
};

[[vk::push_constant]] ConstantBuffer<Constants> gConstants;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    FrameConstants* frame = gConstants.frame;
    int2 pixel = int2(id.xy);
    if (any(float2(pixel) >= frame->renderSize))
    {
        return;
    }
    // Placeholder effect: tint the lit image. Replace with the technique.
    // RTG-TODO(RTG<1|2> ch.<N>): <what the real technique computes here>
    float4 color = gStorageImages[gConstants.color][pixel];
    color.rgb = lerp(color.rgb, color.rgb * gConstants.tint.rgb, gConstants.strength);
    gStorageImages[gConstants.color][pixel] = color;
}
