// Random numbers for shaders: a PCG hash for seeds and a small per-pixel generator. Import with `import Random;`.
// Blue noise lookups join in step 9.
module Random;

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
