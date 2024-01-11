#version 440

#extension GL_ARB_shader_image_load_store : require

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

////////////////////////////////////////////////////////////////////////////////
// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/InjectDirectLight/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// Entry point
void main()
{
    // Only process valid coordinates
	if (gl_GlobalInvocationID.x >= sRenderData.uiNumVoxels ||
		gl_GlobalInvocationID.y >= sRenderData.uiNumVoxels ||
		gl_GlobalInvocationID.z >= sRenderData.uiNumVoxels) return;

    // Voxel grid location
	const ivec3 gridCoords = ivec3(gl_GlobalInvocationID);

    // Ignore invisible voxels
    if (isVoxelEmpty(gridCoords)) { return; }
    
    // Extract the voxel Gbuffer data for the surface
    SurfaceInfo surface = sampleVoxelSurface(gridCoords);
    MaterialInfo material = sampleVoxelMaterial(gridCoords);

    // Compute lighting and accumulate
    accumulateLight(gridCoords, computeVoxelLight(surface, material));
}