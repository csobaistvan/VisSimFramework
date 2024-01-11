#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Shading/Deferred/ComputeLight/common.glsl>

// Input attribus
in GeometryData
{
    vec2 vUv;
} g_out;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    // Process each sample of the pixel
    vec3 result = vec3(0);
    for (int sampleId = 0; sampleId < max(1, sRenderData.iMSAA); ++sampleId)
    {
        // Extract the gbuffer data for the sample
        const SurfaceInfo surface = sampleGbufferSurface(g_out.vUv, gl_Layer, sampleId);
        const MaterialInfo material = sampleGbufferMaterial(g_out.vUv, gl_Layer, sampleId);
    
        // Calculate the final lighting value
        result += computeLight(surface, material).rgb;
    }

    // Write out the results
    colorBuffer = vec4(result / float(max(1, sRenderData.iMSAA)), 0.0);
}