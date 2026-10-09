// Placeholder full-screen pass: a softly breathing "ghost light" glow, shown until the render graph draws real frames.
// Edit and save this file while the engine runs to try shader hot reload.
import Fullscreen;
import Math;

struct SplashConstants
{
    float time;
    float aspect;
};

[[vk::push_constant]] ConstantBuffer<SplashConstants> gConstants;

[shader("fragment")]
float4 fragmentMain(FullscreenVaryings input) : SV_Target
{
    float2 p = (input.uv - float2(0.5, 0.42)) * float2(gConstants.aspect, 1.0);
    float pulse = 0.85 + 0.15 * sin(gConstants.time * 1.5);
    float glow = exp(-dot(p, p) * 7.0) * pulse;
    float3 color = lerp(float3(0.012, 0.010, 0.009), float3(1.0, 0.70, 0.36), glow);
    return float4(linearToSrgb(color), 1.0);
}
