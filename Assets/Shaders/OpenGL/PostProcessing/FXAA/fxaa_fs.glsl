#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/FXAA/common.glsl>

// Input attribs
in GeometryData
{
    vec2 vUv;
} g_out;

// Render targets
layout (location = 0) out vec4 colorBuffer;

////////////////////////////////////////////////////////////////////////////////
//  http://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/
void main()
{
    //////////////////////////////////////////////////
    //  Compute the UV
    const vec2 texelSize = 1.0 / sRenderData.vResolution;
    const vec2 uv = g_out.vUv;

    //////////////////////////////////////////////////
    //  Sample surrounding pixels
    vec3 pixelBL = gbufferAlbedo(uv + (vec2(-1, -1) * texelSize), gl_Layer, 0);
    vec3 pixelBR = gbufferAlbedo(uv + (vec2( 1, -1) * texelSize), gl_Layer, 0);
    vec3 pixelTL = gbufferAlbedo(uv + (vec2(-1,  1) * texelSize), gl_Layer, 0);
    vec3 pixelTR = gbufferAlbedo(uv + (vec2( 1,  1) * texelSize), gl_Layer, 0);
    vec3 pixelM  = gbufferAlbedo(uv                             , gl_Layer, 0);
    
    //////////////////////////////////////////////////
    //  Compute their luminance
    float lumBL = computeLuminance(pixelBL);
    float lumBR = computeLuminance(pixelBR);
    float lumTL = computeLuminance(pixelTL);
    float lumTR = computeLuminance(pixelTR);
    float lumM = computeLuminance(pixelM);

    //////////////////////////////////////////////////
    //  Compute the blur direction
    vec2 dir;
    dir.x = ((lumTL + lumTR) - (lumBL + lumBR));
    dir.y = ((lumTL + lumBL) - (lumTR + lumBR));
    
    float epsilon = max(sFxaaData.fDirReduceMin, (lumBL + lumBR + lumTL + lumTR) * 0.25 * sFxaaData.fDirReduceMultiplier);
    float dirAdjustment = 1.0 / max(min(abs(dir.x), abs(dir.y)), epsilon);
    
    dir = clamp(dir * dirAdjustment, vec2(-sFxaaData.fMaxBlur), vec2(sFxaaData.fMaxBlur)) * texelSize;
    
    //////////////////////////////////////////////////
    //  Compute the two possible results
	vec3 result1 = 0.5 * (gbufferAlbedo((uv + (dir * vec2(1.0 / 3.0 - 0.5))), gl_Layer, 0).xyz +
                          gbufferAlbedo((uv + (dir * vec2(2.0 / 3.0 - 0.5))), gl_Layer, 0).xyz);
                      
	vec3 result2 = result1 * 0.5 + 0.25 * (gbufferAlbedo((uv + (dir * vec2(-0.5))), gl_Layer, 0).xyz +
                                           gbufferAlbedo((uv + (dir * vec2( 0.5))), gl_Layer, 0).xyz);

    //////////////////////////////////////////////////
    //  Choose the appropriate one
	float lumMin = min(lumM, min(min(lumTL, lumTR), min(lumBL, lumBR)));
	float lumMax = max(lumM, max(max(lumTL, lumTR), max(lumBL, lumBR)));
	float lumResult = computeLuminance(result2);
    
    colorBuffer.rgb = lumResult < lumMin ? result1 : result2;
    colorBuffer.a = 1.0;
}