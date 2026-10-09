// Material evaluation shared by raster and ray tracing: glTF factors x textures, the instance's overrides, normal
// mapping. Import with `import Material;`.
module Material;

import Bindless;
import Scene;

public struct MaterialSample
{
    public float3 baseColor;
    public float alpha;
    public float alphaCutoff;
    public float metallic;
    public float roughness;
    public float3 emissive;
    public float3 tangentNormal; // from the normal map, xy scaled by normalScale; (0, 0, 1) without one
    public bool hasNormalMap;
    public bool alphaMasked;
    public bool doubleSided;
};

// How textures are read: implicit derivatives (fragment shaders) or an explicit mip level (everything else).
interface ITextureReader
{
    float4 read(uint texture, float2 uv);
};

struct ImplicitLodReader : ITextureReader
{
    float4 read(uint texture, float2 uv) { return sampleTexture(texture, kSamplerAnisotropicRepeat, uv); }
};

struct ExplicitLodReader : ITextureReader
{
    float lod;
    float4 read(uint texture, float2 uv) { return sampleTextureLevel(texture, kSamplerLinearRepeat, uv, lod); }
};

MaterialSample evaluate<R : ITextureReader>(FrameConstants* frame, uint materialIndex, float2 uv, R reader)
{
    GpuMaterial material = frame->materials[materialIndex];
    MaterialSample result;
    float4 baseColor = material.baseColorFactor;
    if (material.baseColorTexture != kNoTexture)
    {
        baseColor *= reader.read(material.baseColorTexture, uv);
    }
    result.baseColor = baseColor.rgb;
    result.alpha = baseColor.a;
    result.alphaCutoff = material.alphaCutoff;
    result.alphaMasked = (material.flags & kMaterialAlphaMask) != 0;
    result.doubleSided = (material.flags & kMaterialDoubleSided) != 0;

    result.metallic = material.metallic;
    result.roughness = material.roughness;
    if (material.metallicRoughnessTexture != kNoTexture)
    {
        float4 sampled = reader.read(material.metallicRoughnessTexture, uv);
        result.roughness *= sampled.g;
        result.metallic *= sampled.b;
    }
    result.emissive = material.emissive;
    if (material.emissiveTexture != kNoTexture)
    {
        result.emissive *= reader.read(material.emissiveTexture, uv).rgb;
    }
    result.hasNormalMap = material.normalTexture != kNoTexture;
    result.tangentNormal = float3(0.0, 0.0, 1.0);
    if (result.hasNormalMap)
    {
        float3 sampled = reader.read(material.normalTexture, uv).xyz * 2.0 - 1.0;
        result.tangentNormal = float3(sampled.xy * material.normalScale, sampled.z);
    }
    return result;
}

// Fragment shaders: mip level from screen-space derivatives.
public MaterialSample sampleMaterial(FrameConstants* frame, uint materialIndex, float2 uv)
{
    ImplicitLodReader reader;
    return evaluate(frame, materialIndex, uv, reader);
}

// Ray tracing and compute shaders: an explicit mip level (no derivatives there).
public MaterialSample sampleMaterialLevel(FrameConstants* frame, uint materialIndex, float2 uv, float lod)
{
    ExplicitLodReader reader;
    reader.lod = lod;
    return evaluate(frame, materialIndex, uv, reader);
}

// Alpha only, for any-hit shaders and ray-query alpha tests. True when the hit should be ignored.
public bool alphaTestFails(FrameConstants* frame, uint materialIndex, float2 uv, float lod)
{
    GpuMaterial material = frame->materials[materialIndex];
    if ((material.flags & kMaterialAlphaMask) == 0)
    {
        return false;
    }
    float alpha = material.baseColorFactor.a;
    if (material.baseColorTexture != kNoTexture)
    {
        alpha *= sampleTextureLevel(material.baseColorTexture, kSamplerLinearRepeat, uv, lod).a;
    }
    return alpha < material.alphaCutoff;
}

// Per-entity MaterialOverride values (sandbox recoloring) replace the model's.
public void applyInstanceOverrides(GpuInstance instance, inout MaterialSample material)
{
    if ((instance.overrideFlags & kOverrideBaseColor) != 0)
    {
        material.baseColor = instance.baseColor;
    }
    if ((instance.overrideFlags & kOverrideRoughness) != 0)
    {
        material.roughness = instance.roughness;
    }
    if ((instance.overrideFlags & kOverrideMetallic) != 0)
    {
        material.metallic = instance.metallic;
    }
    if ((instance.overrideFlags & kOverrideEmissive) != 0)
    {
        material.emissive = instance.emissive;
    }
}

// Tangent-space normal -> world space. `normal` must already face the viewer for double-sided back faces.
public float3 applyNormalMap(float3 normal, float4 tangent, float3 tangentNormal)
{
    if (dot(tangent.xyz, tangent.xyz) < 1e-8)
    {
        return normal; // no tangents on this mesh
    }
    float3 t = normalize(tangent.xyz - normal * dot(normal, tangent.xyz));
    float3 b = cross(normal, t) * tangent.w;
    return normalize(t * tangentNormal.x + b * tangentNormal.y + normal * tangentNormal.z);
}
