// Procedural HDR sky: a gradient horizon plus a bright "ghost light" bulb. Fills scene.color until the G-buffer and
// lighting passes draw real scenes, and stays as the fallback background when a scene has no HDRI.
import Bindless;
import Math;

struct BackgroundConstants
{
    uint output;     // storage index of scene.color
    float time;
    uint2 size;
};

[[vk::push_constant]] ConstantBuffer<BackgroundConstants> gConstants;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= gConstants.size))
        return;

    float2 uv = (float2(id.xy) + 0.5) / float2(gConstants.size);
    float aspect = float(gConstants.size.x) / float(gConstants.size.y);

    // Dusk gradient: deep blue overhead fading to a warm horizon.
    float height = 1.0 - uv.y;
    float3 sky = lerp(float3(0.60, 0.42, 0.30), float3(0.05, 0.08, 0.18), smoothstep(0.0, 0.9, height));

    // The ghost light: a bulb far brighter than 1.0, so the tonemapper has real HDR to compress.
    float2 p = (uv - float2(0.5, 0.42)) * float2(aspect, 1.0);
    float pulse = 0.9 + 0.1 * sin(gConstants.time * 1.5);
    float bulb = exp(-dot(p, p) * 900.0) * 40.0;
    float halo = exp(-dot(p, p) * 18.0) * 1.6;
    float3 light = float3(1.0, 0.72, 0.40) * (bulb + halo) * pulse;

    gStorageImages[gConstants.output][id.xy] = float4(sky + light, 1.0);
}
