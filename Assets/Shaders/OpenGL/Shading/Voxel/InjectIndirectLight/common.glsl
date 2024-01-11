
////////////////////////////////////////////////////////////////////////////////
// Include the common headers
#include <Shaders/OpenGL/Shading/BRDF/brdf.glsl>
#include <Shaders/OpenGL/Shading/Voxel/sample_grid.glsl>
#include <Shaders/OpenGL/Shading/Voxel/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/sample_grid.glsl>
#include <Shaders/OpenGL/Shading/Voxel/light_trace.glsl>

////////////////////////////////////////////////////////////////////////////////
// Light uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform LightData
{
    TraceDiffuseConeParams sDiffuseTraceParams;
} sLightData;

////////////////////////////////////////////////////////////////////////////////
// Voxel grid images
layout(binding = 0, VOXEL_RADIANCE_TEXTURE_FORMAT) uniform restrict image3D iVoxelRadiance;

////////////////////////////////////////////////////////////////////////////////
// Six voxel faces
const vec3 VOXEL_FACES[6] = 
{
    vec3( -1.0,  0.0,  0.0),
    vec3(  1.0,  0.0,  0.0),
    vec3(  0.0, -1.0,  0.0),
    vec3(  0.0,  1.0,  0.0),
    vec3(  0.0,  0.0, -1.0),
    vec3(  0.0,  0.0,  1.0),
};

////////////////////////////////////////////////////////////////////////////////
vec4 computeVoxelLightDirect(const SurfaceInfo surface, const MaterialInfo material)
{
    return traceDiffuseCone(surface, material, sLightData.sDiffuseTraceParams);
}

////////////////////////////////////////////////////////////////////////////////
vec4 computeVoxelLightNormalWeighted(const SurfaceInfo surface, const MaterialInfo material)
{
    // Weight per axis for the final weighting
    const vec3 weight = surface.normal * surface.normal;

    // Compute the per-face lighting
    SurfaceInfo surfacePerFace = surface;
    vec4 lights[6];
    for (int i = 0; i < 6; ++i)
    {
        surfacePerFace.normal = VOXEL_FACES[i];
        lights[i] = computeVoxelLightDirect(surfacePerFace, material);
    }

    // Compute the final, weighted result
    return normalWeightedSample(weight, lights[0], lights[1], lights[2], lights[3], lights[4], lights[5]);
}

////////////////////////////////////////////////////////////////////////////////
vec4 computeVoxelLight(const SurfaceInfo surface, const MaterialInfo material)
{
    if (sRenderData.uiVoxelShadingMode == VoxelShadingMode_NormalOnly)
        return computeVoxelLightDirect(surface, material);
    if (sRenderData.uiVoxelShadingMode == VoxelShadingMode_NormalWeighted)
        return computeVoxelLightNormalWeighted(surface, material);
    return vec4(0.0);
}

////////////////////////////////////////////////////////////////////////////////
void accumulateLight(const ivec3 gridCoords, const vec4 light)
{
    // Extract the currently stored radiance
    const vec3 prevRadiance = imageLoad(iVoxelRadiance, gridCoords).rgb;

    // Calculate the stored values
    const vec4 outValue = vec4(light.rgb * light.a + prevRadiance, 1.0);

    // write out the results
    imageStore(iVoxelRadiance, gridCoords, outValue);
}