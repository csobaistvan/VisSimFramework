#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/ToneMap/common.glsl>

// Input attribus
in GeometryData
{
    vec2 vUv;
} g_out;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    // Load the average luminance
    const vec2 L_global = extractLuminance(g_out.vUv, sTonemapData.fNumMipLevels);
    const vec2 L_local = extractLuminance(g_out.vUv, sTonemapData.fLocalMipLevel);
    const float L_global_avg = L_global.x;
    const float L_global_max = L_global.y;
    const float L_local_avg = L_local.x;
    const float L_ratio = saturate(abs(L_global_max - L_global_avg) / L_global_max);
    const float L_local_contrib = min(L_ratio, sTonemapData.fMaxLocalContribution);
    const float avgLuminance = mix(L_global_avg, L_local_avg, L_local_contrib);

    // Go through the processing chain
    vec3 color;
    /* Initial color. */ color = gbufferAlbedo(g_out.vUv, gl_Layer, 0);
    /* Tone map */       color = toneMap(color, avgLuminance);
    /* Dodge & burn */   color = applyDodgeBurn(color);
    /* Color table */    color = applyColorTable(color);
    /* Linear-sRGB */    color = saturate(linearToSrgb(color));

    // Write out the result
    colorBuffer.rgb = color;
    colorBuffer.a = 1.0;
}