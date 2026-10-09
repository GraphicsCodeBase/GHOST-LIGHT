// Environment lighting shared by raster lighting and the path tracer: the scene's HDRI (or a procedural daylight sky
// when it has none) times the scene's environment intensity. Import with `import Environment;`.
module Environment;

import Bindless;
import Scene;

// Procedural fallback, in nits: bright horizon, blue zenith, grey ground. Roughly a clear daylight sky dome.
float3 proceduralSky(float3 direction)
{
    const float3 horizon = float3(4500.0, 4800.0, 5200.0);
    const float3 zenith = float3(1500.0, 2400.0, 4500.0);
    const float3 ground = float3(800.0, 750.0, 700.0);
    if (direction.y < 0.0)
    {
        return lerp(horizon * 0.5, ground, saturate(-direction.y * 4.0));
    }
    return lerp(horizon, zenith, pow(saturate(direction.y), 0.6));
}

// Average of proceduralSky over the sphere (precomputed), so ambient terms don't integrate it every pixel.
static const float3 kProceduralSkyAverage = float3(2100.0, 2500.0, 3300.0);

// Radiance arriving from `direction` (world space, normalized).
public float3 environmentRadiance(FrameConstants* frame, float3 direction)
{
    if (frame->environmentTexture == kNoTexture)
    {
        return proceduralSky(direction) * frame->environmentIntensity;
    }
    float2 uv = equirectUv(direction);
    return sampleTextureLevel(frame->environmentTexture, kSamplerLinearRepeat, uv, 0.0).rgb * frame->environmentIntensity;
}

// Radiance averaged over the whole sphere: a constant ambient term for placeholder lighting.
public float3 environmentAverageRadiance(FrameConstants* frame)
{
    float3 average = frame->environmentTexture == kNoTexture ? kProceduralSkyAverage : frame->environmentAverage;
    return average * frame->environmentIntensity;
}
