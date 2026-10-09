// G-buffer decoding: turns the G-buffer textures of one pixel into a Surface. Import with `import GBuffer;`.
// Layout (written by Engine/Graphics/Passes/Shaders/GBufferFill.slang):
//   gbuffer.depth    D32       reverse-Z (1 = near, 0 = sky)
//   gbuffer.normal   RGBA16S   xy = shading normal, zw = geometric normal (octahedral)
//   gbuffer.albedo   RGBA8 sRGB
//   gbuffer.material RG8       r = metallic, g = roughness
//   gbuffer.emissive RGBA16F   emitted radiance
//   gbuffer.motion   RG16F     previous UV - current UV
//   gbuffer.entityId R32U      entity + 1 (0 = sky)
module GBuffer;

import Bindless;
import Packing;
import Scene;

public struct GBufferIndices
{
    public uint depth;
    public uint normal;
    public uint albedo;
    public uint material;
    public uint emissive;
};

public struct Surface
{
    public float3 position;
    public float3 normal;
    public float3 geometricNormal;
    public float3 albedo;
    public float metallic;
    public float roughness;
    public float3 emissive;
    public float depth;
    public bool isSky;
};

public Surface loadSurface(FrameConstants* frame, GBufferIndices textures, int2 pixel)
{
    Surface surface;
    surface.depth = loadTexture(textures.depth, pixel).r;
    surface.isSky = surface.depth <= 0.0;
    float2 uv = (float2(pixel) + 0.5) * frame->inverseRenderSize;
    surface.position = reconstructWorldPosition(frame, uv, surface.depth);
    float4 normals = loadTexture(textures.normal, pixel);
    surface.normal = octahedralDecode(normals.xy);
    surface.geometricNormal = octahedralDecode(normals.zw);
    surface.albedo = loadTexture(textures.albedo, pixel).rgb;
    float2 material = loadTexture(textures.material, pixel).rg;
    surface.metallic = material.r;
    surface.roughness = material.g;
    surface.emissive = loadTexture(textures.emissive, pixel).rgb;
    return surface;
}
