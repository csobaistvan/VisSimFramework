#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>

////////////////////////////////////////////////////////////////////////////////
// Extensions
////////////////////////////////////////////////////////////////////////////////

#extension GL_ARB_shader_ballot : require
#extension GL_ARB_shader_group_vote : require
#extension GL_NV_gpu_shader5 : require
#extension GL_NV_shader_atomic_float : require

////////////////////////////////////////////////////////////////////////////////
// Uniform & data buffers
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform TiledSplatBlurData
{
	// Common, pre-computed values
	uvec2 vNumTiles;
	ivec2 vResolution;
	ivec2 vPaddedResolution;
	vec2 vCameraFov;
	uint uiFragmentBufferSubentries;
	uint uiTileBufferCenterSubentries;
	uint uiTileBufferTotalSubentries;
	uint uiNumSortIterations;

	// Various algorithm types
	uint uiPsfAxisMethod;
	uint uiPsfTextureFormat;
	uint uiPsfTextureDepthLayout;
	uint uiPsfTextureAngleLayout;
	uint uiWeightScaleMethod;
	uint uiWeightRescaleMethod;
	uint uiOutputMode;
	uint uiOverlayMode;
	uint uiAccumulationMethod;
	
	// PSF texture settings
	vec4 vPsfLayersS;
	vec4 vPsfLayersP;

	// Merge properties
	uint uiNumMergeSteps;
	uint uiMergedFragmentSize;

	// Sort properties
	float fSortDepthOffset;
	float fSortDepthScale;

	// Convolution settings
	uint uiRenderChannels;
	uint uiRenderLayers;
	float fDepthOffset;
	float fAlphaThreshold;
	float bNormalizeResult;

	// PSF properties
	uint uiMinBlurRadiusCurrent;
	uint uiMaxBlurRadiusCurrent;
	uint uiMinBlurRadiusGlobal;
	uint uiMaxBlurRadiusGlobal;
	uint uiNumObjectDistances;
	uint uiNumHorizontalAngles;
	uint uiNumVerticalAngles;
	uint uiNumChannels;
	uint uiNumApertures;
	uint uiNumFocusDistances;
	float fObjectDistancesMin;
	float fObjectDistancesMax;
	float fObjectDistancesStep;
	float fAperturesMin;
	float fAperturesMax;
	float fAperturesStep;
	float fFocusDistancesMin;
	float fFocusDistancesMax;
	float fFocusDistancesStep;
	float fHorizontalAnglesMin;
	float fHorizontalAnglesMax;
	float fHorizontalAnglesStep;
	float fVerticalAnglesMin;
	float fVerticalAnglesMax;
	float fVerticalAnglesStep;
} sTiledSplatBlurData;

////////////////////////////////////////////////////////////////////////////////
//  Output image buffer
layout (binding = 0, rgba16f) uniform restrict writeonly image2D sResult;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Data corresponding to a single entry in the per-fragment buffer
struct FragmentData
{
	vec3 vColor;
	vec2 vScreenPosition;
	vec3 vPsfIndex;
	uint uiFragmentSize;
	uint uiBlurRadius;
};

// Packed version of FragmentData
struct FragmentDataPacked
{
	uvec4 data;
};

// Per-fragment data buffer
TYPED_ARRAY_BUFFER(std430, , UNIFORM_BUFFER_GENERIC_3, sFragmentBuffer_, FragmentDataPacked);
#define sFragmentBuffer sFragmentBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Tile parameter buffer
////////////////////////////////////////////////////////////////////////////////

// Per-tile parameters buffer
STRUCT_ARRAY_BUFFER(std430, , UNIFORM_BUFFER_GENERIC_4, sTileParametersBuffer_, 
	uint uiNumFragmentsTile; uint uiNumFragmentsTotal; uint _1; uint _2);
#define sTileParametersBuffer sTileParametersBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Per-tile dispatch data
////////////////////////////////////////////////////////////////////////////////

// Per-tile dispatch command buffer. Contains the number of groups along the three axes,
// as well as the number of fragments in the corresponding tile array.
STRUCT_ARRAY_BUFFER(std430, , UNIFORM_BUFFER_GENERIC_5, sTileDispatchBuffer_, uvec3 vNumGroups; uint uiIndex);
#define sTileDispatchBuffer sTileDispatchBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Structure representing a splat index.
struct SplatIndex
{
	uint uiTileId;
	uint uiFragmentIndex;
	float fFragmentDepth;
	float fBlurRadius;
	vec2 vScreenPosition;
};

// Per-tile splat index buffer
TYPED_ARRAY_BUFFER(std430, , UNIFORM_BUFFER_GENERIC_6, sTileSplatBuffer_, SplatIndex);
#define sTileSplatBuffer sTileSplatBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Structure representing a sort index.
struct SortIndex
{
	uint uiIndex;
	float fDepth;
};

// Per-tile sort index buffer
TYPED_ARRAY_BUFFER(std430, , UNIFORM_BUFFER_GENERIC_7, sTileSortBuffer_, SortIndex);
#define sTileSortBuffer sTileSortBuffer_.sData

// Include the PSF header
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/PSF/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// Fragment data packing & unpacking functions
////////////////////////////////////////////////////////////////////////////////

// Determines whether we should convert the PSF index to the tile representation or not
bool shouldConvertPsfIndex()
{
#if DEBUG_OUTPUT == 1
	return sTiledSplatBlurData.uiOutputMode == OutputMode_Convolution;
#else
	return true;
#endif
}

// Converts the fragment data to the tile-buffer representation
FragmentData convertToTileFragmentData(const FragmentData fragmentData)
{
	FragmentData result = fragmentData;
	if (shouldConvertPsfIndex()) result.vPsfIndex = sphericalCoordsToPsfIndex(result.vPsfIndex);
	return result;
}

// How many bits to use to store the fragment size and no. fragments
// - 12 bits: screen pos X (11 [max width of 2047] + 1 for precision)
// - 12 bits: screen pos Y (11 [max res of 2047] + 1 for precision)
// - 1 bits: fragment size (offset by 1, yielding [1, 2])
// - 7 bits: blur radius (yielding [0, 127])
const uvec4 _LAST_ENTRY_BITS = uvec4(12, 12, 1, 7);

// Unlocks the color from the packed fragment data
vec3 unpackColor(const FragmentDataPacked data)
{
	return vec3(unpackHalf2x16(data.data[0]), unpackHalf2x16(data.data[1]).x);
}

// Unpacks the PSF index from the packed fragment data
vec3 unpackPsfIndex(const FragmentDataPacked data)
{
	return vec3(unpackHalf2x16(data.data[2]), unpackHalf2x16(data.data[1]).y);
}

// Unpacks the screen-space position from the packed fragment data
vec2 unpackScreenPosition(const FragmentDataPacked data)
{
	return vec2(unpackUint4xXYZW_v1(data.data[3], _LAST_ENTRY_BITS), unpackUint4xXYZW_v2(data.data[3], _LAST_ENTRY_BITS)) * 0.5;
}

// Unpacks the fragment size from the packed fragment data
uint unpackFragmentSize(const FragmentDataPacked data)
{
	return unpackUint4xXYZW_v3(data.data[3], _LAST_ENTRY_BITS) + 1;
}

// Unpacks the blur size from the packed fragment data
uint unpackBlurRadius(const FragmentDataPacked data)
{
	return unpackUint4xXYZW_v4(data.data[3], _LAST_ENTRY_BITS);
}

// Packs the data corresponding to a single fragment data
FragmentDataPacked packFragmentData(const FragmentData data)
{
	FragmentDataPacked result;
	result.data[0] = packHalf2x16(data.vColor.xy);
	result.data[1] = packHalf2x16(vec2(data.vColor.z, data.vPsfIndex.z));
	result.data[2] = packHalf2x16(data.vPsfIndex.xy);
	result.data[3] = packUint4xXYZW(uvec4(data.vScreenPosition * 2.0, data.uiFragmentSize - 1, data.uiBlurRadius), _LAST_ENTRY_BITS);
	return result;
}

// Unpacks a packed fragment data entry
FragmentData unpackFragmentData(const FragmentDataPacked data)
{
	FragmentData result;
	result.vColor = unpackColor(data);
	result.uiFragmentSize = unpackFragmentSize(data);
	result.uiBlurRadius = unpackBlurRadius(data);
	result.vScreenPosition = unpackScreenPosition(data);
	result.vPsfIndex = unpackPsfIndex(data);
	return result;
}

////////////////////////////////////////////////////////////////////////////////
// Resolution and tile-related functions
////////////////////////////////////////////////////////////////////////////////

// Computes the original fragment coordinates for the parameter merged fragment coordinates
ivec2 mergedFragmentCoords(const ivec2 fragmentId)
{
	return fragmentId * int(sTiledSplatBlurData.uiMergedFragmentSize);
}

// Computes the original fragment coordinates for the parameter tile and local coordinates
ivec2 mergedFragmentCoords(const ivec2 tileId, const ivec2 threadId)
{
	return mergedFragmentCoords(tileId * MERGED_TILE_SIZE + threadId);
}

// Array index for the merged per-fragment buffer
uint fragmentArrayIndex(ivec2 fragmentCoord)
{
	#if MERGE_STEPS == 0
		return fragmentCoord.y * int(sTiledSplatBlurData.vResolution.x) + fragmentCoord.x;
	#elif MERGE_STEPS == 1
		const uvec2 blockCoords = uvec2(fragmentCoord / 2);
		const uvec2 innerCoords = uvec2(fragmentCoord - blockCoords * 2);
		const int blockStride = ROUNDED_DIV(int(sTiledSplatBlurData.vResolution.x), 2);
		const uint block = (blockCoords.y * blockStride + blockCoords.x) * 4;
		const uint inner = innerCoords.y * 2 + innerCoords.x;
		return block + inner;
	#endif

	/*
	// Start the accumulation process from the final merged size and go downwards from there
	int blockSize = int(sTiledSplatBlurData.uiMergedFragmentSize);
	int outerStride = blockSize * blockSize;
	int innerStride = ROUNDED_DIV(int(sTiledSplatBlurData.vResolution.x), blockSize);

	// Go through each different block and accumulate the local coordinates
	uint result = 0;
	while (blockSize > 0)
	{
		// Computes the fragment's local coordinates w.r.t. the current block size
		const ivec2 blockCoord = fragmentCoord / blockSize;
		fragmentCoord -= (blockCoord * blockSize);

		// Apply striding and accumulate the index
		result += uint(blockCoord.y * innerStride + blockCoord.x) * outerStride;

		// Jump to the next block size and update the strides
		blockSize /= 2;
		innerStride = 2;
		outerStride = blockSize * blockSize;
	}
	return result;
	*/
}

// Array index for the per-tile buffer
uint tileArrayIndex(ivec2 tileId)
{
	return uint(tileId.y * sTiledSplatBlurData.vNumTiles.x + tileId.x) * sTiledSplatBlurData.uiTileBufferTotalSubentries;
}

// Array index for the per-tile counter buffer
uint tileCountArrayIndex(ivec2 tileId)
{
	return uint(tileId.y * sTiledSplatBlurData.vNumTiles.x + tileId.x);
}

// Tests whether a specified tile id is valid.
bool isValidTileId(ivec2 tileId)
{
	return all(greaterThanEqual(tileId, ivec2(0))) && all(lessThan(tileId, ivec2(sTiledSplatBlurData.vNumTiles)));
}

// Calculates the number of fragments from the input fragment size
uint calcNumFragments(const uint fragmentSize)
{
	return (sTiledSplatBlurData.uiMergedFragmentSize * sTiledSplatBlurData.uiMergedFragmentSize) / (fragmentSize * fragmentSize);
}

// Offset for merged fragments
float calcFragmentSizeOffset(const uint fragmentSize)
{
	// sqrt(2) * (size-1)/2
	return 1.4142135623730950488016887242097 * (float(fragmentSize - 1) * 0.5);
}

////////////////////////////////////////////////////////////////////////////////
// Incident angle functions
////////////////////////////////////////////////////////////////////////////////

// Converts the input incident angles to the appropriate representation for rendering
vec2 convertIncidentAngles(const vec2 anglesRad)
{
	return clamp
	(
		degrees(anglesRad),
		vec2(sTiledSplatBlurData.fHorizontalAnglesMin, sTiledSplatBlurData.fVerticalAnglesMin),
		vec2(sTiledSplatBlurData.fHorizontalAnglesMax, sTiledSplatBlurData.fVerticalAnglesMax)
	);
}

// Converts the input camera-space depth to the appropriate representation for rendering
float convertDepth(const float depth)
{
	return max(depth + sTiledSplatBlurData.fDepthOffset, 1e-4);
}

// Converts the input spherical coordinates to the appropriate representation for rendering
vec3 convertSphericalCoordinates(const vec3 sphericalCoords)
{
	return vec3(convertIncidentAngles(sphericalCoords.xy), convertDepth(sphericalCoords.z));
}

// Returns the spherical coordinates of the input screen position
vec3 sphericalCoordinatesOffAxis(const ivec2 screenPos, const float depth)
{
	return convertSphericalCoordinates(screenToSphericalCoordinates(screenPos, sTiledSplatBlurData.vResolution, depth));
}

// Returns the spherical coordinates of the input screen position
vec3 sphericalCoordinatesOnAxis(const ivec2 screenPos, const float depth)
{
	return vec3(0.0, 0.0, sphericalCoordinatesOffAxis(screenPos, depth).z);
	//return vec3(0.0, 0.0, convertDepth(camSpaceDepth(depth)));
}

// Returns the spherical coordinates of the input screen position
vec3 sphericalCoordinates(const ivec2 screenPos, const float depth)
{
	return (sTiledSplatBlurData.uiPsfAxisMethod == PsfAxisMethod_OffAxis) ?
		sphericalCoordinatesOffAxis(screenPos, depth) :
		sphericalCoordinatesOnAxis(screenPos, depth);
}

////////////////////////////////////////////////////////////////////////////////
// Splatting and sorting-related functions
////////////////////////////////////////////////////////////////////////////////

// Computes the fragment's normalized distance from the bottom left corner
float fragmentDist(const vec2 screenPos, const vec2 resolution)
{
	return (screenPos.y * resolution.x + screenPos.x) / (resolution.x * sTiledSplatBlurData.vResolution.y);
}

// Computes the sorting depth value
float computeSortDepth(const vec2 screenPosition, const float depth)
{
	const float offset = fragmentDist(screenPosition, vec2(sTiledSplatBlurData.vResolution)) * sTiledSplatBlurData.fSortDepthOffset;
	const float scale = (1 + sTiledSplatBlurData.fSortDepthScale);
	return (offset + depth) * scale;
}

// Computes the sorting depth value
float computeSortDepthLayered(const vec2 screenPosition, const float depth)
{
	const float offset = fragmentDist(screenPosition, vec2(sTiledSplatBlurData.vResolution)) * sTiledSplatBlurData.fSortDepthOffset;	
	const float psfIndex = ((1.0 / depth) - sTiledSplatBlurData.fObjectDistancesMin) / sTiledSplatBlurData.fObjectDistancesStep;
	const float psfIndexClamped = clamp(psfIndex, 0.0, float(sTiledSplatBlurData.uiNumObjectDistances - 1));
	return -floor(psfIndexClamped) * 1000.0 + offset;
}

////////////////////////////////////////////////////////////////////////////////
// Packs a splat index structure
SplatIndex makeSplatIndex(const uint elementIndex, const ivec2 tileId, const FragmentData fragmentData)
{
	SplatIndex result;
	result.uiTileId = packUint2x16(tileId);
	result.uiFragmentIndex = elementIndex;
	result.fFragmentDepth = computeSortDepth(fragmentData.vScreenPosition, fragmentData.vPsfIndex.z);
	//result.fFragmentDepth = computeSortDepthLayered(fragmentData.vScreenPosition, fragmentData.vPsfIndex.z);
	result.fBlurRadius = float(fragmentData.uiBlurRadius);
	result.vScreenPosition = fragmentData.vScreenPosition;
	return result;
}

////////////////////////////////////////////////////////////////////////////////
// Constructs a sort index from the provided data
SortIndex makeSortIndex(const uint elementIndex, const float depth)
{
	SortIndex result;
	result.uiIndex = elementIndex;
	result.fDepth = depth;
	return result;
}

// Constructs a sort index from the provided data
SortIndex makeSortIndex(const SplatIndex splatIndex)
{
	return makeSortIndex(splatIndex.uiFragmentIndex, splatIndex.fFragmentDepth);
}

// Constructs an empty sort index that is sorted to the end of the buffer
SortIndex nullSortIndex()
{
	return makeSortIndex(0, FLT_MAX);
}

// Sort the parameter sort indices in a front-to-back order
void sortElements(inout SortIndex a, inout SortIndex b)
{	
	// Sort the indices
	if (a.fDepth > b.fDepth)
	{
		SortIndex tmp = a;
		a = b;
		b = tmp;
	}
}