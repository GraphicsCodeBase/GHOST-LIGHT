// Scene data every pass and technique reads: the GPU struct layouts (mirror of Engine/Graphics/GpuScene/GpuSceneTypes.h),
// reached through the frame constants' buffer device addresses, plus hit-surface reconstruction for ray tracing.
// Import with `import Scene;`.
module Scene;

import Bindless;

public static const uint kNoTexture = 0xFFFFFFFF;
public static const uint kMaterialAlphaMask = 1;
public static const uint kMaterialDoubleSided = 2;
public static const uint kOverrideBaseColor = 1;
public static const uint kOverrideRoughness = 2;
public static const uint kOverrideMetallic = 4;
public static const uint kOverrideEmissive = 8;
public static const uint kLightPoint = 0;
public static const uint kLightSpot = 1;

public struct Vertex
{
    public float3 position;
    public float3 normal;
    public float4 tangent; // xyz tangent, w bitangent sign
    public float2 uv;
};

public struct GpuPrimitive
{
    public uint firstIndex;
    public uint indexCount;
    public uint firstVertex;
    public uint material;
};

public struct GpuMaterial
{
    public float4 baseColorFactor;
    public float3 emissive;
    public float metallic;
    public float roughness;
    public float alphaCutoff;
    public float normalScale;
    public float transmission;
    public uint baseColorTexture;
    public uint metallicRoughnessTexture;
    public uint normalTexture;
    public uint emissiveTexture;
    public uint flags;
    public uint3 padding;
};

public struct GpuInstance
{
    public float4x4 world;
    public float4x4 previousWorld;
    public float4x4 normalMatrix;
    public uint entityId;
    public uint firstPrimitive;
    public uint primitiveCount;
    public uint overrideFlags;
    public float3 baseColor;
    public float roughness;
    public float3 emissive;
    public float metallic;
};

public struct GpuDraw
{
    public uint instance;
    public uint primitive;
};

public struct GpuLight
{
    public float3 position;
    public uint type;
    public float3 direction;
    public float range;
    public float3 intensity;
    public float radius;
    public float cosInner;
    public float cosOuter;
    public uint2 padding;
};

public struct FrameConstants
{
    public float4x4 view;
    public float4x4 projection;
    public float4x4 viewProjection;
    public float4x4 inverseViewProjection;
    public float4x4 previousViewProjection;
    public float3 cameraPosition;
    public uint frameIndex;
    public float2 renderSize;
    public float2 inverseRenderSize;
    public float3 sunDirection;
    public float sunIlluminance;
    public float3 sunColor;
    public float sunAngularRadius;
    public float3 environmentAverage;
    public float environmentIntensity;
    public uint environmentTexture;
    public float exposure;
    public uint lightCount;
    public uint instanceCount;
    public Vertex* vertices;
    public uint* indices;
    public GpuPrimitive* primitives;
    public GpuMaterial* materials;
    public GpuInstance* instances;
    public GpuDraw* draws;
    public GpuLight* lights;
    public uint hasSun;
    public uint tlasIndex; // gSceneTlas[tlasIndex] (Bindless): this frame's TLAS
    public uint blueNoiseTexture; // 128x128 RGBA8 blue noise or kNoTexture: use Random's blueNoise()
    public uint padding0;
};

// Equirectangular environment mapping: +Y up, u = 0.5 at -Z.
public float2 equirectUv(float3 direction)
{
    float u = 0.5 + atan2(direction.x, -direction.z) / (2.0 * 3.14159265358979);
    float v = acos(clamp(direction.y, -1.0, 1.0)) / 3.14159265358979;
    return float2(u, v);
}

// World-space position from a depth-buffer value (reverse-Z) and a screen UV. Depth must be > 0 (not sky).
public float3 reconstructWorldPosition(FrameConstants* frame, float2 uv, float depth)
{
    float4 clip = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 world = mul(frame->inverseViewProjection, clip);
    return world.xyz / world.w;
}

// Direction of the camera ray through a screen UV (world space, normalized). Depth 0 is the point at infinity, whose
// homogeneous world position is exactly that direction (w = 0).
public float3 cameraRayDirection(FrameConstants* frame, float2 uv)
{
    float4 world = mul(frame->inverseViewProjection, float4(uv * 2.0 - 1.0, 0.0, 1.0));
    return normalize(world.xyz);
}

// This frame's top-level acceleration structure: every instance of the GPU scene, custom index = instance index.
public RaytracingAccelerationStructure sceneTlas(FrameConstants* frame)
{
    return gSceneTlas[frame->tlasIndex];
}

// A ray hit turned back into a surface.
public struct HitSurface
{
    public float3 position;        // world space
    public float3 geometricNormal; // the triangle's normal (counter-clockwise front), world space
    public float3 normal;          // interpolated vertex normal, world space (no normal map)
    public float4 tangent;         // xyz world tangent, w bitangent sign
    public float2 uv;
    public uint material;
    public uint instance;
};

// Arguments come straight from the hit: InstanceID() (the TLAS custom index = instance index), GeometryIndex() (the
// primitive within the mesh), PrimitiveIndex() (the triangle) and the barycentrics of the hit attributes.
public HitSurface loadHitSurface(FrameConstants* frame, uint instanceIndex, uint geometryIndex, uint triangleIndex, float2 barycentrics)
{
    GpuInstance instance = frame->instances[instanceIndex];
    GpuPrimitive primitive = frame->primitives[instance.firstPrimitive + geometryIndex];
    uint base = primitive.firstIndex + triangleIndex * 3;
    Vertex v0 = frame->vertices[primitive.firstVertex + frame->indices[base + 0]];
    Vertex v1 = frame->vertices[primitive.firstVertex + frame->indices[base + 1]];
    Vertex v2 = frame->vertices[primitive.firstVertex + frame->indices[base + 2]];
    float3 w = float3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    float3x3 normalMatrix = (float3x3)instance.normalMatrix;

    HitSurface surface;
    float3 localPosition = v0.position * w.x + v1.position * w.y + v2.position * w.z;
    surface.position = mul(instance.world, float4(localPosition, 1.0)).xyz;
    surface.normal = normalize(mul(normalMatrix, v0.normal * w.x + v1.normal * w.y + v2.normal * w.z));
    float3 geometric = mul(normalMatrix, cross(v1.position - v0.position, v2.position - v0.position));
    surface.geometricNormal = dot(geometric, geometric) > 0.0 ? normalize(geometric) : surface.normal; // degenerate triangle
    float3 tangent = v0.tangent.xyz * w.x + v1.tangent.xyz * w.y + v2.tangent.xyz * w.z;
    surface.tangent = float4(mul((float3x3)instance.world, tangent), v0.tangent.w);
    surface.uv = v0.uv * w.x + v1.uv * w.y + v2.uv * w.z;
    surface.material = primitive.material;
    surface.instance = instanceIndex;
    return surface;
}
