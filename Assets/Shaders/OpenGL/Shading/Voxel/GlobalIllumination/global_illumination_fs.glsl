#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/GlobalIllumination/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// Input attribus
in GeometryData
{
    vec2 vUv;
} g_out;

////////////////////////////////////////////////////////////////////////////////
// Render targets
layout (location = 0) out vec4 colorBuffer;

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Process each sample of the pixel
    vec4 result = vec4(0);
    int numSamples = 0;
    for (int sampleId = 0; sampleId < max(1, sRenderData.iMSAA); ++sampleId)
    {
        // Extract the gbuffer data for the sample
        const SurfaceInfo surface = sampleGbufferSurface(g_out.vUv, gl_Layer, sampleId);
        const MaterialInfo material = sampleGbufferMaterial(g_out.vUv, gl_Layer, sampleId);
    
        // Skip invalid samples
        if (any(isnan(surface.normal)) || any(isinf(surface.normal))) continue;

        // Calculate the final lighting value
        result += calculateIndirectLighting(surface, material);
        ++numSamples;
    }

    // Write out the results
    colorBuffer = result / float(max(1, numSamples));
}