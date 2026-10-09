// Placeholder deferred lighting: Lambert diffuse from the sun and point/spot lights (no shadows), a constant ambient
// term from the environment's average radiance, plus emission. Sky pixels show the environment.
// Deliberately simple: shadows are M1 techniques and the GGX BRDF is M2 (both the user's work, see 07 Roadmap).
import GBuffer;
import Environment;
import Math;
import Scene;
import Bindless;

struct LightingConstants
{
    FrameConstants* frame;
    GBufferIndices gbuffer;
    uint output; // storage index of scene.color
};

[[vk::push_constant]] ConstantBuffer<LightingConstants> gConstants;

float3 punctualLight(GpuLight light, float3 position, float3 normal)
{
    float3 toLight = light.position - position;
    float distanceSquared = max(dot(toLight, toLight), light.radius * light.radius);
    float3 direction = toLight * rsqrt(max(dot(toLight, toLight), 1e-8));
    float attenuation = 1.0 / distanceSquared; // candela / m^2 = lux
    if (light.range > 0.0)
    {
        float ratio = sqrt(distanceSquared) / light.range;
        attenuation *= saturate(1.0 - ratio * ratio * ratio * ratio);
    }
    if (light.type == kLightSpot)
    {
        float cosAngle = dot(-direction, light.direction);
        attenuation *= smoothstep(light.cosOuter, light.cosInner, cosAngle);
    }
    return light.intensity * attenuation * saturate(dot(normal, direction));
}

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
    Surface surface = loadSurface(frame, gConstants.gbuffer, pixel);
    float3 color;
    if (surface.isSky)
    {
        float2 uv = (float2(pixel) + 0.5) * frame->inverseRenderSize;
        color = environmentRadiance(frame, cameraRayDirection(frame, uv));
    }
    else
    {
        float3 irradiance = float3(0.0);
        if (frame->hasSun != 0)
        {
            irradiance += frame->sunColor * frame->sunIlluminance * saturate(dot(surface.normal, frame->sunDirection));
        }
        for (uint i = 0; i < frame->lightCount; ++i)
        {
            irradiance += punctualLight(frame->lights[i], surface.position, surface.normal);
        }
        // Lambert: radiance = albedo / pi * irradiance. A uniform environment of radiance L gives irradiance pi * L.
        float3 diffuse = surface.albedo * (irradiance * kInvPi + environmentAverageRadiance(frame));
        color = diffuse + surface.emissive;
    }
    gStorageImages[gConstants.output][pixel] = float4(color, 1.0);
}
