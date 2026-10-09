// Naive reference path tracer: one path per pixel per frame, accumulated while nothing changes. Deliberately simple
// (cosine-sampled Lambert, fixed epsilon offset, no light sampling, no MIS): it proves the ray tracing plumbing and
// is the harness the user upgrades into the real reference in M2. Every naive spot carries an RTG-TODO.
import Bindless;
import Environment;
import Material;
import Math;
import Random;
import Scene;

struct PathTracerConstants
{
    FrameConstants* frame;
    uint accumulation; // storage index: running mean (RGBA32F)
    uint output;       // storage index: scene.color
    uint sampleIndex;  // samples already accumulated (0 = restart)
    uint maxBounces;
    uint traceSample;  // 0 = converged: only copy the mean to the output
    uint padding;
};

[[vk::push_constant]] ConstantBuffer<PathTracerConstants> gConstants;

struct PathPayload
{
    float hitT; // < 0: miss
    uint instance;
    uint geometry;
    uint triangle;
    float2 barycentrics;
};

// RTG-TODO(RTG1 ch.16): replace with your sample transformations; this is the textbook cosine-weighted hemisphere.
float3 sampleCosineHemisphere(float3 normal, float2 u)
{
    float r = sqrt(u.x);
    float phi = kTwoPi * u.y;
    float3 local = float3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - u.x)));
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    return normalize(tangent * local.x + bitangent * local.y + normal * local.z);
}

// The sun as a disc of the right solid angle, so a path that happens to point at it picks up its light.
// RTG-TODO(RTG2 ch.20): sample the sun (and emissive surfaces) explicitly and combine with BSDF sampling via MIS.
float3 sunDisc(FrameConstants* frame, float3 direction)
{
    if (frame->hasSun == 0 || frame->sunAngularRadius <= 0.0)
    {
        return float3(0.0);
    }
    float cosRadius = cos(frame->sunAngularRadius);
    if (dot(direction, frame->sunDirection) < cosRadius)
    {
        return float3(0.0);
    }
    float solidAngle = kTwoPi * (1.0 - cosRadius);
    return frame->sunColor * frame->sunIlluminance / solidAngle;
}

[shader("raygeneration")]
void rayGeneration()
{
    FrameConstants* frame = gConstants.frame;
    uint2 pixel = DispatchRaysIndex().xy;
    if (gConstants.traceSample == 0)
    {
        gStorageImages[gConstants.output][pixel] = float4(min(gStorageImages[gConstants.accumulation][pixel].rgb, 65000.0), 1.0);
        return;
    }
    Rng rng = makeRng(pixel, gConstants.sampleIndex);

    float2 uv = (float2(pixel) + rng.next2()) * frame->inverseRenderSize; // box-filtered pixel jitter
    RayDesc ray;
    ray.Origin = frame->cameraPosition;
    ray.Direction = cameraRayDirection(frame, uv);
    ray.TMin = 0.0;
    ray.TMax = 1e30;

    float3 radiance = float3(0.0);
    float3 throughput = float3(1.0);
    for (uint bounce = 0; bounce <= gConstants.maxBounces; ++bounce)
    {
        PathPayload payload;
        // sbtRecordStride 0: every geometry of every BLAS uses hit group 0 (the hit record index is
        // instanceOffset + sbtRecordOffset + stride * GeometryIndex(), and Sponza's BLAS has 103 geometries).
        TraceRay(sceneTlas(frame), RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
        if (payload.hitT < 0.0)
        {
            radiance += throughput * (environmentRadiance(frame, ray.Direction) + sunDisc(frame, ray.Direction));
            break;
        }

        HitSurface hit = loadHitSurface(frame, payload.instance, payload.geometry, payload.triangle, payload.barycentrics);
        // RTG-TODO(RTG1 ch.20): choose the texture LOD with ray cones; mip 0 aliases on distant surfaces.
        MaterialSample material = sampleMaterialLevel(frame, hit.material, hit.uv, 0.0);
        applyInstanceOverrides(frame->instances[hit.instance], material);

        // Shade the side the ray arrived from (single-sided back faces too: they're rare inside closed meshes).
        bool frontFace = dot(ray.Direction, hit.geometricNormal) < 0.0;
        float3 geometricNormal = frontFace ? hit.geometricNormal : -hit.geometricNormal;
        float3 normal = frontFace ? hit.normal : -hit.normal;
        if (material.hasNormalMap)
        {
            normal = applyNormalMap(normal, hit.tangent, material.tangentNormal);
        }
        if (dot(normal, geometricNormal) < 0.0)
        {
            normal = geometricNormal; // shading normal below the surface: fall back to the triangle's
        }

        radiance += throughput * material.emissive;
        if (bounce == gConstants.maxBounces)
        {
            break;
        }

        // Lambert BSDF sampled with pdf cos/pi: the cosine and pi cancel, leaving the albedo as the path weight.
        // RTG-TODO(RTG1 ch.15): importance-sample a GGX specular lobe too (metals and glossy surfaces look matte here).
        // RTG-TODO(RTG1 ch.17): Russian roulette and firefly clamping (paths always run to maxBounces here).
        throughput *= material.baseColor;
        ray.Direction = sampleCosineHemisphere(normal, rng.next2());
        // RTG-TODO(RTG1 ch.6): fast and robust self-intersection avoidance instead of a fixed epsilon.
        ray.Origin = hit.position + geometricNormal * 1e-3;
        ray.TMin = 0.0;
        ray.TMax = 1e30;
    }

    // Running mean: stays precise for any number of samples (a plain sum would lose low bits).
    float4 previous = gConstants.sampleIndex == 0 ? float4(0.0) : gStorageImages[gConstants.accumulation][pixel];
    float4 mean = previous + (float4(radiance, 1.0) - previous) / float(gConstants.sampleIndex + 1);
    gStorageImages[gConstants.accumulation][pixel] = mean;
    gStorageImages[gConstants.output][pixel] = float4(min(mean.rgb, 65000.0), 1.0); // stay finite in RGBA16F
}

[shader("miss")]
void miss(inout PathPayload payload)
{
    payload.hitT = -1.0;
}

[shader("closesthit")]
void closestHit(inout PathPayload payload, BuiltInTriangleIntersectionAttributes attributes)
{
    payload.hitT = RayTCurrent();
    payload.instance = InstanceID();
    payload.geometry = GeometryIndex();
    payload.triangle = PrimitiveIndex();
    payload.barycentrics = attributes.barycentrics;
}

// Runs only for geometry flagged non-opaque (alpha-masked materials).
[shader("anyhit")]
void anyHitAlphaTest(inout PathPayload payload, BuiltInTriangleIntersectionAttributes attributes)
{
    FrameConstants* frame = gConstants.frame;
    HitSurface hit = loadHitSurface(frame, InstanceID(), GeometryIndex(), PrimitiveIndex(), attributes.barycentrics);
    if (alphaTestFails(frame, hit.material, hit.uv, 0.0))
    {
        IgnoreHit();
    }
}
