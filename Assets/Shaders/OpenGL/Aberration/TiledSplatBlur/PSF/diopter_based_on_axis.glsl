
////////////////////////////////////////////////////////////////////////////////
// Intermediate storage for PSF interpolation purposes
////////////////////////////////////////////////////////////////////////////////

// PSF interpolation intermediate data buffer
UINT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_10, sPsfInterpolationBuffer_);
#define sPsfInterpolationBuffer sPsfInterpolationBuffer_.sData

// Number of texture layers in total
#define uiNumTextureLayers sPsfInterpolationBuffer[0]

////////////////////////////////////////////////////////////////////////////////
// Cache PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for single interpolated PSF
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_11, sCachePsfParamsBuffer_, 
	uint uiStartLayer; uint uiNumLayers; float fBlurRadius);
#define sCachePsfParamsBuffer sCachePsfParamsBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Interpolated PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for single interpolated PSF
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_12, sInterpolatedPsfParamBuffer_, 
	uint uiStartLayer; uint uiNumLayers; float fBlurRadius);
#define sInterpolatedPsfParamBuffer sInterpolatedPsfParamBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// PSF radius functions
////////////////////////////////////////////////////////////////////////////////

// Interpolates the blur radius across the aperture size and focus distance axes
float lerpPsfRadiusDeg(const uint d, const vec3 a, const vec3 f, const uint c)
{
	return lerpPsfProperty
	(	
		psfBlurRadiusDeg(makePsfArrayIndex(d, uint(a[0]), uint(f[0]), c)),
		psfBlurRadiusDeg(makePsfArrayIndex(d, uint(a[1]), uint(f[0]), c)),
		psfBlurRadiusDeg(makePsfArrayIndex(d, uint(a[0]), uint(f[1]), c)),
		psfBlurRadiusDeg(makePsfArrayIndex(d, uint(a[1]), uint(f[1]), c)),
		vec2(fract(a[2]), fract(f[2]))
	);
}

// Interpolates the blur radius across the aperture size and focus distance axes
float lerpPsfRadiusPx(const uint d, const vec3 a, const vec3 f, const uint c)
{
	return projectBlurRadius(lerpPsfRadiusDeg(d, a, f, c));
}

// Interpolates and projects the blur radius and stores it in the output buffer
void storeLerpedPsfRadius(const uint d, const vec3 a, const vec3 f, const uint c)
{            
	sInterpolatedPsfParamBuffer[psfParamArrayIndex(d, c)].fBlurRadius = lerpPsfRadiusPx(d, a, f, c);
}

// Returns the blur radius for the parameter indices
float interpolatedPsfBlurRadius(const uint d, const uint c)
{
	return sInterpolatedPsfParamBuffer[psfParamArrayIndex(d, c)].fBlurRadius;
}

// Interpolates the blur radius across the PSF index axis
float lerpPsfRadius(const vec3 d, const uint c)
{
	return lerpPsfProperty
	(
		interpolatedPsfBlurRadius(uint(d[0]), c),
		interpolatedPsfBlurRadius(uint(d[1]), c),
		fract(d[2])
	);
}

// Compute the largest blur radius across all channels
float maxLerpedBlurRadius(const vec3 psfIds)
{
	float result = 0.0;
	for (uint channelId = 0; channelId < sTiledSplatBlurData.uiRenderChannels; ++channelId)
		result = max(result, ceil(lerpPsfRadius(psfIds, channelId)));
	return result;
}

// Computes the blur radii for the parameter PSF indices
vec3 blurRadii(const vec3 sphericalCoords, const uint channelId)
{
    // calculate the corresponding PSF index
	const vec3 psfIds = objectDistanceIndices(sphericalCoords.z);

    // return the clamped blur radius
    return makeIndices(lerpPsfRadius(psfIds, channelId));
}

////////////////////////////////////////////////////////////////////////////////
// PSF layers functions
////////////////////////////////////////////////////////////////////////////////

// Calculates the number of texture layers necessary
uint calcNumTextureLayersNeededObjectDistances(const uint d, const uint c)
{
	return calcNumTextureLayers(interpolatedPsfBlurRadius(d, c), interpolatedPsfBlurRadius(d + 1, c),
        sTiledSplatBlurData.vPsfLayersS.z, sTiledSplatBlurData.vPsfLayersP.z);
}

// Stores the number of texture layers needed
void storeTextureLayerInfo(const uint d, const uint sl, const uint nl)
{
	const uint idx = psfParamArrayIndex(d, 0);
	sInterpolatedPsfParamBuffer[idx].uiStartLayer = sl;
	sInterpolatedPsfParamBuffer[idx].uiNumLayers = nl;
}

// converts the input layer id to the corresponding PSF index
float layerIdToPsfIndex(const uint d, const uint l)
{
	const uint idx = psfParamArrayIndex(d, 0);
    const uint s = sInterpolatedPsfParamBuffer[idx].uiStartLayer;
    const uint n = sInterpolatedPsfParamBuffer[idx].uiNumLayers;
    return d + float(l - s) / float(n);
}

// converts the input layer ID to the corresponding object distance indices
vec3 layerIdToObjectDistanceIndices(const uint d, const uint l)
{
	return objectDistanceIndicesFromIndex(layerIdToPsfIndex(d, l));
}

// converts the input psf index to the corresponding layer ID
float psfIndexToLayerId(const vec3 d)
{
	// Get the first layer and the number of layers
	const uint idx = psfParamArrayIndex(uint(d[0]), 0);
	const float startLayer = float(sInterpolatedPsfParamBuffer[idx].uiStartLayer);
	const float numLayers = float(sInterpolatedPsfParamBuffer[idx].uiNumLayers);

    // Return the texture layer
    return startLayer + numLayers * fract(d[2]);
}

////////////////////////////////////////////////////////////////////////////////
// PSF weights functionality
////////////////////////////////////////////////////////////////////////////////

// Determines whether the input sample coordinates fall into the padding region or not
bool isSamplePadding(const ivec2 sampleCoords, const float radius)
{
	return any(lessThan(sampleCoords, ivec2(int(sTiledSplatBlurData.uiMaxBlurRadiusCurrent) - radius))) || 
        any(greaterThan(sampleCoords, ivec2(int(sTiledSplatBlurData.uiMaxBlurRadiusCurrent) + radius)));
}

// Determines whether the input sample coordinates fall into the padding region or not
bool isSamplePadding(const ivec2 sampleCoords, const vec3 psfIds)
{
	return isSamplePadding(sampleCoords, maxLerpedBlurRadius(psfIds));
}

// Interpolates the PSF weights
float lerpPsfWeight(const uint d, const vec3 a, const vec3 f, const uint c, const uint r, const uvec2 sc)
{
	return lerpPsfProperty
	(	
		psfWeight(makePsfArrayIndex(d, uint(a[0]), uint(f[0]), c), r, sc),
		psfWeight(makePsfArrayIndex(d, uint(a[1]), uint(f[0]), c), r, sc),
		psfWeight(makePsfArrayIndex(d, uint(a[0]), uint(f[1]), c), r, sc),
		psfWeight(makePsfArrayIndex(d, uint(a[1]), uint(f[1]), c), r, sc),
		vec2(fract(a[2]), fract(f[2]))
	);
}

// Interpolates the PSF weights
float lerpPsfWeight(const vec3 d, const vec3 a, const vec3 f, const uint c, const uint r, const uvec2 sc)
{
	return lerpPsfProperty
	(	
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(a[0]), uint(f[0]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(a[0]), uint(f[0]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(a[1]), uint(f[0]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(a[1]), uint(f[0]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(a[0]), uint(f[1]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(a[0]), uint(f[1]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[0]), uint(a[1]), uint(f[1]), c), r, sc),
		psfWeight(makePsfArrayIndex(uint(d[1]), uint(a[1]), uint(f[1]), c), r, sc),
		vec3(fract(d[2]), fract(a[2]), fract(f[2]))
	);
}

// Samples the PSF weight buffer for a given radius, sample coordinate and PSF coordinates
float samplePsfWeightBufferPadded(const vec3 d, const vec3 a, const vec3 f, const uint c, const uint r, const uvec2 scp)
{
	// Remove padding from the sample coordinates
    const ivec2 sc = ivec2(scp - sTiledSplatBlurData.uiMaxBlurRadiusCurrent + r);

    // ignore coordinates that fall outside the padding region
	if (any(lessThan(sc, ivec2(0))) || any(greaterThanEqual(sc, ivec2(2 * r + 1)))) return 0.0;
    
    // Calculate the corresponding PSF coordinates and return the sample
    return lerpPsfWeight(d, a, f, c, r, sc);
}

// Samples the PSF weight buffer for a given radius, sample coordinate and PSF coordinates
float samplePsfWeightBufferPadded(const vec3 d, const vec3 a, const vec3 f, const uint c, const vec3 r, const uvec2 scp)
{
    return lerpPsfProperty
	(
		samplePsfWeightBufferPadded(d, a, f, c, uint(r[0]), scp),
		samplePsfWeightBufferPadded(d, a, f, c, uint(r[1]), scp),
		fract(r[2])
	);
}

// Samples the PSF weight buffer for a given radius, sample coordinate and PSF coordinates
vec4 samplePsfWeightBufferPaddedAllChannels(const vec3 d, const vec3 a, const vec3 f, const uvec2 scp)
{
    vec4 result = vec4(0, 0, 0, 1);
    for (uint c = 0; c < sTiledSplatBlurData.uiNumChannels; ++c)
        result[c] = samplePsfWeightBufferPadded(d, a, f, c, makeIndices(lerpPsfRadius(d, c)), scp);
	return result;
}

////////////////////////////////////////////////////////////////////////////////
// PSF cache interpolation command
////////////////////////////////////////////////////////////////////////////////

void emitPsfCacheInterpolationCommand(const uint apertureId, const uint focusId)
{
    // Do nothing
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture cache interpolation
///////////////////////////////////////////////////////////////////////////////

// Interpolates one pixel of the the PSF texture
void interpolatePsfTextureCacheSample(const uint apertureId, const uint focusId)
{
    // Do nothing
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture cache parameters interpolation
///////////////////////////////////////////////////////////////////////////////

void interpolatePsfCacheParams()
{
    // Do nothing
}

////////////////////////////////////////////////////////////////////////////////
// PSF param interpolation command
////////////////////////////////////////////////////////////////////////////////

void emitPsfTextureParametersCommand()
{
    sTileDispatchBuffer[0].vNumGroups = uvec3(1, 1, 1);
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
    for (int psfIndex = 0; psfIndex < sTiledSplatBlurData.uiNumObjectDistances; ++psfIndex)
    for (int channelId = 0; channelId < sTiledSplatBlurData.uiNumChannels; ++channelId)
    {
        storeLerpedPsfRadius(psfIndex, apertureIds, focusIds, channelId);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture interpolation command
////////////////////////////////////////////////////////////////////////////////

// Generates the PSF texture interpolation command
void emitPsfTextureInterpolationCommand()
{    
    // Starting texture slice for the next object distance
    uiNumTextureLayers = 0;
    for (uint psfIndex = 0; psfIndex < sTiledSplatBlurData.uiNumObjectDistances; ++psfIndex)
    {
        // Compute the total number of layers needed
        uint maxNumLayers = 1;
        if (psfIndex < (sTiledSplatBlurData.uiNumObjectDistances - 1))
            for (uint channelId = 0; channelId < sTiledSplatBlurData.uiNumChannels; ++channelId)
                maxNumLayers = max(maxNumLayers, calcNumTextureLayersNeededObjectDistances(psfIndex, channelId));

        // Write out the total number of layers needed
        storeTextureLayerInfo(psfIndex, uiNumTextureLayers, maxNumLayers);

        // Write out the PSF indices for the interpolation process
        for (uint i = uiNumTextureLayers; i < (uiNumTextureLayers + maxNumLayers); ++i)
            sPsfInterpolationBuffer[i + 1] = psfIndex;

        // Increment the starting index
        uiNumTextureLayers += maxNumLayers;
    }

    // Generate the PSF interpolation dispatch command
    const uint maxPsfDiameter = sTiledSplatBlurData.uiMaxBlurRadiusCurrent * 2 + 1;
    const uvec3 finalTextureSize = uvec3(maxPsfDiameter, maxPsfDiameter, uiNumTextureLayers);
    const uvec3 interpolationGroupSize = uvec3(INTERPOLATION_GROUP_SIZE, INTERPOLATION_GROUP_SIZE, 1);
    sTileDispatchBuffer[0].vNumGroups = ROUNDED_DIV(finalTextureSize, interpolationGroupSize);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture interpolation
///////////////////////////////////////////////////////////////////////////////

// Interpolates one pixel of the the PSF texture
void interpolatePsfTextureSample()
{
    // Coordinates for the current sample
	const ivec2 sampleCoords = ivec2(gl_GlobalInvocationID.xy);
    const uint layerId = gl_GlobalInvocationID.z;

    // Interpolation indices
    const vec3 apertureIds = apertureIndices(sCameraData.fAperture);
    const vec3 focusIds = focusDistanceIndices(sCameraData.fFocusDistance);
    const uint psfId = sPsfInterpolationBuffer[layerId + 1];
    const vec3 psfIds = layerIdToObjectDistanceIndices(psfId, layerId);

    // Ignore coordinates that fall outside the actual PSF area
    if ((gl_GlobalInvocationID.z >= uiNumTextureLayers)) return;
    if (isSamplePadding(sampleCoords, psfIds)) return;

    // Compute and store the lerped weights for each channel
    const vec4 textureValue = samplePsfWeightBufferPaddedAllChannels(psfIds, apertureIds, focusIds, sampleCoords);

    // Store the interpolated value in the psf texture
    imageStore(sPsfImage, ivec3(gl_GlobalInvocationID), textureValue);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture sampling
////////////////////////////////////////////////////////////////////////////////

// Init the PSF data
void initPsfData()
{

}

// Transform depth to PSF index
vec3 sphericalCoordsToPsfIndex(const vec3 sphericalCoords)
{
    // Get the corresponding PSF index
	const vec3 psfIds = objectDistanceIndices(sphericalCoords.z);

    // Maximum PSF sizes
    const uint maxPsfRadius = sTiledSplatBlurData.uiMaxBlurRadiusCurrent;
    const uint maxPsfDiameter = maxPsfRadius * 2 + 1;

    // Return the texture layer
    return vec3(maxPsfRadius, maxPsfRadius, psfIndexToLayerId(psfIds));
}

// Samples the PSF texture at the parameter sample coords and PSF index
vec3 samplePsfTexture(const vec2 sampleCoords, const vec3 psfIndex)
{
    // Sample the PSF texture and return it as the result
	const vec3 texCoordsIndices = vec3(sampleCoords, 0) + psfIndex;
    return samplePsfTexture(texCoordsIndices);
}

// Samples the psf for a given fragment
vec3 interpolatePsf(const vec2 fragmentPosition, const FragmentData fragmentData)
{
    return samplePsfTexture(
        fragmentPosition - fragmentData.vScreenPosition, 
        fragmentData.vPsfIndex);
}

/*
// Samples the PSF texture at the parameter sample coords and PSF index
vec3 samplePsfTexture(const vec2 sampleCoords, const uint blurRadius, const vec3 psfIndex)
{
	if (dot(sampleCoords, sampleCoords) > float(blurRadius * blurRadius)) return vec3(0.0);
    // Sample the PSF texture and return it as the result
	const vec3 texCoordsIndices = vec3(sampleCoords, 0) + psfIndex;
    return sampleBasePsfTexture(texCoordsIndices);
}

// Samples the psf for a given fragment
vec3 interpolatePsf(const vec2 fragmentPosition, const FragmentData fragmentData)
{
    return samplePsfTexture(
        fragmentPosition - fragmentData.vScreenPosition, 
		fragmentData.uiBlurRadius, 
        fragmentData.vPsfIndex);
}
*/