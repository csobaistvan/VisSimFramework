#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/MipMap/common.glsl>

////////////////////////////////////////////////////////////////////////////////
layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Only process valid coordinates
	if (gl_GlobalInvocationID.x >= vMipDimension.x ||
		gl_GlobalInvocationID.y >= vMipDimension.y ||
		gl_GlobalInvocationID.z >= vMipDimension.z) return;

    // Source and target positions
	const ivec3 writePos = ivec3(gl_GlobalInvocationID);
	const ivec3 sourcePos = writePos * 2;

    // Fetch buffer
	vec4 values[8];

	// x -
	FetchTexels(sVoxelMipmapSrc[0], sourcePos, values);
	imageStore(sVoxelMipmapDst[0], writePos, 
	(
		values[0] + values[4] * (1 - values[0].a) + 
		values[1] + values[5] * (1 - values[1].a) +
		values[2] + values[6] * (1 - values[2].a) +
		values[3] + values[7] * (1 - values[3].a)) * 0.25
	);
	// x +
	FetchTexels(sVoxelMipmapSrc[1], sourcePos, values);
    imageStore(sVoxelMipmapDst[1], writePos, 
	(
		values[4] + values[0] * (1 - values[4].a) +
    	values[5] + values[1] * (1 - values[5].a) +
    	values[6] + values[2] * (1 - values[6].a) +
    	values[7] + values[3] * (1 - values[7].a)) * 0.25
    );
	// y -	
	FetchTexels(sVoxelMipmapSrc[2], sourcePos, values);
    imageStore(sVoxelMipmapDst[2], writePos, 
	(
		values[0] + values[2] * (1 - values[0].a) +
    	values[1] + values[3] * (1 - values[1].a) +
    	values[5] + values[7] * (1 - values[5].a) +
    	values[4] + values[6] * (1 - values[4].a)) * 0.25
    );
	// y +
	FetchTexels(sVoxelMipmapSrc[3], sourcePos, values);
    imageStore(sVoxelMipmapDst[3], writePos, 
	(
		values[2] + values[0] * (1 - values[2].a) +
    	values[3] + values[1] * (1 - values[3].a) +
    	values[7] + values[5] * (1 - values[7].a) +
    	values[6] + values[4] * (1 - values[6].a)) * 0.25
    );
	// z -
	FetchTexels(sVoxelMipmapSrc[4], sourcePos, values);
    imageStore(sVoxelMipmapDst[4], writePos, 
	(
		values[0] + values[1] * (1 - values[0].a) +
    	values[2] + values[3] * (1 - values[2].a) +
    	values[4] + values[5] * (1 - values[4].a) +
    	values[6] + values[7] * (1 - values[6].a)) * 0.25
    );
	// z +
	FetchTexels(sVoxelMipmapSrc[5], sourcePos, values);
    imageStore(sVoxelMipmapDst[5], writePos, 
	(
		values[1] + values[0] * (1 - values[1].a) +
    	values[3] + values[2] * (1 - values[3].a) +
    	values[5] + values[4] * (1 - values[5].a) +
    	values[7] + values[6] * (1 - values[7].a)) * 0.25
    );
}