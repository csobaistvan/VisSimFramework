#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/ShadowMap/common.glsl>

// Input attribus
in VertexData
{
    vec2 vUv;
} fs_in;

// Render targets
layout (location = 0) out vec4 shadowBuffer;

void main()
{
    // Evaluate the material fn. 
    MaterialInfo materialInfo = evaluateMaterialFn(fs_in.vUv);

    // Discard transparent pixels
    if (materialInfo.opacity < 0.02)
        discard;

    // Compute the first and second moments
    const float depth = extractShadowMapDepth(gl_FragCoord);
    
    // Basic algorithm: depth only
    if (uiShadowMapAlgorithm == ShadowMapAlgorithm_Basic)
    {
        shadowBuffer.x = depth;
    }

    // VSM: first and second moments
    else if (uiShadowMapAlgorithm == ShadowMapAlgorithm_Variance)
    {
        shadowBuffer.xy = calculateMoments2(depth, dFdx(depth), dFdy(depth));
    }

    // ESM: linear depth
    else if (uiShadowMapAlgorithm == ShadowMapAlgorithm_Exponential)
    {
        shadowBuffer.x = depth;
    }

    // EVSM: first and second moments, positive and negative version
    else if (uiShadowMapAlgorithm == ShadowMapAlgorithm_ExponentialVariance)
    {
        const vec2 exponentialConstants = clampEsmExponent(vExponentialConstants, uiShadowMapPrecision);
        const float pos = exp(exponentialConstants.x * depth);
        const float neg = -exp(-exponentialConstants.y * depth);
        shadowBuffer.xy = calculateMoments2(pos, dFdx(pos), dFdy(pos));
        shadowBuffer.zw = calculateMoments2(neg, dFdx(neg), dFdy(neg));
    }

    // MSM: first 4 moments
    else if (uiShadowMapAlgorithm == ShadowMapAlgorithm_Moments)
    {
        shadowBuffer = calculateMoments4(depth, dFdx(depth), dFdy(depth));
        //shadowBuffer = calculateOptimizedMoments4(depth, dFdx(depth), dFdy(depth));
    }
}
