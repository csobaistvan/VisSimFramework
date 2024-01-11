#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

void main()
{	
	// Compute the coordinates of the pixel
	const ivec2 fragmentCoord = ivec2(gl_GlobalInvocationID.xy);

	// Compute the array index for the current fragment's and tile data
	const uint arrayIndex = fragmentArrayIndex(fragmentCoord);
	
	// Extract the fragment color and depth information
	const vec3 fragmentColor = gbufferAlbedo(fragmentCoord);
	const float fragmentDepth = gbufferDepth(fragmentCoord);

	// Construct the fragment entry
	FragmentData fragmentData;
	fragmentData.vColor = fragmentColor;
	fragmentData.vScreenPosition = vec2(fragmentCoord);
	fragmentData.vPsfIndex = sphericalCoordinates(fragmentCoord, fragmentDepth);
	fragmentData.uiBlurRadius = uint(ceil(maxBlurRadii(fragmentData.vPsfIndex)[2]));
	fragmentData.uiFragmentSize = 1;

	// Store the fragment in the fragment buffer
	if (all(lessThan(fragmentCoord.xy, sTiledSplatBlurData.vResolution)))
		sFragmentBuffer[arrayIndex] = packFragmentData(fragmentData);
}