#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/ComplexBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    const vec3 color = gbufferAlbedo(vUv, 0, 0);
    const float depth = gbufferDepth(vUv, 0, 0);
    const float blurRadius = computeBlurRadius(gl_FragCoord.xy, depth);
    colorBuffer = vec4(color, blurRadius);
}