
////////////////////////////////////////////////////////////////////////////////
// Intermediate storage for PSF interpolation purposes
////////////////////////////////////////////////////////////////////////////////

// PSF interpolation intermediate data buffer
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_10, sPsfInterpolationBuffer_, uint _);
#define sPsfInterpolationBuffer sPsfInterpolationBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Cache PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for single interpolated PSF
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_11, sCachePsfParamsBuffer_, 
	float fBlurRadius);
#define sCachePsfParamsBuffer sCachePsfParamsBuffer_.sData

////////////////////////////////////////////////////////////////////////////////
// Interpolated PSF buffers
////////////////////////////////////////////////////////////////////////////////

// Parameters for single interpolated PSF
STRUCT_ARRAY_BUFFER(std430, restrict, UNIFORM_BUFFER_GENERIC_12, sInterpolatedPsfParamBuffer_, 
	float fBlurRadius);
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

// Computes the blur radii for the parameter PSF indices
vec3 blurRadii(const vec3 sphericalCoords, const uint channelId)
{
    // calculate the corresponding PSF index
	const vec3 psfIds = objectDistanceIndices(sphericalCoords.z);

    // return the clamped blur radius
    return makeIndices(lerpPsfRadius(psfIds, channelId));
}

////////////////////////////////////////////////////////////////////////////////
// PSF weights functionality
////////////////////////////////////////////////////////////////////////////////

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

// Interpolates the PSF weights for all channels
vec4 lerpPsfWeightAllChannels(const uint d, const vec3 a, const vec3 f, const uint r, const uvec2 sc)
{
    vec4 result = vec4(0, 0, 0, 1);
    for (int c = 0; c < sTiledSplatBlurData.uiNumChannels; ++c)
        result[c] = lerpPsfWeight(d, a, f, c, r, sc);
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
    // Generate the PSF interpolation dispatch command
    const uint numDefocuses = sTiledSplatBlurData.uiNumObjectDistances;
    const uint maxPsfRadius = sTiledSplatBlurData.uiMaxBlurRadiusCurrent;
    const uint maxPsfDiameter = sTiledSplatBlurData.uiMaxBlurRadiusCurrent * 2 + 1;
    const uvec3 finalTextureSize = uvec3(maxPsfDiameter, maxPsfDiameter, numDefocuses * (maxPsfRadius + 1));
    const uvec3 interpolationGroupSize = uvec3(INTERPOLATION_GROUP_SIZE, INTERPOLATION_GROUP_SIZE, 1);
    sTileDispatchBuffer[0].vNumGroups = ROUNDED_DIV(finalTextureSize, interpolationGroupSize);	
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture interpolation
///////////////////////////////////////////////////////////////////////////////

// Interpolates one pixel of the the PSF texture
void interpolatePsfTextureSample()
{
    // Current sample coordinates
	const ivec2 sampleCoords = ivec2(gl_GlobalInvocationID.xy);
    const uint psfId = gl_WorkGroupID.z % sTiledSplatBlurData.uiNumObjectDistances;
    const uint radius = gl_WorkGroupID.z / sTiledSplatBlurData.uiNumObjectDistances;
    const uint textureLayerId = radius * sTiledSplatBlurData.uiNumObjectDistances + psfId;

    // Skip unnecessary invocations
    if (radius > sTiledSplatBlurData.uiMaxBlurRadiusCurrent) return; // Unnecessary blur radii
    if (sampleCoords.x >= (radius * 2 + 1) || sampleCoords.y >= (radius * 2 + 1)) return; // Outside area

    // Aperture and focus indices
    const vec3 apertureIds = apertureIndices(sCameraData.fAperture);
    const vec3 focusIds = focusDistanceIndices(sCameraData.fFocusDistance);
    
    // Compute the lerped weights for each channel
    const vec4 textureValue = lerpPsfWeightAllChannels(psfId, apertureIds, focusIds, radius, sampleCoords);
        
    // Store the final result in the PSF texture
    imageStore(sPsfImage, ivec3(sampleCoords.x, sampleCoords.y, textureLayerId), textureValue);
}

////////////////////////////////////////////////////////////////////////////////
// PSF texture sampling
////////////////////////////////////////////////////////////////////////////////

// Init the PSF data
void initPsfData()
{

}

// Depth to PSF index conversion
vec3 sphericalCoordsToPsfIndex(const vec3 sphericalCoords)
{
    return sphericalCoords;
}

////////////////////////////////////////////////////////////////////////////////
// Samples the parameter PSF using the texture data and trilinear (XY and PSF) sampling
float samplePsfTextureRadiusBased(const vec3 psfIndices, const uint channelId, const int sampleRadius, const vec2 sampleCoords)
{
	// Compute the texture sample coordinates
	const vec3 texCoordsIndices = vec3
	(
		sampleCoords + sampleRadius,
		psfIndices[2] + sampleRadius * sTiledSplatBlurData.uiNumObjectDistances
	);

	// Sample the psf texture and return the result
	return samplePsfTexture(texCoordsIndices)[channelId];
}

// Samples the parameter PSF at the parameter channel
float samplePsfRadiusBasedChannel(const vec2 sampleCoords, const vec3 psfIndices, const uint channelId, const float paddedSampleRadius)
{
	return lerp
	(
		samplePsfTextureRadiusBased(psfIndices, channelId, int(floor(paddedSampleRadius)), sampleCoords),
		samplePsfTextureRadiusBased(psfIndices, channelId, int(ceil(paddedSampleRadius)), sampleCoords),
		fract(paddedSampleRadius)
	);
}

// Samples the PSF using the parameter sample coords and PSF indices
vec3 samplePsf(const vec2 sampleCoords, const vec3 sphericalCoords)
{
    // Compute the corresponding PF indices
	const vec3 psfIndices = objectDistanceIndices(sphericalCoords.z);

	// Sample the PSF texture
	vec3 weight = vec3(0.0);
	
	// Evaluate the PSF for the requested number of channels
	for (uint channelId = 0; channelId < sTiledSplatBlurData.uiRenderChannels; ++channelId)
	{
		const vec3 blurRadii = blurRadii(sphericalCoords, channelId);
        const float blurRadius = blurRadii[2];

		weight[channelId] = !overlapsFragment(sampleCoords, ceil(blurRadius)) ? 0.0 :
			samplePsfRadiusBasedChannel(sampleCoords, psfIndices, channelId, blurRadius);
	}
	
	// Copy over the last channel's PSF for the rest of the channels
    for (uint i = sTiledSplatBlurData.uiRenderChannels; i < 3; ++i)
        weight[i] = weight[sTiledSplatBlurData.uiRenderChannels - 1];

	// Return the sample PSF weights
	return weight;
}

////////////////////////////////////////////////////////////////////////////////
// Samples the psf for a given fragment
vec3 interpolatePsf(const vec2 fragmentPosition, const FragmentData fragmentData)
{
	return samplePsf(fragmentPosition - fragmentData.vScreenPosition, fragmentData.vPsfIndex);
}