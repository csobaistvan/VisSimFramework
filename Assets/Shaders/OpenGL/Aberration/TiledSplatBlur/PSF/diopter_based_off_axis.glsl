
////////////////////////////////////////////////////////////////////////////////
// Intermediate storage for PSF interpolation purposes
////////////////////////////////////////////////////////////////////////////////

// PSF interpolation intermediate data buffer
UVEC4_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_10, sPsfInterpolationBuffer_);
#define sPsfInterpolationBuffer sPsfInterpolationBuffer_.sData

// Number of texture layers in total
#define uiNumCacheEntries (sTiledSplatBlurData.uiNumApertures * sTiledSplatBlurData.uiNumFocusDistances)
#define uiNumCacheTextureLayers(a,f) sPsfInterpolationBuffer[a * sTiledSplatBlurData.uiNumFocusDistances + f].xyz
#define uiNumTextureLayers sPsfInterpolationBuffer[uiNumCacheEntries].xyz
#define uiPsfLayerId(idx) sPsfInterpolationBuffer[uiNumCacheEntries + idx + 1]

////////////////////////////////////////////////////////////////////////////////
// Cache PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for single interpolated PSF
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_11, sCachePsfParamsBuffer_, 
	uvec3 uvTextureStartLayer; uvec3 uvNumTextureLayers; float fBlurRadius);
#define sCachePsfParamsBuffer sCachePsfParamsBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Interpolated PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for single interpolated PSF
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_12, sInterpolatedPsfParamBuffer_, 
	uvec3 uvTextureStartLayer; uvec3 uvNumTextureLayers; float fBlurRadius);
#define sInterpolatedPsfParamBuffer sInterpolatedPsfParamBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// PSF radius functions
////////////////////////////////////////////////////////////////////////////////

// Interpolates the blur radius across the PSF index and incident angle axes
float lerpPsfRadiusCacheDeg(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f, const uint c)
{
	return lerpPsfProperty
	(	
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[0]), uint(h[0]), uint(v[0]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[1]), uint(h[0]), uint(v[0]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[0]), uint(h[1]), uint(v[0]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[1]), uint(h[1]), uint(v[0]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[0]), uint(h[0]), uint(v[1]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[1]), uint(h[0]), uint(v[1]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[0]), uint(h[1]), uint(v[1]), a, f, c)),
		psfBlurRadiusDeg(makePsfArrayIndex(uint(d[1]), uint(h[1]), uint(v[1]), a, f, c)),
		vec3(fract(d[2]), fract(h[2]), fract(v[2]))
	);
}

// Interpolates the blur radius across the aperture size and focus distance axes
float lerpPsfRadiusDeg(const uint d, const uint h, const uint v, const vec3 a, const vec3 f, const uint c)
{
	return lerpPsfProperty
	(	
		psfBlurRadiusDeg(makePsfArrayIndex(d, h, v, uint(a[0]), uint(f[0]), c)),
		psfBlurRadiusDeg(makePsfArrayIndex(d, h, v, uint(a[1]), uint(f[0]), c)),
		psfBlurRadiusDeg(makePsfArrayIndex(d, h, v, uint(a[0]), uint(f[1]), c)),
		psfBlurRadiusDeg(makePsfArrayIndex(d, h, v, uint(a[1]), uint(f[1]), c)),
		vec2(fract(a[2]), fract(f[2]))
	);
}

// Interpolates the blur radius across the aperture size and focus distance axes
float lerpPsfRadiusPx(const uint d, const uint h, const uint v, const vec3 a, const vec3 f, const uint c)
{
	return projectBlurRadius(lerpPsfRadiusDeg(d, h, v, a, f, c));
}

// Interpolates the blur radius across the PSF index and incident angle axes
float lerpPsfRadiusCachePx(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f, const uint c)
{
	return projectBlurRadius(lerpPsfRadiusCacheDeg(d, h, v, a, f, c));
}

// Interpolates and projects the blur radius and stores it in the output buffer
void storeLerpedPsfRadius(const uint d, const uint h, const uint v, const vec3 a, const vec3 f, const uint c)
{
	sInterpolatedPsfParamBuffer[psfParamArrayIndex(d, h, v, c)].fBlurRadius = lerpPsfRadiusPx(d, h, v, a, f, c);
}

// Returns the blur radius for the parameter indices
float interpolatedPsfBlurRadius(const uint d, const uint h, const uint v, const uint c)
{
	return sInterpolatedPsfParamBuffer[psfParamArrayIndex(d, h, v, c)].fBlurRadius;
}

// Interpolates the blur radius across the PSF index and incident angle axes
float lerpPsfRadius(const vec3 d, const vec3 h, const vec3 v, const uint c)
{
	return lerpPsfProperty
	(	
		interpolatedPsfBlurRadius(uint(d[0]), uint(h[0]), uint(v[0]), c),
		interpolatedPsfBlurRadius(uint(d[1]), uint(h[0]), uint(v[0]), c),
		interpolatedPsfBlurRadius(uint(d[0]), uint(h[1]), uint(v[0]), c),
		interpolatedPsfBlurRadius(uint(d[1]), uint(h[1]), uint(v[0]), c),
		interpolatedPsfBlurRadius(uint(d[0]), uint(h[0]), uint(v[1]), c),
		interpolatedPsfBlurRadius(uint(d[1]), uint(h[0]), uint(v[1]), c),
		interpolatedPsfBlurRadius(uint(d[0]), uint(h[1]), uint(v[1]), c),
		interpolatedPsfBlurRadius(uint(d[1]), uint(h[1]), uint(v[1]), c),
		vec3(fract(d[2]), fract(h[2]), fract(v[2]))
	);
}

// Computes the blur radii for the parameter PSF indices
vec3 blurRadii(const vec3 sphericalCoords, const uint channelId)
{
    // calculate the corresponding PSF index
    const vec3 horizontalIds = horizontalAngleIndices(sphericalCoords.x);
	const vec3 verticalIds = verticalAngleIndices(sphericalCoords.y);
	const vec3 psfIds = objectDistanceIndices(sphericalCoords.z);

    // return the clamped blur radius
    return makeIndices(lerpPsfRadius(psfIds, horizontalIds, verticalIds, channelId));
}

// Compute the largest blur radius across all channels
float maxLerpedBlurRadius(const vec3 psfIds, const vec3 horizontalIds, const vec3 verticalIds)
{
	float result = 0.0;
	for (uint channelId = 0; channelId < sTiledSplatBlurData.uiRenderChannels; ++channelId)
		result = max(result, ceil(lerpPsfRadius(psfIds, horizontalIds, verticalIds, channelId)));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
// PSF layers functions
////////////////////////////////////////////////////////////////////////////////

// Calculates neighboring PSF offsets
const uvec4 _psfIdOffsets[4] = { uvec4(1, 0, 0, 0), uvec4(0, 1, 0, 0), uvec4(0, 0, 1, 0), uvec4(0, 0, 0, 1) };
uvec4 neighboringPsfIndex(const uint ax, const uint delta, const uint d, const uint h, const uint v, const uint c)
{
	const uvec4 psfId = uvec4(h, v, d, c) + delta * _psfIdOffsets[ax];
	return uvec4(psfId.z, psfId.x, psfId.y, psfId.w);
}

// Calculates the number of texture layers necessary
uint calcNumTextureLayersNeeded(const uint ax, const uint d, const uint h, const uint v, const uint a, const uint f, const uint c)
{
	const uvec4 psfId0 = neighboringPsfIndex(ax, 0, d, h, v, c);
	const uvec4 psfId1 = neighboringPsfIndex(ax, 1, d, h, v, c);
	return calcNumTextureLayers
	(
		psfBlurRadiusPx(makePsfArrayIndex(psfId0.x, psfId0.y, psfId0.z, a, f, psfId0.w)),
		psfBlurRadiusPx(makePsfArrayIndex(psfId1.x, psfId1.y, psfId1.z, a, f, psfId1.w)),
		sTiledSplatBlurData.vPsfLayersS[ax], 
		sTiledSplatBlurData.vPsfLayersP[ax]
	);
}

// Calculates the number of texture layers necessary
uint calcNumTextureLayersNeeded(const uint ax, const uint d, const uint h, const uint v, const uint c)
{
	const uvec4 psfId0 = neighboringPsfIndex(ax, 0, d, h, v, c);
	const uvec4 psfId1 = neighboringPsfIndex(ax, 1, d, h, v, c);
	return calcNumTextureLayers
	(
		interpolatedPsfBlurRadius(psfId0.x, psfId0.y, psfId0.z, psfId0.w), 
		interpolatedPsfBlurRadius(psfId1.x, psfId1.y, psfId1.z, psfId1.w),
		sTiledSplatBlurData.vPsfLayersS[ax], 
		sTiledSplatBlurData.vPsfLayersP[ax]
	);
}

// Calculates the cache's PSF index
uvec4 cachePsfIndex(const uint ax, const uint axid)
{
	const uvec4 psfId = axid * _psfIdOffsets[ax];
	return uvec4(psfId.z, psfId.x, psfId.y, psfId.w);
}

// Calculates the number of texture layers necessary
uint calcNumTextureLayersNeeded(const uint ax, const uint axid, const vec3 a, const vec3 f)
{
	// Calculate the base PSF index
	const uvec4 psfId = cachePsfIndex(ax, axid);
	
	// Calculate the 4 PSF ID's for the variable aperture and focus settings
	const uvec4 idx = uvec4
	(
		psfParamArrayIndex(psfId.x, psfId.y, psfId.z, uint(a[0]), uint(f[0]), 0),
		psfParamArrayIndex(psfId.x, psfId.y, psfId.z, uint(a[1]), uint(f[0]), 0),
		psfParamArrayIndex(psfId.x, psfId.y, psfId.z, uint(a[0]), uint(f[1]), 0),
		psfParamArrayIndex(psfId.x, psfId.y, psfId.z, uint(a[1]), uint(f[1]), 0)
	);

	// Return the maximum layer count
	return max
	(
		max
		(
			sCachePsfParamsBuffer[idx[0]].uvNumTextureLayers[ax],
			sCachePsfParamsBuffer[idx[1]].uvNumTextureLayers[ax]
		),
		max
		(
			sCachePsfParamsBuffer[idx[2]].uvNumTextureLayers[ax],
			sCachePsfParamsBuffer[idx[3]].uvNumTextureLayers[ax]
		)
	);
}

// Stores the number of texture layers needed
void storeTextureLayerInfo(const uint d, const uint h, const uint v, const uint a, const uint f, const uint ax, const uint sl, const uint nl)
{
	const uint idx = psfParamArrayIndex(d, h, v, a, f, 0);
	sCachePsfParamsBuffer[idx].uvTextureStartLayer[ax] = sl;
	sCachePsfParamsBuffer[idx].uvNumTextureLayers[ax] = nl;
}

// converts the input layer id to the corresponding PSF index
vec3 layerIdsToPsfIndices(const uint d, const uint h, const uint v, const uint a, const uint f, const uvec3 l)
{
	const uint idx = psfParamArrayIndex(d, h, v, a, f, 0);
    const uvec3 s = sCachePsfParamsBuffer[idx].uvTextureStartLayer;
    const uvec3 n = sCachePsfParamsBuffer[idx].uvNumTextureLayers;
    return vec3(h, v, d) + vec3(l - s) / vec3(n);
}

// converts the input psf indices to the corresponding number of layers
uvec3 psfIndexNumTotalLayers(const uint a, const uint f)
{
	return uiNumCacheTextureLayers(a, f);
}

// converts the input psf indices to the corresponding layer IDs, for the cache texture
vec3 psfIndexToLayerId(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f)
{
	// Get the first layer and the number of layers
	const uint idx = psfParamArrayIndex(uint(d[0]), uint(h[0]), uint(v[0]), a, f, 0);
	const vec3 startLayer = vec3(sCachePsfParamsBuffer[idx].uvTextureStartLayer);
	const vec3 numLayers = vec3(sCachePsfParamsBuffer[idx].uvNumTextureLayers);

    // Return the texture layer
    return startLayer + numLayers * vec3(fract(h[2]), fract(v[2]), fract(d[2]));
}

// Stores the number of texture layers needed
void storeTextureLayerInfo(const uint d, const uint h, const uint v, const uint ax, const uint sl, const uint nl)
{
	const uint idx = psfParamArrayIndex(d, h, v, 0);
	sInterpolatedPsfParamBuffer[idx].uvTextureStartLayer[ax] = sl;
	sInterpolatedPsfParamBuffer[idx].uvNumTextureLayers[ax] = nl;
}

// converts the input layer id to the corresponding PSF index
vec3 layerIdsToPsfIndices(const uint d, const uint h, const uint v, const uvec3 l)
{
	const uint idx = psfParamArrayIndex(d, h, v, 0);
    const uvec3 s = sInterpolatedPsfParamBuffer[idx].uvTextureStartLayer;
    const uvec3 n = sInterpolatedPsfParamBuffer[idx].uvNumTextureLayers;
    return vec3(h, v, d) + vec3(l - s) / vec3(n);
}

// converts the input psf indices to the corresponding layer IDs
vec3 psfIndexToLayerId(const vec3 d, const vec3 h, const vec3 v)
{
	// Get the first layer and the number of layers
	const uint idx = psfParamArrayIndex(uint(d[0]), uint(h[0]), uint(v[0]), 0);
	const vec3 startLayer = vec3(sInterpolatedPsfParamBuffer[idx].uvTextureStartLayer);
	const vec3 numLayers = vec3(sInterpolatedPsfParamBuffer[idx].uvNumTextureLayers);

    // Return the texture layer
    return startLayer + numLayers * vec3(fract(h[2]), fract(v[2]), fract(d[2]));
}

////////////////////////////////////////////////////////////////////////////////
// PSF weights functionality
////////////////////////////////////////////////////////////////////////////////

// Interpolates the PSF weights
float lerpPsfWeight(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f, const uint c, const uint r, const uvec2 sc)
{
	return lerpPsfProperty
	(	
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(h[0]), uint(v[0]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(h[0]), uint(v[0]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(h[1]), uint(v[0]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(h[1]), uint(v[0]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(h[0]), uint(v[1]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(h[0]), uint(v[1]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(h[1]), uint(v[1]), a, f, c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(h[1]), uint(v[1]), a, f, c), r, sc),
		vec3(fract(d[2]), fract(h[2]), fract(v[2]))
	);
}

// Samples the PSF weight buffer for a given radius, sample coordinate and PSF coordinates
float samplePsfWeightBufferPadded(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f, const uint c, const uint r, const uvec2 scp)
{
	// Remove padding from the sample coordinates
    const ivec2 sc = ivec2(scp - sTiledSplatBlurData.uiMaxBlurRadiusGlobal + r);

    // ignore coordinates that fall outside the padding region
	if (any(lessThan(sc, ivec2(0))) || any(greaterThanEqual(sc, ivec2(2 * r + 1)))) return 0.0;
    
    // Calculate the corresponding PSF coordinates and return the sample
    return lerpPsfWeight(d, h, v, a, f, c, r, sc);
}

// Samples the PSF weight buffer for a given radius, sample coordinate and PSF coordinates
float samplePsfWeightBufferPadded(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f, const uint c, const vec3 r, const uvec2 scp)
{
    return lerpPsfProperty
	(
		samplePsfWeightBufferPadded(d, h, v, a, f, c, uint(r[0]), scp),
		samplePsfWeightBufferPadded(d, h, v, a, f, c, uint(r[1]), scp),
		fract(r[2])
	);
}

// Samples the PSF weight buffer for a given radius, sample coordinate and PSF coordinates
vec4 samplePsfWeightBufferPaddedAllChannels(const vec3 d, const vec3 h, const vec3 v, const uint a, const uint f, const uvec2 scp)
{
    vec4 result = vec4(0, 0, 0, 1);
    for (uint c = 0; c < sTiledSplatBlurData.uiNumChannels; ++c)
        result[c] = samplePsfWeightBufferPadded(d, h, v, a, f, c, makeIndices(lerpPsfRadiusCachePx(d, h, v, a, f, c)), scp);
	return result;
}

// Determines whether the input sample coordinates fall into the padding region or not
bool isSamplePadding(const ivec2 sampleCoords, const float radius)
{
	return any(lessThan(sampleCoords, ivec2(int(sTiledSplatBlurData.uiMaxBlurRadiusGlobal) - radius))) || 
           any(greaterThan(sampleCoords, ivec2(int(sTiledSplatBlurData.uiMaxBlurRadiusGlobal) + radius)));
}

// Determines whether the input sample coordinates fall into the padding region or not
bool isSamplePadding(const ivec2 sampleCoords, const vec3 psfIds, const vec3 horizontalIds, const vec3 verticalIds)
{
	return isSamplePadding(sampleCoords, maxLerpedBlurRadius(psfIds, horizontalIds, verticalIds));
}

////////////////////////////////////////////////////////////////////////////////
// PSF cache interpolation command
////////////////////////////////////////////////////////////////////////////////

// Calculate the number of layers for the parameter axis 'AXIS', looping over PSF axes 'L0' and 'L1'
#define CALC_NUMBER_OF_LAYERS_CACHE(AXIS_ID, AXIS, L0, L1) \
    for (int AXIS##Id = 0; AXIS##Id < sTiledSplatBlurData.uiNum##AXIS; ++AXIS##Id) { \
        uint maxNumLayers = 1; \
        if (AXIS##Id < (sTiledSplatBlurData.uiNum##AXIS - 1)) { \
            for (int L0##Id = 0; L0##Id < sTiledSplatBlurData.uiNum##L0; ++L0##Id) \
            for (int L1##Id = 0; L1##Id < sTiledSplatBlurData.uiNum##L1; ++L1##Id) \
            for (int ChannelsId = 0; ChannelsId < sTiledSplatBlurData.uiNumChannels; ++ChannelsId) \
                maxNumLayers = max(maxNumLayers, calcNumTextureLayersNeeded(AXIS_ID, ObjectDistancesId, HorizontalAnglesId, VerticalAnglesId, apertureId, focusId, ChannelsId)); \
        } \
        for (uint i = uiNumTextureLayers[AXIS_ID]; i < (uiNumTextureLayers[AXIS_ID] + maxNumLayers); ++i) \
            uiPsfLayerId(i)[AXIS_ID] = AXIS##Id; \
        for (int L0##Id = 0; L0##Id < sTiledSplatBlurData.uiNum##L0; ++L0##Id) \
        for (int L1##Id = 0; L1##Id < sTiledSplatBlurData.uiNum##L1; ++L1##Id) \
			storeTextureLayerInfo(ObjectDistancesId, HorizontalAnglesId, VerticalAnglesId, apertureId, focusId, AXIS_ID, uiNumTextureLayers[AXIS_ID], maxNumLayers); \
        uiNumTextureLayers[AXIS_ID] += maxNumLayers; \
    }

void emitPsfCacheInterpolationCommand(const uint apertureId, const uint focusId)
{
    // Calculate the number of texture layers needed
    uiNumTextureLayers = uvec3(0);
    CALC_NUMBER_OF_LAYERS_CACHE(0, HorizontalAngles, VerticalAngles, ObjectDistances);
    CALC_NUMBER_OF_LAYERS_CACHE(1, VerticalAngles, ObjectDistances, HorizontalAngles)
    CALC_NUMBER_OF_LAYERS_CACHE(2, ObjectDistances, HorizontalAngles, VerticalAngles);

	// Store the total layer count
	uiNumCacheTextureLayers(apertureId, focusId) = uiNumTextureLayers;

    // How many threads are needed for the X and Y samples
    const uint maxPsfDiameter = sTiledSplatBlurData.uiMaxBlurRadiusGlobal * 2 + 1;
    const uvec3 finalTextureSize = uiNumCacheTextureLayers(apertureId, focusId) * uvec3(maxPsfDiameter, maxPsfDiameter, 1);
    const uvec3 interpolationGroupSize = uvec3(INTERPOLATION_GROUP_SIZE, INTERPOLATION_GROUP_SIZE, 1);
    sTileDispatchBuffer[0].vNumGroups = ROUNDED_DIV(finalTextureSize, interpolationGroupSize);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture cache interpolation
///////////////////////////////////////////////////////////////////////////////

// Interpolates one pixel of the the PSF texture
void interpolatePsfTextureCacheSample(const uint apertureId, const uint focusId)
{
    // Total blur diameter
    const int maxPsfDiameter = int(sTiledSplatBlurData.uiMaxBlurRadiusGlobal) * 2 + 1;

	// Ignore coordinates that fall outside the current PSF texture sub-region
    if (any(greaterThanEqual(gl_GlobalInvocationID, uvec3(maxPsfDiameter, maxPsfDiameter, 1) * uiNumTextureLayers))) return;

    // Coordinates for the current sample
	const uvec3 layerIds = uvec3(gl_GlobalInvocationID.xy / maxPsfDiameter, gl_GlobalInvocationID.z);
	const ivec2 sampleCoords = ivec2(gl_GlobalInvocationID.xy) % ivec2(maxPsfDiameter);

	// Ignore unused layers
	if (any(greaterThanEqual(layerIds, uiNumTextureLayers))) return;
    
    // Starting PSF slice indices
    const uint horizontalId = uiPsfLayerId(layerIds.x).x;
    const uint verticalId = uiPsfLayerId(layerIds.y).y;
    const uint psfId = uiPsfLayerId(layerIds.z).z;
	const vec3 psfIndices = layerIdsToPsfIndices(psfId, horizontalId, verticalId, apertureId, focusId, layerIds);

    // Interpolation indices
    const vec3 horizontalIds = horizontalAngleIndicesFromIndex(psfIndices.x);
    const vec3 verticalIds = verticalAngleIndicesFromIndex(psfIndices.y);
    const vec3 psfIds = objectDistanceIndicesFromIndex(psfIndices.z);
    
    // Compute and store the lerped weights for each channel 
    const vec4 textureValue = samplePsfWeightBufferPaddedAllChannels(psfIds, horizontalIds, verticalIds, apertureId, focusId, sampleCoords);
	//textureValue *= 100;
	//textureValue = samplePsfWeightBufferPaddedAllChannels(vec3(0), vec3(0), vec3(0), 0, 0, sampleCoords);
	//textureValue = vec4(vec2(sampleCoords) / vec2(maxPsfDiameter), 0, 1 );
	//textureValue = vec4(vec3(horizontalIds.z, verticalIds.z, psfIds.z) / vec3(sTiledSplatBlurData.uiNumHorizontalAngles, sTiledSplatBlurData.uiNumVerticalAngles, sTiledSplatBlurData.uiNumObjectDistances), 1 );
	
    // Store the interpolated value in the psf texture
    imageStore(sPsfImage, ivec3(gl_GlobalInvocationID), textureValue);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture cache parameters interpolation
///////////////////////////////////////////////////////////////////////////////

// Calculate the number of layers for the parameter axis 'AXIS', looping over PSF axes 'L0' and 'L1'
#define CALC_NUMBER_OF_LAYERS_FIXED(AXIS_ID, AXIS, L0, L1) \
    for (int AXIS##Id = 0; AXIS##Id < sTiledSplatBlurData.uiNum##AXIS; ++AXIS##Id) { \
        uint maxNumLayers = 1; \
        if (AXIS##Id < (sTiledSplatBlurData.uiNum##AXIS - 1)) \
			maxNumLayers = max(maxNumLayers, calcNumTextureLayersNeeded(AXIS_ID, AXIS##Id, vec3(0.0), vec3(0.0))); \
        for (uint i = uiNumTextureLayers[AXIS_ID]; i < (uiNumTextureLayers[AXIS_ID] + maxNumLayers); ++i) \
            uiPsfLayerId(i)[AXIS_ID] = AXIS##Id; \
        for (int L0##Id = 0; L0##Id < sTiledSplatBlurData.uiNum##L0; ++L0##Id) \
        for (int L1##Id = 0; L1##Id < sTiledSplatBlurData.uiNum##L1; ++L1##Id) \
            storeTextureLayerInfo(ObjectDistancesId, HorizontalAnglesId, VerticalAnglesId, AXIS_ID, uiNumTextureLayers[AXIS_ID], maxNumLayers); \
        uiNumTextureLayers[AXIS_ID] += maxNumLayers; \
    }

void interpolatePsfCacheParams()
{
	// Generate the PSF params if the full PSF is cached (fixed aperture and focus settings)
	if (sTiledSplatBlurData.uiNumApertures <= 1 || sTiledSplatBlurData.uiNumFocusDistances <= 1)
	{
		// Texture layers
		uiNumTextureLayers = uvec3(0);
		CALC_NUMBER_OF_LAYERS_FIXED(0, HorizontalAngles, VerticalAngles, ObjectDistances);
		CALC_NUMBER_OF_LAYERS_FIXED(1, VerticalAngles, ObjectDistances, HorizontalAngles)
		CALC_NUMBER_OF_LAYERS_FIXED(2, ObjectDistances, HorizontalAngles, VerticalAngles);

		// PSF radii
		for (int psfIndex = 0; psfIndex < sTiledSplatBlurData.uiNumObjectDistances; ++psfIndex)
		for (int horizontalIndex = 0; horizontalIndex < sTiledSplatBlurData.uiNumHorizontalAngles; ++horizontalIndex)
		for (int verticalIndex = 0; verticalIndex < sTiledSplatBlurData.uiNumVerticalAngles; ++verticalIndex)
		for (int channelId = 0; channelId < sTiledSplatBlurData.uiNumChannels; ++channelId)
		{
			storeLerpedPsfRadius(psfIndex, horizontalIndex, verticalIndex, vec3(0.0), vec3(0.0), channelId);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// PSF param interpolation command
////////////////////////////////////////////////////////////////////////////////

void emitPsfTextureParametersCommand()
{
    const uvec3 numParameters = uvec3(
		sTiledSplatBlurData.uiNumObjectDistances, 
		sTiledSplatBlurData.uiNumHorizontalAngles, 
		sTiledSplatBlurData.uiNumVerticalAngles);
    const uvec3 interpolationGroupSize = uvec3(INTERPOLATION_GROUP_SIZE, INTERPOLATION_GROUP_SIZE, 1);
    sTileDispatchBuffer[0].vNumGroups = ROUNDED_DIV(numParameters, interpolationGroupSize);
}

////////////////////////////////////////////////////////////////////////////////
// PSF param interpolation
////////////////////////////////////////////////////////////////////////////////

void interpolatePsfParams()
{
    // Aperture and focus indices
    const vec3 apertureIds = apertureIndices(sCameraData.fAperture);
    const vec3 focusIds = focusDistanceIndices(sCameraData.fFocusDistance);

    // Interpolate the PSF blur radii
    for (int channelId = 0; channelId < sTiledSplatBlurData.uiNumChannels; ++channelId)
        storeLerpedPsfRadius(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, apertureIds, focusIds, channelId);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture interpolation command
////////////////////////////////////////////////////////////////////////////////

// Calculate the number of layers for the parameter axis 'AXIS', looping over PSF axes 'L0' and 'L1'
#define CALC_NUMBER_OF_LAYERS_CURRENT(AXIS_ID, AXIS, L0, L1) \
    for (int AXIS##Id = 0; AXIS##Id < sTiledSplatBlurData.uiNum##AXIS; ++AXIS##Id) { \
        uint maxNumLayers = 1; \
        if (AXIS##Id < (sTiledSplatBlurData.uiNum##AXIS - 1)) \
			maxNumLayers = max(maxNumLayers, calcNumTextureLayersNeeded(AXIS_ID, AXIS##Id, apertureIds, focusIds)); \
        for (uint i = uiNumTextureLayers[AXIS_ID]; i < (uiNumTextureLayers[AXIS_ID] + maxNumLayers); ++i) \
            uiPsfLayerId(i)[AXIS_ID] = AXIS##Id; \
        for (int L0##Id = 0; L0##Id < sTiledSplatBlurData.uiNum##L0; ++L0##Id) \
        for (int L1##Id = 0; L1##Id < sTiledSplatBlurData.uiNum##L1; ++L1##Id) \
            storeTextureLayerInfo(ObjectDistancesId, HorizontalAnglesId, VerticalAnglesId, AXIS_ID, uiNumTextureLayers[AXIS_ID], maxNumLayers); \
        uiNumTextureLayers[AXIS_ID] += maxNumLayers; \
    }

// Generates the PSF texture interpolation command
void emitPsfTextureInterpolationCommand()
{	
    // Aperture and focus indices
    const vec3 apertureIds = apertureIndices(sCameraData.fAperture);
    const vec3 focusIds = focusDistanceIndices(sCameraData.fFocusDistance);

    uiNumTextureLayers = uvec3(0);
    CALC_NUMBER_OF_LAYERS_CURRENT(0, HorizontalAngles, VerticalAngles, ObjectDistances);
    CALC_NUMBER_OF_LAYERS_CURRENT(1, VerticalAngles, ObjectDistances, HorizontalAngles)
    CALC_NUMBER_OF_LAYERS_CURRENT(2, ObjectDistances, HorizontalAngles, VerticalAngles);

    // How many threads are needed for the X and Y samples
    const uint maxPsfDiameter = sTiledSplatBlurData.uiMaxBlurRadiusGlobal * 2 + 1;
	const uvec3 finalTextureSize = uvec3(uiNumTextureLayers.xyz) * uvec3(maxPsfDiameter, maxPsfDiameter, 1);
    const uvec3 interpolationGroupSize = uvec3(INTERPOLATION_GROUP_SIZE, INTERPOLATION_GROUP_SIZE, 1);
    sTileDispatchBuffer[0].vNumGroups = ROUNDED_DIV(finalTextureSize, interpolationGroupSize);
}

// Samples the parameter PSF cache
vec4 samplePsfCache(const uint cacheId, const ivec3 sampleCoords)
{
	return vec4(texelFetch(sPsfCache[cacheId], sampleCoords, 0).rgb, 1.0);
}

// Samples the four parameter PSF caches and interpolates the results
vec4 samplePsfCache(const vec3 apertureIds, const vec3 focusIds, const ivec3 sampleCoords)
{
	// Use the cache texture if we have more than 1 aperture or focus distance
	return lerp
	(
		lerp(samplePsfCache(0, sampleCoords), samplePsfCache(1, sampleCoords), fract(apertureIds[2])),
		lerp(samplePsfCache(2, sampleCoords), samplePsfCache(3, sampleCoords), fract(apertureIds[2])),
		fract(focusIds[2])
	);
}

// Interpolates one pixel of the the PSF texture
void interpolatePsfTextureSample()
{
	// Interpolate the current aperture and focus settings
	const vec3 apertureIds = apertureIndices(sCameraData.fAperture);
	const vec3 focusIds = focusDistanceIndices(sCameraData.fFocusDistance);

	// Interpolate the cache textures
	const vec4 textureValue = samplePsfCache(apertureIds, focusIds, ivec3(gl_GlobalInvocationID.xyz));

	// Write out the result
	imageStore(sPsfImage, ivec3(gl_GlobalInvocationID.xyz), textureValue);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture sampling
////////////////////////////////////////////////////////////////////////////////

// Init the PSF data
void initPsfData()
{}

// Transform depth to PSF index
vec3 sphericalCoordsToPsfIndex(const vec3 sphericalCoords)
{
    // calculate the corresponding PSF index
    const vec3 horizontalIds = horizontalAngleIndices(sphericalCoords.x);
	const vec3 verticalIds = verticalAngleIndices(sphericalCoords.y);
	const vec3 psfIds = objectDistanceIndices(sphericalCoords.z);
	//const vec3 psfIds = floor(objectDistanceIndices(sphericalCoords.z));

    // Calculate the corresponding layer IDs
	return psfIndexToLayerId(psfIds, horizontalIds, verticalIds);
}

// Samples the psf for a given fragment
vec3 samplePsfTexture(const vec2 sampleCoords, const uint blurRadius, const uint fragmentSize, const vec3 psfIndex)
{
	//return vec3(1.0 / float(blurRadius * blurRadius));

	// Skip irrelevant PSF samples
	if (dot(sampleCoords, sampleCoords) > float(blurRadius * blurRadius)) return vec3(0.0);
	
	// PSF center offset
	const uint maxPsfRadius = sTiledSplatBlurData.uiMaxBlurRadiusGlobal;
    const uint maxPsfDiameter = maxPsfRadius * 2 + 1;
	
	// Sample the 4 relevant PSF layers
	const vec3 texCoordBase = vec3(maxPsfRadius, maxPsfRadius, 0) + vec3(sampleCoords, 0);
	const vec3 texCoordLayerStride = vec3(maxPsfDiameter, maxPsfDiameter, 1);
	const vec3 texCoordLayerOffsets[4] = 
	{
		vec3(floor(psfIndex.x), floor(psfIndex.y), psfIndex.z),
		vec3( ceil(psfIndex.x), floor(psfIndex.y), psfIndex.z),
		vec3(floor(psfIndex.x),  ceil(psfIndex.y), psfIndex.z),
		vec3( ceil(psfIndex.x),  ceil(psfIndex.y), psfIndex.z)
	};

	// Sample the 4 neighboring PSFs and interpolate
	return lerp
	(
		lerp
		(
			samplePsfTexture(texCoordBase + texCoordLayerOffsets[0] * texCoordLayerStride), 
			samplePsfTexture(texCoordBase + texCoordLayerOffsets[1] * texCoordLayerStride), 
			fract(psfIndex.x)
		),
		lerp
		(
			samplePsfTexture(texCoordBase + texCoordLayerOffsets[2] * texCoordLayerStride), 
			samplePsfTexture(texCoordBase + texCoordLayerOffsets[3] * texCoordLayerStride), 
			fract(psfIndex.x)
		),
		fract(psfIndex.y)
	);
}

// Samples the psf for a given fragment
vec3 interpolatePsf(const vec2 fragmentPosition, const FragmentData fragmentData)
{
    return samplePsfTexture(
		fragmentPosition - fragmentData.vScreenPosition, 
		fragmentData.uiBlurRadius, 
		fragmentData.uiFragmentSize,
		fragmentData.vPsfIndex);
}