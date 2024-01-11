#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/MotionBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec2 velocityBuffer;

void main()
{
    // Constants
    const int tileSize = sMotionBlurData.iTileSize;
    const vec2 texelSize = 1.0 / sRenderData.vMaxResolution;

    ivec2 offset = ivec2(0, 0);
    vec3 maxVelocity = vec3(0);

    // Traverse each col
    for (int i = 0; i < tileSize; i++)
    {
        // Traverse each column
        for (int j = 0; j < tileSize; j++)
        {
            const vec2 uv = saturate(vUv + offset * texelSize);
            const vec2 velocity = gbufferVelocity(uv, 0, 0);

            maxVelocity = velocityMax(packVelocityData(velocity), maxVelocity);
            offset.x += 1;
        }
        
        offset.x = 0;
        offset.y += 1;
    }
    
    velocityBuffer = maxVelocity.xy;
}