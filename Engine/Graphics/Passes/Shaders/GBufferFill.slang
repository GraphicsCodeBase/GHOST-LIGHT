// G-buffer fill: rasterizes every scene draw with vertex pulling (no vertex buffers: vertices are read from the scene's global
// buffer by address) and writes what ShaderLibrary/GBuffer.slang decodes, plus motion vectors and entity IDs.
import Material;
import Packing;
import Scene;

struct GBufferConstants
{
    FrameConstants* frame;
};

[[vk::push_constant]] ConstantBuffer<GBufferConstants> gConstants;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
    float4 currentClip : TEXCOORD1;
    float4 previousClip : TEXCOORD2;
    nointerpolation uint instance : INSTANCE0;
    nointerpolation uint material : MATERIAL0;
};

struct GBufferOutput
{
    float4 normal : SV_Target0;   // xy shading, zw geometric (octahedral)
    float4 albedo : SV_Target1;
    float2 material : SV_Target2; // metallic, roughness
    float4 emissive : SV_Target3;
    float2 motion : SV_Target4;   // previous UV - current UV
    uint entityId : SV_Target5;
};

// SV_VulkanVertexID includes the draw's vertexOffset (= the primitive's first vertex), so it indexes the global vertex
// buffer directly. SV_VulkanInstanceID includes firstInstance, which the GPU scene sets to the draw index.
[shader("vertex")]
VertexOutput vertexMain(uint vertexIndex : SV_VulkanVertexID, uint drawIndex : SV_VulkanInstanceID)
{
    FrameConstants* frame = gConstants.frame;
    GpuDraw draw = frame->draws[drawIndex];
    Vertex vertex = frame->vertices[vertexIndex];
    float4x4 world = frame->instances[draw.instance].world;
    float4x4 previousWorld = frame->instances[draw.instance].previousWorld;
    float4x4 normalMatrix = frame->instances[draw.instance].normalMatrix;

    float4 worldPosition = mul(world, float4(vertex.position, 1.0));
    VertexOutput output;
    output.position = mul(frame->viewProjection, worldPosition);
    output.worldPosition = worldPosition.xyz;
    output.normal = mul((float3x3)normalMatrix, vertex.normal);
    output.tangent = float4(mul((float3x3)world, vertex.tangent.xyz), vertex.tangent.w);
    output.uv = vertex.uv;
    output.currentClip = output.position;
    output.previousClip = mul(frame->previousViewProjection, mul(previousWorld, float4(vertex.position, 1.0)));
    output.instance = draw.instance;
    output.material = frame->primitives[draw.primitive].material;
    return output;
}

GBufferOutput shade(VertexOutput input, bool frontFace, bool alphaTest)
{
    FrameConstants* frame = gConstants.frame;
    MaterialSample material = sampleMaterial(frame, input.material, input.uv);
    if (alphaTest && material.alpha < material.alphaCutoff)
    {
        discard;
    }
    GpuInstance instance = frame->instances[input.instance];
    applyInstanceOverrides(instance, material);

    // Back faces only reach here for double-sided materials: shade them with the flipped normal.
    float3 vertexNormal = normalize(input.normal) * (frontFace ? 1.0 : -1.0);
    float3 normal = material.hasNormalMap ? applyNormalMap(vertexNormal, input.tangent, material.tangentNormal) : vertexNormal;
    // Triangle normal from screen-space derivatives, on the same side as the vertex normal.
    float3 geometricNormal = normalize(cross(ddy(input.worldPosition), ddx(input.worldPosition)));
    if (dot(geometricNormal, vertexNormal) < 0.0)
    {
        geometricNormal = -geometricNormal;
    }

    float2 currentUv = input.currentClip.xy / input.currentClip.w * 0.5 + 0.5;
    float2 previousUv = input.previousClip.xy / input.previousClip.w * 0.5 + 0.5;

    GBufferOutput output;
    output.normal = float4(octahedralEncode(normal), octahedralEncode(geometricNormal));
    output.albedo = float4(material.baseColor, 1.0);
    output.material = float2(material.metallic, material.roughness);
    output.emissive = float4(material.emissive, 1.0);
    output.motion = previousUv - currentUv;
    output.entityId = instance.entityId;
    return output;
}

[shader("fragment")]
GBufferOutput fragmentOpaque(VertexOutput input, bool frontFace : SV_IsFrontFace)
{
    return shade(input, frontFace, false);
}

[shader("fragment")]
GBufferOutput fragmentMasked(VertexOutput input, bool frontFace : SV_IsFrontFace)
{
    return shade(input, frontFace, true);
}
