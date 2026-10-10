// Texture viewer: shows one render graph texture inside a screen rectangle. Point-sampled (every texel is a real value),
// with channel selection and a [min, max] range mapped to black..white. Pixels outside the rectangle are left untouched.
import Fullscreen;
import Bindless;
import Math;
import Packing;

struct TextureViewerConstants
{
    uint input;      // sampled index of the viewed texture
    uint channels;   // 0 RGB, 1 R, 2 G, 3 B, 4 A, 5 signed RGB (x * 0.5 + 0.5 after the range: motion),
                     // 6 octahedral normal in xy, 7 in zw (decoded, shown as n * 0.5 + 0.5)
    float rangeMin;
    float rangeMax;
    float4 rect;     // uv min.xy, max.xy of the rectangle on screen
};

[[vk::push_constant]] ConstantBuffer<TextureViewerConstants> gConstants;

[shader("fragment")]
float4 fragmentMain(FullscreenVaryings input) : SV_Target
{
    float4 rect = gConstants.rect;
    if (any(input.uv < rect.xy) || any(input.uv >= rect.zw))
    {
        discard;
    }
    float2 local = (input.uv - rect.xy) / (rect.zw - rect.xy);
    uint width, height;
    gTextures[NonUniformResourceIndex(gConstants.input)].GetDimensions(width, height);
    int2 texel = min(int2(local * float2(width, height)), int2(width, height) - 1);
    float4 value = loadTexture(gConstants.input, texel);

    float3 shown;
    switch (gConstants.channels)
    {
    case 1: shown = value.rrr; break;
    case 2: shown = value.ggg; break;
    case 3: shown = value.bbb; break;
    case 4: shown = value.aaa; break;
    case 6: shown = octahedralDecode(value.xy) * 0.5 + 0.5; break;
    case 7: shown = octahedralDecode(value.zw) * 0.5 + 0.5; break;
    default: shown = value.rgb; break;
    }
    float range = max(gConstants.rangeMax - gConstants.rangeMin, 1e-12);
    if (gConstants.channels == 5)
    {
        shown = clamp(shown / max(abs(gConstants.rangeMax), 1e-12), -1.0, 1.0) * 0.5 + 0.5;
    }
    else if (gConstants.channels < 6)
    {
        shown = saturate((shown - gConstants.rangeMin) / range);
    }
    // NaN / inf show as magenta so broken values stand out.
    if (any(isnan(value.rgb)) || any(isinf(value.rgb)))
    {
        shown = float3(1.0, 0.0, 1.0);
    }
    return float4(linearToSrgb(shown), 1.0);
}
