

////////////////////////////////////////////////////////////////////////////////
// Input and output textures
layout(binding = TEXTURE_POST_PROCESS_1) uniform sampler3D sVoxelMipmapSrc[6];
layout(binding = 0, VOXEL_RADIANCE_TEXTURE_FORMAT) uniform writeonly image3D sVoxelMipmapDst[6];

////////////////////////////////////////////////////////////////////////////////
// Mipmap parameters
layout (location = 0) uniform vec3 vMipDimension;
layout (location = 1) uniform uint uiMipLevel;

////////////////////////////////////////////////////////////////////////////////
const ivec3 sampleOffsets[] = ivec3[8]
(
	ivec3(1, 1, 1),
	ivec3(1, 1, 0),
	ivec3(1, 0, 1),
	ivec3(1, 0, 0),
	ivec3(0, 1, 1),
	ivec3(0, 1, 0),
	ivec3(0, 0, 1),
	ivec3(0, 0, 0)
);

////////////////////////////////////////////////////////////////////////////////
void FetchTexels(sampler3D voxelGrid, const ivec3 position, inout vec4 val[8]) 
{
	for (int i = 0; i < 8; i++)
		val[i] = texelFetch(voxelGrid, position + sampleOffsets[i], int(uiMipLevel));
}