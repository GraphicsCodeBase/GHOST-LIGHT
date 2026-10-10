// RayQueryNormals: a camera ray per pixel traced inline (RayQuery) against the scene TLAS; the hit's normal becomes a color.
import Bindless;
import Material;
import Scene;

struct Constants
{
    FrameConstants* frame;
    uint color;            // storage index of scene.color
    float blend;           // 0 = lit image, 1 = normals
    uint geometricNormals; // 1: triangle normals, 0: interpolated vertex normals
    uint alphaTest;        // 1: alpha-masked texels let the ray through (leaves, fences)
};

[[vk::push_constant]] ConstantBuffer<Constants> gConstants;

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
    float2 uv = (float2(pixel) + 0.5) * frame->inverseRenderSize;
    RayDesc ray;
    ray.Origin = frame->cameraPosition;
    ray.Direction = cameraRayDirection(frame, uv);
    ray.TMin = 0.0;
    ray.TMax = 1e30;

    RayQuery<RAY_FLAG_NONE> query;
    query.TraceRayInline(sceneTlas(frame), gConstants.alphaTest != 0 ? RAY_FLAG_NONE : RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);
    // Proceed() stops only at non-opaque candidates (alpha-masked geometry): commit them where the texel is solid.
    while (query.Proceed())
    {
        HitSurface candidate = loadHitSurface(frame, query.CandidateInstanceID(), query.CandidateGeometryIndex(),
                                              query.CandidatePrimitiveIndex(), query.CandidateTriangleBarycentrics());
        if (!alphaTestFails(frame, candidate.material, candidate.uv, 0.0))
        {
            query.CommitNonOpaqueTriangleHit();
        }
    }

    float3 normalColor = float3(0.0); // miss: black
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        HitSurface hit = loadHitSurface(frame, query.CommittedInstanceID(), query.CommittedGeometryIndex(),
                                        query.CommittedPrimitiveIndex(), query.CommittedTriangleBarycentrics());
        float3 normal = gConstants.geometricNormals != 0 ? hit.geometricNormal : hit.normal;
        normalColor = normal * 0.5 + 0.5;
    }
    // scene.color holds radiance; dividing by the exposure makes the tonemapper show the debug color at roughly its value.
    float4 lit = gStorageImages[gConstants.color][pixel];
    gStorageImages[gConstants.color][pixel] = float4(lerp(lit.rgb, normalColor / frame->exposure, gConstants.blend), 1.0);
}
