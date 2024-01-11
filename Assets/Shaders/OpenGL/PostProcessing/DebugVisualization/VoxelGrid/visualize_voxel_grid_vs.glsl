#version 440

#extension GL_ARB_shader_image_load_store : require

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/DebugVisualization/VoxelGrid/common.glsl>

out vec4 albedo;

void main()
{
    // Cube indices
	const vec3 position = vec3
	(
		gl_VertexID % sRenderData.uiNumVoxels,
		(gl_VertexID / sRenderData.uiNumVoxels) % sRenderData.uiNumVoxels,
		gl_VertexID / (sRenderData.uiNumVoxels * sRenderData.uiNumVoxels)
	);

    // Grid coordinates
	const int textureDimensions = 1 << sDebugVisualizationData.iLodLevel;
	const ivec3 gridTexCoords = ivec3(position / textureDimensions);

	// Color component
	if (sDebugVisualizationData.iComponent == BufferComponent_Albedo)
	{
		albedo = texelFetch(sVoxelAlbedo, gridTexCoords, sDebugVisualizationData.iLodLevel);
	}

	// Normal component
	else if (sDebugVisualizationData.iComponent == BufferComponent_Normal)
	{
		albedo = texelFetch(sVoxelNormal, gridTexCoords, sDebugVisualizationData.iLodLevel);
		albedo.rgb = visualizeNormal(normalize(albedo.rgb * 2.0 - 1.0));
	}
	
	// Metallic component
	else if (sDebugVisualizationData.iComponent == BufferComponent_Metallic)
	{
		albedo = texelFetch(sVoxelSpecular, gridTexCoords, sDebugVisualizationData.iLodLevel);
		albedo.rgb = vec3(albedo.r);
	}
	
	// Roughness component
	else if (sDebugVisualizationData.iComponent == BufferComponent_Roughness)
	{
		albedo = texelFetch(sVoxelSpecular, gridTexCoords, sDebugVisualizationData.iLodLevel);
		albedo.rgb = vec3(albedo.g);
	}
	
	// Specular component
	else if (sDebugVisualizationData.iComponent == BufferComponent_Specular)
	{
		albedo = texelFetch(sVoxelSpecular, gridTexCoords, sDebugVisualizationData.iLodLevel);
		albedo.rgb = vec3(albedo.b);
	}
	
	// Radiance component
	else if (sDebugVisualizationData.iComponent == BufferComponent_Radiance)
	{
		if (sDebugVisualizationData.iVoxelFace == VoxelFace_Isotropic)
		{
			albedo = texelFetch(sVoxelRadiance, gridTexCoords, sDebugVisualizationData.iLodLevel);
		}
		else
		{
			const int faceId = sDebugVisualizationData.iVoxelFace - VoxelFace_AnisotropicXNeg;
			albedo = texelFetch(sVoxelRadianceAnisotropic[faceId], gridTexCoords, max(0, sDebugVisualizationData.iLodLevel - 1));
		}
	}

	gl_Position = vec4(position, 1.0);
}