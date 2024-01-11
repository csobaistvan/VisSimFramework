#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/MotionBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec2 velocityBuffer;

//////////////////////////////////////////////////
vec3 velocityMaxTileBuffer(const vec3 mx, const ivec2 uv, ivec2 offset)
{
    const vec2 velocity = texelFetch(sTileMax, uv + offset, 0).xy;
    const vec3 tmp = packVelocityData(velocity);

    return velocityMax(tmp, mx);
}

//////////////////////////////////////////////////
vec3 velocityMaxOffAxis(const vec3 mx, const ivec2 uv, const ivec2 offset)
{
    //////////////////////////////////////////////////
    //  Get velocity at this tile
    const vec2 velocity = texelFetch(sTileMax, uv + offset, 0).xy;
    const vec3 tmp = packVelocityData(velocity);
    
    //////////////////////////////////////////////////
    //  Compare it to the current max
    const float relation = gt(tmp.z, mx.z);
    
    //////////////////////////////////////////////////
    //  Get the angle between the current velocity and
    //  the vector pointing from the center to this tile
    const float angle = gt(dot(vec2(offset), tmp.xy / tmp.z), 0.0);
    
    //////////////////////////////////////////////////
    //  Sum the comparisons
    const float cmp = and(relation, angle);
    
    // Return the result
    return tmp * cmp + mx * (1.0 - cmp);
}

void main()
{
    const ivec2 texelCoord = ivec2(gl_FragCoord.xy);

    vec3 maxVelocity = vec3(0);
    
    maxVelocity = velocityMaxTileBuffer(maxVelocity, texelCoord, ivec2( 0,  0));
    maxVelocity = velocityMaxTileBuffer(maxVelocity, texelCoord, ivec2( 1,  0));
    maxVelocity = velocityMaxTileBuffer(maxVelocity, texelCoord, ivec2( 0,  1));
    maxVelocity = velocityMaxTileBuffer(maxVelocity, texelCoord, ivec2(-1,  0));
    maxVelocity = velocityMaxTileBuffer(maxVelocity, texelCoord, ivec2( 0, -1));
    
    maxVelocity = velocityMaxOffAxis(maxVelocity, texelCoord, ivec2( 1,  1));
    maxVelocity = velocityMaxOffAxis(maxVelocity, texelCoord, ivec2(-1,  1));
    maxVelocity = velocityMaxOffAxis(maxVelocity, texelCoord, ivec2( 1, -1));
    maxVelocity = velocityMaxOffAxis(maxVelocity, texelCoord, ivec2(-1, -1));
    
    velocityBuffer = maxVelocity.xy;
}