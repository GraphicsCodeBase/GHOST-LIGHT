// Random numbers for shaders: a PCG hash for seeds, a small per-pixel generator and blue noise lookups.
// Import with `import Random;`.
module Random;

import Bindless;

// PCG-RXS-M-XS 32-bit hash (Jarzynski & Olano 2020): good avalanche, cheap.
public uint pcgHash(uint value)
{
    uint state = value * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

public struct Rng
{
    public uint state;

    // Uniform in [0, 1).
    [mutating]
    public float next()
    {
        state = pcgHash(state);
        return float(state >> 8) * (1.0 / 16777216.0);
    }

    [mutating]
    public float2 next2()
    {
        float a = next();
        float b = next();
        return float2(a, b);
    }
};

// A different sequence per pixel and per sample index.
public Rng makeRng(uint2 pixel, uint sampleIndex)
{
    Rng rng;
    rng.state = pcgHash(pixel.x + pcgHash(pixel.y + pcgHash(sampleIndex)));
    return rng;
}

// Blue noise: 128x128 RGBA8 texture (Christoph Peters, CC0), index in FrameConstants.blueNoiseTexture. Four independent
// blue noise channels per pixel, tiled over the screen. Pass frameIndex to animate it: each frame adds the golden ratio
// (a Cranley-Patterson rotation), so every frame stays blue noise in space and the frames are well spread in time.
// Without the texture (kNoTexture = 0xFFFFFFFF) it falls back to white noise so shaders keep working.
public static const uint kBlueNoiseSize = 128;

public float4 blueNoise(uint texture, uint2 pixel, uint frameIndex)
{
    float4 value;
    if (texture == 0xFFFFFFFFu)
    {
        Rng rng = makeRng(pixel, frameIndex);
        value = float4(rng.next2(), rng.next2());
    }
    else
    {
        value = loadTexture(texture, int2(pixel % kBlueNoiseSize));
    }
    const float goldenRatioConjugate = 0.61803398875;
    return frac(value + float(frameIndex % 4096u) * goldenRatioConjugate);
}