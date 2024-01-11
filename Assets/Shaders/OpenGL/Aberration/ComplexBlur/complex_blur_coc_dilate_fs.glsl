#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/ComplexBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Uniforms
layout (location = 0) uniform uint uiPassId;
layout (location = 1) uniform uint uiDilatedTileSizePrev;
layout (location = 2) uniform uint uiDilatedTileSizeCurr;
layout (location = 3) uniform vec2 vDilatedResolutionPrev;
layout (location = 4) uniform vec2 vDilatedResolutionCurr;

// Render targets
layout (location = 0) out vec4 colorBuffer;

ivec2 calcSampleCoords(const int x, const int y, const vec2 resolution)
{
    return clamp(ivec2(gl_FragCoord.xy) * ivec2(sComplexBlurData.uiDilationSearchRadius) + ivec2(x, y), ivec2(0), ivec2(resolution - 1));
}

// Samples the scene radius buffer
vec2 sampleRadiusScene(const int x, const int y)
{
    const ivec2 sampleCoords = calcSampleCoords(x, y, sRenderData.vResolution);
    return vec2(texelFetch(sDownscaledBuffer, sampleCoords, 0).a);
}

// Samples the dilated minmax radius buffer
vec2 sampleRadiusDilated(const int x, const int y)
{
    const ivec2 sampleCoords = calcSampleCoords(x, y, vDilatedResolutionPrev);
    return vec2(texelFetch(sDilatedBuffer, sampleCoords, 0).xy);
}

// Samples the previous min-max blur radius buffer
vec2 samplePrevMinMaxRadius(const int x, const int y)
{
    if (uiPassId == 0) // First pass: sample the scene texture
        return sampleRadiusScene(x, y);
    else // Subsequent passes: sample the dilated texture
        return sampleRadiusDilated(x, y);
}

void main()
{    
    vec2 minmax = vec2(1000, -1000);
    for (int y = -int(sComplexBlurData.uiDilationSearchRadius); y <= int(sComplexBlurData.uiDilationSearchRadius); y++)
    for (int x = -int(sComplexBlurData.uiDilationSearchRadius); x <= int(sComplexBlurData.uiDilationSearchRadius); x++)
    {
        const vec2 prevMinMax = samplePrevMinMaxRadius(x, y);
        minmax.x = min(minmax.x, prevMinMax.x);
        minmax.y = max(minmax.y, prevMinMax.y);
    }
    colorBuffer.xy = minmax;
}