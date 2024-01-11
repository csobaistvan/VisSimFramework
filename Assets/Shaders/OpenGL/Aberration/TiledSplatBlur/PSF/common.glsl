
////////////////////////////////////////////////////////////////////////////////
// Pre-computed PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for each individual PSF
STRUCT_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_8, sPsfParamsBuffer_, 
	uint uiMinBlurRadius; uint uiMaxBlurRadius; uint uiWeightStartId; float fBlurRadiusDeg);
#define sPsfParamsBuffer sPsfParamsBuffer_.sData

// All PSF weights buffer
FLOAT_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_9, sPsfBuffer_);
#define sPsfBuffer sPsfBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Textures and images
////////////////////////////////////////////////////////////////////////////////

// Per-frame PSF weights texture and image
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler3D sPsfTexture;
layout (binding = TEXTURE_POST_PROCESS_2) uniform sampler3D sPsfCache[4];
layout (binding = 1, PSF_TEXTURE_TYPE) uniform restrict writeonly image3D sPsfImage;

////////////////////////////////////////////////////////////////////////////////
// PSF texture sampling
////////////////////////////////////////////////////////////////////////////////

// Samples the PSF texture
vec3 samplePsfTexture(const vec3 texCoordsIndices)
{
	const vec3 texCoords = (texCoordsIndices + vec3(0.5)) / vec3(textureSize(sPsfTexture, 0));
	return texture(sPsfTexture, texCoords).rgb;
}

// Samples the PSF cache texture
vec3 samplePsfCacheTexture(const uint cacheId, const vec3 texCoordsIndices)
{
	const vec3 texCoords = (texCoordsIndices + vec3(0.5)) / vec3(textureSize(sPsfCache[cacheId], 0));
	return texture(sPsfCache[cacheId], texCoords).rgb;
}

////////////////////////////////////////////////////////////////////////////////
// PSF index functions
////////////////////////////////////////////////////////////////////////////////

// Converts the parameter distance to dioptres
float dioptres(const float dist)
{
	return 1.0 / dist;
}

float centerIndex(const float idx)
{
	return floor(idx) + 0.5;
}

// Converts a single index to a collection of indices in the form (floor(id), ceil(id), id)
vec3 makeIndices(float index)
{
	return vec3(floor(index), ceil(index), index);
}

// Clamps the input PSF indices to a valid range
float clampPsfIndex(const float index, const uint numPsfs)
{
	return clamp(index, 0.0, float(numPsfs - 1));
}

// Clamps the input PSF indices to a valid range
vec3 clampPsfIndices(const vec3 indices, const uint numPsfs)
{
	return clamp(indices, vec3(0.0), vec3(float(numPsfs - 1)));
}

// Clamps the input PSF index to a valid range
vec3 clampPsfIndices(const float index, const uint numPsfs)
{
	return clampPsfIndices(makeIndices(index), numPsfs);
}

// Helper macro to generate the various PSF axis index calculators
#define PARAM_INDEX(FNAME, PNAME, TRANSFORM) \
	float FNAME##IndexUnclamped(const float val) \
	{ \
		return (TRANSFORM(val) - sTiledSplatBlurData.f##PNAME##Min) / sTiledSplatBlurData.f##PNAME##Step; \
	} \
	float FNAME##Index(const float val) \
	{ \
		return clampPsfIndex(FNAME##IndexUnclamped(val), sTiledSplatBlurData.uiNum##PNAME); \
	} \
	vec3 FNAME##IndicesFromIndex(const float index) \
	{ \
		return clampPsfIndices(index, sTiledSplatBlurData.uiNum##PNAME); \
	} \
	vec3 FNAME##Indices(const float val) \
	{ \
		return FNAME##IndicesFromIndex(FNAME##IndexUnclamped(val)); \
	} \

// Instantiate the PSF axis index calculator functions
PARAM_INDEX(objectDistance, ObjectDistances, dioptres)
PARAM_INDEX(aperture, Apertures, )
PARAM_INDEX(focusDistance, FocusDistances, dioptres)
PARAM_INDEX(horizontalAngle, HorizontalAngles, )
PARAM_INDEX(verticalAngle, VerticalAngles, )

////////////////////////////////////////////////////////////////////////////////
// PSF array indexing functions
////////////////////////////////////////////////////////////////////////////////

// Identifies a single PSF in the stack
struct PsfArrayIndex
{
	uint uiFocusDistance;
	uint uiAperture;
	uint uiObjectDistance;
	uint uiHorizontalAngle;
	uint uiVerticalAngle;
	uint uiWavelength;
};

// Makes a PSF index structure from the input psf indices
PsfArrayIndex makePsfArrayIndex(const uint d, const uint h, const uint v, const uint a, const uint f, const uint c)
{
	PsfArrayIndex result;
	result.uiFocusDistance = f;
	result.uiAperture = a;
	result.uiObjectDistance = d;
	result.uiHorizontalAngle = h;
	result.uiVerticalAngle = v;
	result.uiWavelength = c;
	return result;
}

// Makes a PSF index structure from the input psf indices
PsfArrayIndex makePsfArrayIndex(const uint d, const uint a, const uint f, const uint c)
{
	return makePsfArrayIndex(d, sTiledSplatBlurData.uiNumHorizontalAngles / 2, sTiledSplatBlurData.uiNumVerticalAngles / 2, a, f, c);
}

// Makes a PSF index structure from the input psf indices
PsfArrayIndex makePsfArrayIndex(const uint d, const uint c)
{
	return makePsfArrayIndex(d, 0, 0, c);
}

// Start index for the psf weight buffer
uint psfArrayIndex(const PsfArrayIndex index)
{
	return multiArrayIndex6
	(
		sTiledSplatBlurData.uiNumObjectDistances, 
		sTiledSplatBlurData.uiNumHorizontalAngles,
		sTiledSplatBlurData.uiNumVerticalAngles, 
		sTiledSplatBlurData.uiNumChannels,
		sTiledSplatBlurData.uiNumApertures, 
		sTiledSplatBlurData.uiNumFocusDistances, 
		index.uiObjectDistance, 
		index.uiHorizontalAngle, 
		index.uiVerticalAngle, 
		index.uiWavelength, 
		index.uiAperture, 
		index.uiFocusDistance
	);
}

// Index into the interpolated PSF parameter buffer
uint psfParamArrayIndex(const uint d, const uint c)
{
	return multiArrayIndex2
	(
		sTiledSplatBlurData.uiNumObjectDistances,
		sTiledSplatBlurData.uiNumChannels,
		d,
		c
	);
}

// Index into the interpolated PSF parameter buffer
uint psfParamArrayIndex(const uint d, const uint h, const uint v, const uint c)
{
	return multiArrayIndex4
	(
		sTiledSplatBlurData.uiNumObjectDistances,
		sTiledSplatBlurData.uiNumHorizontalAngles,
		sTiledSplatBlurData.uiNumVerticalAngles, 
		sTiledSplatBlurData.uiNumChannels,
		d,
		h,
		v,
		c
	);
}

// Index into the interpolated PSF parameter buffer
uint psfParamArrayIndex(const uint d, const uint h, const uint v, const uint a, const uint f, const uint c)
{
	return multiArrayIndex6
	(
		sTiledSplatBlurData.uiNumObjectDistances, 
		sTiledSplatBlurData.uiNumHorizontalAngles,
		sTiledSplatBlurData.uiNumVerticalAngles, 
		sTiledSplatBlurData.uiNumChannels,
		sTiledSplatBlurData.uiNumApertures, 
		sTiledSplatBlurData.uiNumFocusDistances, 
		d, 
		h, 
		v, 
		c, 
		a, 
		f
	);
}

////////////////////////////////////////////////////////////////////////////////
// Comnmon PSF interpolation functions
////////////////////////////////////////////////////////////////////////////////

// Interpolate the parameter PSF indices
float lerpPsfProperty(
	const float f0, const float f1, 
	const float alpha)
{
	return lerp(f0, f1, alpha);
}

// Interpolate the parameter PSF indices
float lerpPsfProperty(
	const float f00, const float f10, 
	const float f01, const float f11, 
	const vec2 alpha)
{
	return lerpPsfProperty(
		lerpPsfProperty(f00, f10, alpha[0]), 
		lerpPsfProperty(f01, f11, alpha[0]), 
		alpha[1]);
}

// Interpolate the parameter PSF indices
float lerpPsfProperty(
	const float f000, const float f100, 
	const float f010, const float f110, 
	const float f001, const float f101, 
	const float f011, const float f111, 
	const vec3 alpha)
{
	return lerpPsfProperty(
		lerpPsfProperty(f000, f100, f010, f110, alpha.xy), 
		lerpPsfProperty(f001, f101, f011, f111, alpha.xy), 
		alpha.z);
}

// Interpolate the parameter PSF indices
float lerpPsfProperty(
	const float f0000, const float f1000, 
	const float f0100, const float f1100, 
	const float f0010, const float f1010, 
	const float f0110, const float f1110, 
	const float f0001, const float f1001, 
	const float f0101, const float f1101, 
	const float f0011, const float f1011, 
	const float f0111, const float f1111, 
	const vec4 alpha)
{
	return lerpPsfProperty(
		lerpPsfProperty(f0000, f1000, f0100, f1100, f0010, f1010, f0110, f1110, alpha.xyz), 
		lerpPsfProperty(f0001, f1001, f0101, f1101, f0011, f1011, f0111, f1111, alpha.xyz), 
		alpha.w);
}

// Interpolate the parameter PSF indices
float lerpPsfProperty(
	const float f00000, const float f10000, 
	const float f01000, const float f11000, 
	const float f00100, const float f10100, 
	const float f01100, const float f11100, 
	const float f00010, const float f10010, 
	const float f01010, const float f11010, 
	const float f00110, const float f10110, 
	const float f01110, const float f11110, 
	const float f00001, const float f10001, 
	const float f01001, const float f11001, 
	const float f00101, const float f10101, 
	const float f01101, const float f11101, 
	const float f00011, const float f10011, 
	const float f01011, const float f11011, 
	const float f00111, const float f10111, 
	const float f01111, const float f11111, 
	const vec4 alpha, const float beta)
{
	return lerpPsfProperty(
		lerpPsfProperty(f00000, f10000, f01000, f11000, f00100, f10100, f01100, f11100, f00010, f10010, f01010, f11010, f00110, f10110, f01110, f11110, alpha), 
		lerpPsfProperty(f00001, f10001, f01001, f11001, f00101, f10101, f01101, f11101, f00011, f10011, f01011, f11011, f00111, f10111, f01111, f11111, alpha), 
		beta);
}

////////////////////////////////////////////////////////////////////////////////
// PSF blur radius interpolation functions
////////////////////////////////////////////////////////////////////////////////

// Clamps the parameter blur radius to a valid range
float clampBlurRadius(const float blurRadius)
{
    return clamp(blurRadius, 0.0, float(sTiledSplatBlurData.uiMaxBlurRadiusGlobal));
}

// Computes the projected blur radius
float projectBlurRadius(const float blurRadiusDeg)
{
	return clampBlurRadius((blurRadiusDeg / sTiledSplatBlurData.vCameraFov[1]) * sTiledSplatBlurData.vResolution[1]);
}

// Returns the blur radius for the parameter indices
float psfBlurRadiusDeg(const PsfArrayIndex index)
{
	return sPsfParamsBuffer[psfArrayIndex(index)].fBlurRadiusDeg;
}

// Returns the blur radius for the parameter indices
float psfBlurRadiusPx(const PsfArrayIndex index)
{
	return projectBlurRadius(sPsfParamsBuffer[psfArrayIndex(index)].fBlurRadiusDeg);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture layering functions
////////////////////////////////////////////////////////////////////////////////

// Calculates the reduced texture count
uint calcReducedNumLayers(const float md, const float s, const float p)
{
	const float reduction = 1.0 / max(pow(md * s, p), 1.0);
	return uint(max(1, md * reduction));
}

// Calculates the number of texture layers from two neighboring radii
uint calcNumTextureLayers(const float r0, const float r1, const float s, const float p)
{
	return calcReducedNumLayers(ceil(abs(r1 - r0)), s, p);
}

////////////////////////////////////////////////////////////////////////////////
// PSF weight interpolation functions
////////////////////////////////////////////////////////////////////////////////

// How many weights per channel for a single PSF entry
uint psfWeightsPerEntry(const int radius)
{
    // FullSimplify[Sum[(i * 2 + 1) * (i * 2 + 1), { i, 0, n }]]
    return ((1 + radius) * (1 + 2 * radius) * (3 + 2 * radius)) / 3;
}

// How many weights per channel for a single PSF entry
uint psfWeightsPerEntry(const int minRadius, const int maxRadius)
{
	// FullSimplify[Sum[(i * 2 + 1) * (i * 2 + 1), { i, n0, n1 }]]
	return (minRadius - 4 * minRadius * minRadius * minRadius + (1 + maxRadius) * (1 + 2 * maxRadius) * (3 + 2 * maxRadius)) / 3;
}

// Weight start index for a given PSF radius
uint psfWeightStartId(const uint minRadius, const uint radius)
{
	return (radius == minRadius ? 0 : psfWeightsPerEntry(int(minRadius), int(radius) - 1));
}

// Start offset of the PSF weights in the PSF weight buffer
uint psfStartOffset(const uint psfId, const uint radius, const uint minRadius)
{
	return sPsfParamsBuffer[psfId].uiWeightStartId + psfWeightStartId(minRadius, radius);
}

// Offset of the PSF sample within the current PSF
uint psfSampleOffset(const uint radius, const uvec2 sampleCoords)
{
	return (sampleCoords.y * (radius * 2 + 1) + sampleCoords.x);
}

// Index of the PSF sample within the PSF weight buffer
uint psfWeightIndex(const uint psfId, const uint radius, const uint minRadius, const uvec2 sampleCoords)
{
	return psfStartOffset(psfId, radius, minRadius) + psfSampleOffset(radius, sampleCoords);
}

// Returns the psf weight for the parameter indices
float psfWeight(const PsfArrayIndex index, const uint r, const uvec2 sc)
{
	const uint idx = psfArrayIndex(index);
	const uint minR = sPsfParamsBuffer[idx].uiMinBlurRadius;
	const uint maxR = sPsfParamsBuffer[idx].uiMaxBlurRadius;
	return (r >= minR && r <= maxR) ? sPsfBuffer[psfWeightIndex(idx, r, minR, sc)] : 0.0;
}

////////////////////////////////////////////////////////////////////////////////
// Sample scaling functions
////////////////////////////////////////////////////////////////////////////////

// Apply a uniform scaling of 1
float scaleFactorFragmentSizeOne(const uint fragmentSize)
{
	return 1.0;
}

// Scales the weight according to the corresponding fragment size
float scaleFactorFragmentSizeLinear(const uint fragmentSize)
{
	return fragmentSize;
}

// Scales the weight according to the corresponding fragment size
float scaleFactorFragmentSizeAreaCircle(const uint fragmentSize)
{
	return PI * (float(fragmentSize) * 0.5) * (float(fragmentSize) * 0.5);
}

// Scales the weight according to the corresponding fragment size
float scaleFactorFragmentSizeAreaSquare(const uint fragmentSize)
{
	return fragmentSize * fragmentSize;
}

// Scales the weight according to the corresponding fragment size
float scaleFactorFragmentSize(const uint fragmentSize)
{
	// No scaling
	if (sTiledSplatBlurData.uiWeightScaleMethod == WeightScaleMethod_One)
		return scaleFactorFragmentSizeOne(fragmentSize);

	// Linear scaling
	else if (sTiledSplatBlurData.uiWeightScaleMethod == WeightScaleMethod_Linear)
		return scaleFactorFragmentSizeLinear(fragmentSize);

	// Area-based scaling
	else if (sTiledSplatBlurData.uiWeightScaleMethod == WeightScaleMethod_AreaCircle)
		return scaleFactorFragmentSizeAreaCircle(fragmentSize);

	// Area-based scaling
	else if (sTiledSplatBlurData.uiWeightScaleMethod == WeightScaleMethod_AreaSquare)
		return scaleFactorFragmentSizeAreaSquare(fragmentSize);

    // Fallback to the original weight
    return 1.0;
}

// Applies the scale factor linearly to the input weight
vec3 rescaleWeightLinear(const vec3 weight, const float scaleFactor)
{
	return scaleFactor * weight;
}

// Applies alpha-blended scaling to the input weight
vec3 rescaleWeightAlpha(const vec3 weight, const float scaleFactor)
{    
    // https://math.stackexchange.com/questions/971761/calculating-sum-of-consecutive-powers-of-a-number
    // weight: sum(0, ..., n - 1){1 - w} * w =>
    //         ((1-w)^(n - 1 + 1) - 1) / (1 - w - 1) * w =>
    //         ((1-w)^n - 1) / (-w) * w =>
    //        -((1-w)^n - 1) =>
    //          1 - (1-w) ^ n
    return 1.0 - pow(1.0 - weight, vec3(scaleFactor));
}

// Chooses the appropriate scaling strategy and scales the weight accordingly
vec3 rescaleWeight(const vec3 weight, const float scaleFactor)
{
    // Linear scaling
    if (sTiledSplatBlurData.uiWeightRescaleMethod == WeightRescaleMethod_LinearRescale)
        return rescaleWeightLinear(weight, scaleFactor);

    // Alpha blending
    else if (sTiledSplatBlurData.uiWeightRescaleMethod == WeightRescaleMethod_AlphaBlend)
        return rescaleWeightAlpha(weight, scaleFactor);

    // Fall back to linear
    return rescaleWeightLinear(weight, scaleFactor);
}

// Chooses the appropriate scaling strategy and scales the weight accordingly
vec3 scaleWeight(const vec3 weight, const uint fragmentSize)
{
    return rescaleWeight(weight, scaleFactorFragmentSize(fragmentSize));
}

////////////////////////////////////////////////////////////////////////////////
// Overlap testing functions
////////////////////////////////////////////////////////////////////////////////

// Tests whether the center fragment with the parameter relative coordinates
// overlaps the fragment's blur radius
bool overlapsFragment(const vec2 relativeCoords, const float radius)
{
	return all(lessThanEqual(abs(relativeCoords), vec2(radius)));
}

// Tests whether a fragment overlaps the parameter tile
bool overlapsTile(const ivec2 tileId, const vec2 fragmentCoord, const float blurRadius)
{
	// The two corners of the neighboring tile.
	const vec2 tileMin = tileId * ivec2(TILE_SIZE);
	const vec2 tileMax = tileMin + ivec2(TILE_SIZE - 1);

	// Distance of the center fragment to the edges of the neighboring tile
	const vec2 distToMin = abs(tileMin - fragmentCoord);
	const vec2 distToMax = abs(tileMax - fragmentCoord);
	
	// Evaluate the side and corner conditions
	return
		// Vertical neighbors
		(fragmentCoord.x >= tileMin.x && fragmentCoord.x <= tileMax.x && (distToMin.y <= blurRadius || distToMax.y <= blurRadius)) ||

		// Horizontal neighbors
		(fragmentCoord.y >= tileMin.y && fragmentCoord.y <= tileMax.y && (distToMin.x <= blurRadius || distToMax.x <= blurRadius)) ||

		// Oblique neighbors
		(distToMin.x <= blurRadius && distToMin.y <= blurRadius) ||
		(distToMin.x <= blurRadius && distToMax.y <= blurRadius) ||
		(distToMax.x <= blurRadius && distToMin.y <= blurRadius) ||
		(distToMax.x <= blurRadius && distToMax.y <= blurRadius);
}

////////////////////////////////////////////////////////////////////////////////
// Layout-dependent PSF functionality
////////////////////////////////////////////////////////////////////////////////

#includeif(PSF_TEXTURE_DEPTH_LAYOUT_ID == PsfTextureDepthLayout_RadiusBased) <Shaders/OpenGL/Aberration/TiledSplatBlur/PSF/radius_based.glsl>
#includeif(PSF_TEXTURE_DEPTH_LAYOUT_ID == PsfTextureDepthLayout_DiopterBased && PSF_AXIS_METHOD_ID == PsfAxisMethod_OnAxis) <Shaders/OpenGL/Aberration/TiledSplatBlur/PSF/diopter_based_on_axis.glsl>
#includeif(PSF_TEXTURE_DEPTH_LAYOUT_ID == PsfTextureDepthLayout_DiopterBased && PSF_AXIS_METHOD_ID == PsfAxisMethod_OffAxis) <Shaders/OpenGL/Aberration/TiledSplatBlur/PSF/diopter_based_off_axis.glsl>

////////////////////////////////////////////////////////////////////////////////
// Blur radius functions
////////////////////////////////////////////////////////////////////////////////

// Compute the smallest blur radius across all channels
vec3 minBlurRadii(const vec3 sphericalCoords)
{
	vec3 result = vec3(FLT_MAX);
	for (uint channelId = 0; channelId < sTiledSplatBlurData.uiRenderChannels; ++channelId)
		result = min(result, blurRadii(sphericalCoords, channelId));
	return result;
}

// Compute the largest blur radius across all channels
vec3 maxBlurRadii(const vec3 sphericalCoords)
{
	vec3 result = vec3(0.0);
	for (uint channelId = 0; channelId < sTiledSplatBlurData.uiRenderChannels; ++channelId)
		result = max(result, blurRadii(sphericalCoords, channelId));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture sampling
////////////////////////////////////////////////////////////////////////////////

// Samples the psf for a given fragment
vec3 samplePsf(vec2 fragmentPosition, const FragmentData fragmentData)
{
	// Sample the PSF texture
	const vec3 weight = interpolatePsf(fragmentPosition, fragmentData);

	// Compute and return the scaled weight
	return saturate(scaleWeight(saturate(weight), fragmentData.uiFragmentSize));
}