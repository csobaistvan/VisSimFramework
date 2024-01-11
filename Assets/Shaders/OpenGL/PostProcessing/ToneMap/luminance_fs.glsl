#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/ToneMap/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec2 outLuminance;

void main()
{
    // Compute the current and previous luminance
    
    const float currentLuminance = max(computeLuminance(gbufferAlbedo(vUv)), 0.0f);
    const float lastLuminance = extractLuminance(ivec2(gl_FragCoord.xy));

    // Return the adapted luminance
    outLuminance = vec2(computeAdaptedLuminance(currentLuminance, lastLuminance));
}