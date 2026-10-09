// Fullscreen triangle: three vertices generated from SV_VertexID cover the screen with no vertex buffer. Shared by full-screen passes.
module Fullscreen;

public struct FullscreenVaryings
{
    public float4 position : SV_Position;
    public float2 uv : TEXCOORD0;
};

[shader("vertex")]
public FullscreenVaryings vertexMain(uint vertexId : SV_VertexID)
{
    // Vertex 0 -> uv (0,0), 1 -> (2,0), 2 -> (0,2): one oversized triangle, clipped to the screen.
    float2 uv = float2(float((vertexId << 1) & 2), float(vertexId & 2));
    FullscreenVaryings output;
    output.uv = uv;
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}
