// Shared math helpers: constants and color-space conversion. Import with `import Math;`.
module Math;

public static const float kPi = 3.14159265358979323846;
public static const float kTwoPi = 6.28318530717958647692;
public static const float kInvPi = 0.31830988618379067154;

// Linear -> sRGB transfer function. The swapchain is UNORM, so passes that write to it encode explicitly.
public float3 linearToSrgb(float3 linear)
{
    float3 low = linear * 12.92;
    float3 high = 1.055 * pow(max(linear, 0.0), 1.0 / 2.4) - 0.055;
    return select(linear <= 0.0031308, low, high);
}

public float3 srgbToLinear(float3 srgb)
{
    float3 low = srgb / 12.92;
    float3 high = pow((srgb + 0.055) / 1.055, 2.4);
    return select(srgb <= 0.04045, low, high);
}
