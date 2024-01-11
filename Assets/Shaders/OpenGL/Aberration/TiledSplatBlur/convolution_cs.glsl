#version 440

#extension GL_ARB_shader_group_vote: enable

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = CONVOLUTION_GROUP_SIZE, local_size_y = CONVOLUTION_GROUP_SIZE, local_size_z = 1) in;

// Whether we want to use unpacked shared data or not
#if CONVOLUTION_GROUP_SIZE >= 32
	#define USE_PACKED_SHARED_DATA 1
	#define SHARED_DATA_TYPE FragmentDataPacked
#else
	#define USE_PACKED_SHARED_DATA 0
	#define SHARED_DATA_TYPE FragmentData
#endif

// Size of shared fragment arrays
#define SHARED_ARRAY_SIZE CONVOLUTION_GROUP_SIZE * CONVOLUTION_GROUP_SIZE

// Shared array for holding the per-tile fragments
shared SHARED_DATA_TYPE sTileFragments[SHARED_ARRAY_SIZE];

// Number of total fragments in the tile
uint uiNumFragments = 0;

// Result of the convolution
vec3 vResult = vec3(0.0);

// Total weight during convolution
vec3 vTotalWeight = vec3(0.0);

// Number of samples contributing to the result
int iNumSamples = 0;

// Various necessary coordinates and indices
#define DEFINE_COORDINATES_INDICES() \
	const ivec2 fragmentCoord = ivec2(gl_GlobalInvocationID.xy); \
	const ivec2 tileId = ivec2(gl_GlobalInvocationID.xy / TILE_SIZE); \
	const ivec2 threadId = ivec2(gl_LocalInvocationID.xy); \
    const uint threadIndex = gl_LocalInvocationID.y * CONVOLUTION_GROUP_SIZE + gl_LocalInvocationID.x; \
    const uint arrayIndex = tileArrayIndex(tileId.xy); \
    const uint countArrayIndex = tileCountArrayIndex(tileId.xy); \
	const uint numTileFragments = CONVOLUTION_GROUP_SIZE * CONVOLUTION_GROUP_SIZE; \
	const uint groupSize = CONVOLUTION_GROUP_SIZE * CONVOLUTION_GROUP_SIZE; \
	const uint batchSize = SHARED_ARRAY_SIZE

// Unpacks the fragments in a given batch
uint unpackBatchFragments(const int batchId)
{
	// Define the necessary coordinates
	DEFINE_COORDINATES_INDICES();

	// Batch start index
	const uint batchStartIndex = batchId * batchSize;

	// Number of fragments to fetch by this thread
	const uint numFragmentsLeft = min(uiNumFragments - batchStartIndex, batchSize);
	
	// Index into the sort index buffer
	const uint sortElementIndex = arrayIndex + batchStartIndex + threadIndex;

	// Index into the fragment buffer
	const uint fragmentIndex = sTileSortBuffer[sortElementIndex].uiIndex;

	// Extract the unpacked fragment data
	#if USE_PACKED_SHARED_DATA
	sTileFragments[threadIndex] = sFragmentBuffer[fragmentIndex];
	#else
	sTileFragments[threadIndex] = unpackFragmentData(sFragmentBuffer[fragmentIndex]);
	#endif

	// Return the number of extracted fragments
	return numFragmentsLeft;
}

#if DEBUG_OUTPUT == 1

// Returns true if the current fragment corresponds to the center fragment
bool isCenterFragmentData(const vec2 fragmentCoord, const FragmentData fragmentData)
{
	return distance(fragmentData.vScreenPosition, vec2(fragmentCoord)) <= calcFragmentSizeOffset(fragmentData.uiFragmentSize) + 0.01;
}

// Write out debug info using the current fragment
void debugVisualization(const FragmentData fragmentData)
{
	// Define the necessary coordinates
	DEFINE_COORDINATES_INDICES();

	// Compute the blur radii for the 3 channels
	vec3 blurRadius = vec3(0.0);
	for (uint i = 0; i < sTiledSplatBlurData.uiRenderChannels; ++i)
	{
		const vec3 blurRadii = blurRadii(fragmentData.vPsfIndex, i);
		blurRadius[i] = blurRadii[2];
	}
	for (uint i = sTiledSplatBlurData.uiRenderChannels; i < 3; ++i)
	{
		blurRadius[i] = blurRadius[sTiledSplatBlurData.uiRenderChannels - 1];
	}

	// Visualize the merged color
	if (sTiledSplatBlurData.uiOutputMode == OutputMode_MergedColor)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			vResult = vec3(fragmentData.vColor);
			vTotalWeight = vec3(1.0);
		}
	}

	// Visualize the merged depth
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_MergedDepth)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			vResult = vec3(linearCamSpaceDepth(fragmentData.vPsfIndex.z));
			vTotalWeight = vec3(1.0);
		}
	}

	// Visualize alpha
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_Alpha)
	{
		vResult = vec3(vTotalWeight);
		vTotalWeight = vec3(1.0);
	}

	// Visualize the tile size
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_TileBufferSize)
	{
		float ratio = uiNumFragments / float(sTiledSplatBlurData.uiTileBufferTotalSubentries - 1);
		vResult = ratio > 1.0 ? vec3(1, 0, 0) : vec3(ratio);
		vTotalWeight = vec3(1.0);
	}
	
	// Visualize the fragment size
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_FragmentSize)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			const float fragmentSize = fragmentData.uiFragmentSize;
			vResult = vec3(fragmentSize / float(sTiledSplatBlurData.uiMergedFragmentSize));
			vTotalWeight = vec3(1.0);
		}
	}

	// Visualize the PSF id of the center fragment
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_PsfId)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			const vec3 psfIndex = vec3
			(
				floor(horizontalAngleIndex(fragmentData.vPsfIndex.x)),
				floor(verticalAngleIndex(fragmentData.vPsfIndex.y)),
				floor(objectDistanceIndex(fragmentData.vPsfIndex.z))
			);
			const vec3 psfIndexNorm = vec3
			(
				saturate(psfIndex.x / float(sTiledSplatBlurData.uiNumHorizontalAngles - 1)),
				saturate(psfIndex.y / float(sTiledSplatBlurData.uiNumVerticalAngles - 1)),
				saturate(psfIndex.z / float(sTiledSplatBlurData.uiNumObjectDistances - 1))
			);

			vResult = psfIndexNorm;
			vTotalWeight = vec3(1.0);
		}
	}

	// Visualize the PSF id of the center fragment
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_LerpFactor)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			const vec3 lerpFactor = vec3
			(
				fract(horizontalAngleIndex(fragmentData.vPsfIndex.x)),
				fract(verticalAngleIndex(fragmentData.vPsfIndex.y)),
				fract(objectDistanceIndex(fragmentData.vPsfIndex.z))
			);
			vResult = lerpFactor;
			vTotalWeight = vec3(1.0);
		}
	}

	// Visualize the blur radius of the center fragment
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_BlurRadiusCont || 
		sTiledSplatBlurData.uiOutputMode == OutputMode_BlurRadius ||
		sTiledSplatBlurData.uiOutputMode == OutputMode_BlurRadiusFract)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			if (sTiledSplatBlurData.uiOutputMode == OutputMode_BlurRadiusCont)
			{
				vResult = blurRadius / sTiledSplatBlurData.uiMaxBlurRadiusGlobal;
				vTotalWeight = vec3(1.0);
			}
			else if (sTiledSplatBlurData.uiOutputMode == OutputMode_BlurRadius)
			{
				vResult = floor(blurRadius) / sTiledSplatBlurData.uiMaxBlurRadiusGlobal;
				vTotalWeight = vec3(1.0);
			}
			else if (sTiledSplatBlurData.uiOutputMode == OutputMode_BlurRadiusFract)
			{
				vResult = fract(blurRadius);
				vTotalWeight = vec3(1.0);
			}
		}
	}

	// Visualize the object depth borders
	if (sTiledSplatBlurData.uiOverlayMode == OverlayMode_PsfBorder)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			const vec3 psfIndex = vec3
			(
				floor(horizontalAngleIndex(fragmentData.vPsfIndex.x)),
				floor(verticalAngleIndex(fragmentData.vPsfIndex.y)),
				floor(objectDistanceIndex(fragmentData.vPsfIndex.z))
			);
			if (fract(psfIndex[0]) < 0.08) { vResult.x = 1.0; vTotalWeight.x = 1.0; }
			if (fract(psfIndex[1]) < 0.08) { vResult.y = 1.0; vTotalWeight.y = 1.0; }
			if (fract(psfIndex[2]) < 0.08) { vResult.z = 1.0; vTotalWeight.z = 1.0; }
		}
	}

	// Visualize the blur radius borders
	else if (sTiledSplatBlurData.uiOverlayMode == OverlayMode_BlurRadiusBorder)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			for (int i = 0; i < 3; ++i)
			{
				if (fract(blurRadius[i]) < 0.08)
				{
					vResult[i] = 1.0;
					vTotalWeight[i] = 1.0;
				}
			}
		}
	}

	// Visualize the tile borders as an overlay
	else if (sTiledSplatBlurData.uiOverlayMode == OverlayMode_TileBorder)
	{
		if
		(
			(gl_GlobalInvocationID.x % TILE_SIZE) == 0 || 
			(gl_GlobalInvocationID.y % TILE_SIZE) == 0 ||
			(gl_GlobalInvocationID.x % TILE_SIZE) == TILE_SIZE - 1 ||
			(gl_GlobalInvocationID.y % TILE_SIZE) == TILE_SIZE - 1
		)
		{
			vResult = vec3(1.0, 0.0, 0.0);
			vTotalWeight = vec3(1.0);
		}
	}

	// Visualize the object depth increments as an overlay
	else if (sTiledSplatBlurData.uiOverlayMode == OverlayMode_ObjectDepthBorder)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			const float depthFract = fract(fragmentData.vPsfIndex.z);
			const float smallDepthFract = fract(fragmentData.vPsfIndex.z * 10.0);
			if (depthFract < 0.02 || depthFract > 0.998)
			{
				vResult = vec3(1.0, 0.0, 0.0);
				vTotalWeight = vec3(1.0);
			}
			else if (fragmentData.vPsfIndex.z < 1.0 && (smallDepthFract < 0.02 || smallDepthFract > 0.998))
			{
				vResult = vec3(0.0, 1.0, 0.0);
				vTotalWeight = vec3(1.0);
			}
		}
	}

	// Visualize the blur radius of the center fragment
	else if (sTiledSplatBlurData.uiOutputMode == OutputMode_IncidentAngle)
	{
		if (isCenterFragmentData(fragmentCoord, fragmentData))
		{
			vResult.xy = abs(fragmentData.vPsfIndex.xy) / vec2(sTiledSplatBlurData.fHorizontalAnglesMax, sTiledSplatBlurData.fVerticalAnglesMax);
			vResult.z = 0.0;
			vTotalWeight = vec3(1.0);
		}
	}
}

#endif

// Perform the convolution process with the parameter fragment
void convolveFragment(const FragmentData fragmentData)
{
	// Define the necessary coordinates
	DEFINE_COORDINATES_INDICES();
	
	// Sample the psf
	const vec3 weight = samplePsf(vec2(fragmentCoord), fragmentData);

	// Increment the number of samples
	++iNumSamples;

	// Front-to-back blending
	if (sTiledSplatBlurData.uiAccumulationMethod == AccumulationMethod_FrontToBack)
	{
		vResult += (1.0 - vTotalWeight) * weight * fragmentData.vColor;
		vTotalWeight = weight + (1.0 - weight) * vTotalWeight;
	}

	// Back-to-front blending
	else if (sTiledSplatBlurData.uiAccumulationMethod == AccumulationMethod_BackToFront)
	{
		vResult = weight * fragmentData.vColor + (1.0 - weight) * vResult;
		vTotalWeight = weight + (1.0 - weight) * vTotalWeight;
	}

	// Regular summation
	else if (sTiledSplatBlurData.uiAccumulationMethod == AccumulationMethod_Sum)
	{
		vResult += weight * fragmentData.vColor;
		vTotalWeight += weight;
	}
}

// Perform the convolution with the current batch
void convolve(const uint numBatchFragments)
{
	// Define the necessary coordinates
	DEFINE_COORDINATES_INDICES();

	// Process each fragment in the tile array
	for (int elementIndex = 0; elementIndex < numBatchFragments; ++elementIndex)
	{
		// Extract the fragment's data
		#if USE_PACKED_SHARED_DATA
		const FragmentData fragmentData = unpackFragmentData(sTileFragments[elementIndex]);
		#else
		const FragmentData fragmentData = sTileFragments[elementIndex];
		#endif

		// Accumulate, if needed
		#if DEBUG_OUTPUT == 1
		if (sTiledSplatBlurData.uiOutputMode == OutputMode_Convolution || 
			sTiledSplatBlurData.uiOutputMode == OutputMode_Alpha)
		#endif
			convolveFragment(fragmentData);

		#if DEBUG_OUTPUT == 1
		debugVisualization(fragmentData);
		#endif
	}
}

void main()
{
	// Define the necessary coordinates
	DEFINE_COORDINATES_INDICES();

	// Extract the number of fragments to process
	uiNumFragments = sTileParametersBuffer[countArrayIndex].uiNumFragmentsTotal;

	// Number of batches to make
	const uint numBatches = ROUNDED_DIV(uiNumFragments, batchSize);

	// Init the PSF data
	initPsfData();

	// Fetch the current fragments
	for (int batchId = 0; batchId < numBatches; ++batchId)
	{
		// Unpack the current batch to groupshared memory
		const uint numBatchFragments = unpackBatchFragments(batchId);
		memoryBarrierShared();
		barrier();
		
		// Perform convolution using the local data
		convolve(numBatchFragments);
		memoryBarrierShared();
		barrier();
	}

	// Ignore non-existent pixels
	if (any(greaterThanEqual(vec2(fragmentCoord), sTiledSplatBlurData.vResolution)))
		return;

	// Normalize back the result
	if (sTiledSplatBlurData.bNormalizeResult == 1)
		vResult /= vTotalWeight;

	// Write out the convolution result
	imageStore(sResult, ivec2(fragmentCoord), vec4(vResult, 1.0));
}