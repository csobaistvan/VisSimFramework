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
    const float r_c = textureDR(sDownscaledBuffer, vUv, sComplexBlurData.vUvScale).a;
    const vec3 blurred = textureDR(sResultsBuffer, vUv, sComplexBlurData.vUvScale).rgb;
    const vec3 sharp = gbufferAlbedo(vUv, 0, 0);
    colorBuffer = vec4(lerp(sharp, blurred, saturate(abs(r_c))), 0.0);

    // =================================================================================================================

    // Debug outputs - simply pass through the blurred result if it containst debug info
    if (sComplexBlurData.uiOutputMode != OutputMode_Convolution)
    {
        colorBuffer = vec4(blurred, 1.0);
    }
}