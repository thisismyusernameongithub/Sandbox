#version 300 es
precision highp float;

in vec2 texCoord;

uniform sampler2D depthTexture;
uniform sampler2D flowTexture;
uniform sampler2D velocityTexture;
uniform sampler2D terrainTexture;

uniform float g;
uniform float visc;
uniform float l;
uniform int w;
uniform int h;
uniform float friction;
uniform float dTime;

layout(location = 0) out float outDepth;
layout(location = 1) out vec4 outFlow;
layout(location = 2) out vec2 outVelocity;

void main()
{
    ivec2 coords = ivec2(texCoord * vec2(w, h));

    float depth = texelFetch(depthTexture, coords, 0).r;
    vec4 flow = texelFetch(flowTexture, coords, 0);
    float terrain = texelFetch(terrainTexture, coords, 0).r;

    int x = coords.x;
    int y = coords.y;

    float friction_dTime = 1.0 - dTime * (1.0 - friction);

    // Fetch neighboring depths and terrain heights
    float depthRight = texelFetch(depthTexture, ivec2(x + 1, y), 0).r;
    float terrainRight = texelFetch(terrainTexture, ivec2(x + 1, y), 0).r;
    
    float depthDown = texelFetch(depthTexture, ivec2(x, y + 1), 0).r;
    float terrainDown = texelFetch(terrainTexture, ivec2(x, y + 1), 0).r;
    
    float depthLeft = texelFetch(depthTexture, ivec2(x - 1, y), 0).r;
    float terrainLeft = texelFetch(terrainTexture, ivec2(x - 1, y), 0).r;
    
    float depthUp = texelFetch(depthTexture, ivec2(x, y - 1), 0).r;
    float terrainUp = texelFetch(terrainTexture, ivec2(x, y - 1), 0).r;

    float flowFromLeft  = texelFetch(flowTexture, ivec2(x - 1, y), 0).r;
    float flowFromUp    = texelFetch(flowTexture, ivec2(x, y - 1), 0).g;
    float flowFromRight = texelFetch(flowTexture, ivec2(x + 1, y), 0).b;
    float flowFromDown  = texelFetch(flowTexture, ivec2(x, y + 1), 0).a;

    float A = l * l;
    
    // Compute flow
    flow.r = max(flow.r * friction_dTime + (depth + terrain - depthRight - terrainRight) * dTime * A * g / l, 0.0);
    flow.g = max(flow.g * friction_dTime + (depth + terrain - depthDown - terrainDown) * dTime * A * g / l, 0.0);
    flow.b = max(flow.b * friction_dTime + (depth + terrain - depthLeft - terrainLeft) * dTime * A * g / l, 0.0);
    flow.a = max(flow.a * friction_dTime + (depth + terrain - depthUp - terrainUp) * dTime * A * g / l, 0.0);

    if (visc > 0.0) {
        float V = (depth * depth) / ((depth * depth) + 3.0 * visc * dTime);
        flow *= V;
    }

    // make sure flow out of cell isn't greater than inflow + existing fluid
    if (depth - (flow.r + flow.g + flow.b + flow.a) < 0.0) {
        float K = min(depth * A / ((flow.r + flow.g + flow.b + flow.a) * dTime), 1.0);
        flow.r *= K;
        flow.g *= K;
        flow.b *= K;
        flow.a *= K;
    }

        // Update depth
    float deltaV = (
        (flowFromDown + flowFromLeft + flowFromRight + flowFromUp) -
        (flow.r + flow.g + flow.b + flow.a)
    ) * dTime;

    outDepth = max(depth + (deltaV / A), 0.0);

    // Write outputs
    
    outFlow = flow;   // Example
    // outVelocity = velocity; // Example
}