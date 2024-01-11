#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>

//////////////////////////////////////////////////
//  Parameters
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform MotionBlurParameters
{
    float fVelocityScale;
    float fMaxVelocity;
    int iNumTaps;
    int iTileSize;
    float fCenterWeight;
    float fTileFalloff;
    float fInterpolationThreshold;
    float fJitterScale;
} sMotionBlurData;

//////////////////////////////////////////////////
// Various texture maps
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sTileMax;
layout (binding = TEXTURE_POST_PROCESS_2) uniform sampler2D sNeighborMax;

////////////////////////////////////////////////////////////////////////////////
vec3 unpackVelocityData(const vec2 originalVelocity)
{
    //////////////////////////////////////////////////
    //  Halve it, scale it and take it to pixel space
    const vec2 scaledVelocity = (originalVelocity * sMotionBlurData.fVelocityScale * 0.5) * sRenderData.vMaxResolution;
    
    //////////////////////////////////////////////////
    //  Clamp the velocity
    const float vl = length(scaledVelocity);
    const float weight = max(min(vl, sMotionBlurData.fMaxVelocity), 0.5) / max(vl, 1e-4);
    const vec2 clampedVelocity = scaledVelocity * weight;
    
    // Return the result
    return vec3(clampedVelocity, max(length(clampedVelocity), 1e-4));
}

////////////////////////////////////////////////////////////////////////////////
vec3 packVelocityData(const vec2 velocity)
{
    return vec3(velocity, max(length(velocity), 1e-4));
}

////////////////////////////////////////////////////////////////////////////////
vec3 velocityMax(const vec3 tmp, const vec3 mx)
{
    const float cmp = gt(tmp.z, mx.z);
 
    return tmp * cmp + mx * (1.0 - cmp);
}

////////////////////////////////////////////////////////////////////////////////
vec2 jitterCoords(const vec2 uv, const float jitter, vec2 pixelSize)
{
    //////////////////////////////////////////////////
    //  Get integral coords
    const ivec2 iUV = ivec2(uv * sRenderData.vMaxResolution.xy);
    
    //////////////////////////////////////////////////
    //  Integral coords inside the tile
    const ivec2 iTileUV = iUV % sMotionBlurData.iTileSize;
    
    //////////////////////////////////////////////////
    //  Distance from tile center (in pixels)
    const int halfTileSize = int(sMotionBlurData.iTileSize * 0.5);
    const ivec2 iTileUVDist = abs(ivec2(halfTileSize) - iTileUV);
    
    //////////////////////////////////////////////////
    //  Normalized tile coords
    const vec2 vTileUV = vec2(iTileUVDist) / vec2(halfTileSize);
    
    //////////////////////////////////////////////////
    //  Weight
    const float falloff = length(vTileUV) * sMotionBlurData.fTileFalloff;
    
    // Return the result
    return uv + jitter * falloff * pixelSize;
}

////////////////////////////////////////////////////////////////////////////////
float cone(const float T, const float invMagnitude)
{
    return saturate(1.0 - T * invMagnitude);
}

////////////////////////////////////////////////////////////////////////////////
float cylinder(const float T, const float magnitude)
{
    return 1.0 - smoothstep(0.95 * magnitude, 1.05 * magnitude, T);
}

////////////////////////////////////////////////////////////////////////////////
float zCompare(const float a, const float b)
{
    return saturate(1.0 - (a - b) / max(min(a, b), 1e-4));
}