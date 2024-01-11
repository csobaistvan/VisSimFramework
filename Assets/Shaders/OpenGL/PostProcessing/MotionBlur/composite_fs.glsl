#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/MotionBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    //////////////////////////////////////////////////
    //  Constants
    const vec2 pixelSize = 1.0 / textureSize(sNeighborMax, 0);
    const vec2 texelSize = 1.0 / sRenderData.vMaxResolution;
    const float maxPixelSize = max(pixelSize.x, pixelSize.y);
    const float maxTexelSize = max(texelSize.x, texelSize.y);
    const float jitter = pseudoNoise2D(vUv, -1.0, 1.0);
    
    //////////////////////////////////////////////////
    //  NeighborMax data
    const vec2 neighborMax = texture(sNeighborMax, jitterCoords(vUv, jitter, pixelSize)).xy;
    const vec3 neighborMaxData = unpackVelocityData(neighborMax);
    const vec2 vmax = neighborMaxData.xy;
    
    //////////////////////////////////////////////////
    //  Center color
    const vec3 vCenterColor = gbufferAlbedo(vUv, 0, 0);
    
    //////////////////////////////////////////////////
    //  Filter out unaffected pixels
    if (neighborMaxData.z <= 1e-1)
    {
        colorBuffer = vec4(vCenterColor, 1);
        return;
    }
    
    //////////////////////////////////////////////////
    //  Calculate velocities
    const vec3 centerVelocity = unpackVelocityData(gbufferVelocity(vUv, 0, 0));
    const float centerDepth = gbufferDepth(vUv, 0, 0);
    const vec2 wn = vmax / neighborMaxData.z;
    const vec2 vc = centerVelocity.xy;
    const vec2 vn = centerVelocity.xy / centerVelocity.z;
    vec2 wp = vec2(-wn.y, wn.x);
    if (dot(wp, vc) < 0.0) wp = -wp;
    
    const vec2 wc = normalize(mix(wp, vn, (centerVelocity.z - 0.5) / sMotionBlurData.fInterpolationThreshold));
    
    //////////////////////////////////////////////////
    //  Weight center
    float totalWeight = ((sMotionBlurData.iNumTaps + 1) * sMotionBlurData.fCenterWeight) / centerVelocity.z;
    vec3 result = vCenterColor * totalWeight;
    
    //////////////////////////////////////////////////
    //  Sample along velocities
    float odd = 0.0, even = 1.0;
    for (int i = 0; i < sMotionBlurData.iNumTaps; i++)
    {
        const float t = mix(-1.0, 1.0, (i + jitter * sMotionBlurData.fJitterScale + 1) / (sMotionBlurData.iNumTaps + 1));
        
        const vec2 d = vc * odd + vmax * even;
        const vec2 wd = wc * odd + wn * even;
        const float T = t * neighborMaxData.z;
        const vec2 S = saturate(vUv + vec2(t * d * texelSize));
        
        even = odd;
        odd = 1.0 - odd;
        
        const vec3 currentVelocity = unpackVelocityData(gbufferVelocity(S, 0, 0));
        const float currentDepth = gbufferDepth(S, 0, 0);
        const vec3 colorSample = gbufferAlbedo(S, 0, 0);
        
        const float f = zCompare(centerDepth, currentDepth);
        const float b = zCompare(currentDepth, centerDepth);
        
        const float wA = max(0.0, dot(wc, wd));
        const float wB = max(0.0, dot(currentVelocity.xy / currentVelocity.z, wd));
        
        const float weight = 
            f * cone(T, 1.0 / currentVelocity.z) * wB +
            b * cone(T, 1.0 / centerVelocity.z) * wA + 
            cylinder(T, min(centerVelocity.z, currentVelocity.z)) * max(wA, wB) * 2.0;
        
        totalWeight += weight;
        result += colorSample * weight;
    }

    //////////////////////////////////////////////////
    //  Apply weight
    colorBuffer = vec4(result / totalWeight, 1.0);
}