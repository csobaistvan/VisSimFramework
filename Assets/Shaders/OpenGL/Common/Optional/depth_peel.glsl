///////////////////////////////////////////////////////////////////////////////
// Extracts the depth value of the front layer
vec2 extractPrevLayerDepth(const vec3 position, const sampler2DArray depthBuffer, const int layer)
{
    // Compute the previous view-space coordinates
    const vec4 positionVS = sCameraData.mView * vec4(position, 1.0);

    // Compute the previous screen-space coordinates
    const vec4 positionCS = sCameraData.mProjection * positionVS;

    // UV to read the depth buffer at
    const vec2 uv = ndcToScreen(positionCS.xy / positionCS.w);
    
    // Read back the depth value of the upper layer
    const float frontDepth = textureDR(depthBuffer, uv, layer).r;

    // Get the Z value using the current projection matrix
    const float frontZ = camSpaceDepth(frontDepth, sCameraData.mProjection);

    // The current Z value
    const float currentZ = -meters(positionVS.z);

    // Return the front and current depth values
    return vec2(currentZ, frontZ);
}

///////////////////////////////////////////////////////////////////////////////
// Performs reverse reprojection on the parameter vertex and depth buffer. 
// Returns the camera space depth from the previous frame at the reprojected position
// and the reprojected depth of the previous position.
vec2 reprojectDepth(const vec3 prevPosition, const sampler2DArray prevDepthBuffer, const int layer)
{
    // Compute the previous view-space coordinates
    const vec4 prevPosVS = sCameraData.mPrevView * vec4(prevPosition, 1.0);

    // Compute the previous screen-space coordinates
    const vec4 prevPosCS = sCameraData.mPrevProjection * prevPosVS;

    // Compute the UV coordinates from the clip space position
    const vec2 prevUV = ndcToScreen(prevPosCS.xy / prevPosCS.w);

    // Read back the previous depth value of the upper layer
    const float oldDepth = textureDR(prevDepthBuffer, prevUV, layer).r;
    
    // Reproject the previous Z value using the current projection matrix
    const float frontZ = camSpaceDepth(oldDepth, sCameraData.mProjection);

    // The current Z value
    const float currentZ = -meters(prevPosVS.z);

    // Return the previous and current depth values
    return vec2(currentZ, frontZ);
}

// Returns the depth value for the current and the top layer
vec2 extractLayerDepths(const sampler2DArray prevDepthBuffer, const vec3 position, const vec3 prevPosition, const int layer)
{
    // Reverse reprojection
    return reprojectDepth(prevPosition, prevDepthBuffer, layer);
}

// Performs depth peeling using minimum separation. Returns true if the pixel survives the peeling process
bool depthPeelMinimumSeparation(const float currentDepth, const float frontDepth)
{
    // Keep if the current fragment is further from the camera than the previous layer
    return (currentDepth > frontDepth + sRenderData.fLayerDepthGap);
}

// Performs depth peeling using umbra thresholding. Returns true if the pixel survives the peeling process
bool depthPeelUmbraThresholding(const float currentDepth, const float frontDepth)
{
    // Compute the umbra threshold
    const float umbra = computeUmbra(frontDepth);

    // Compute the umbra threshold
    const float scaledUmbra = clamp(umbra * sRenderData.fUmbraScaling, sRenderData.fUmbraMin, sRenderData.fUmbraMax);

    // Keep if the current fragment is further from the camera than the previous layer
    return (umbra >= 0.0 && currentDepth > frontDepth + scaledUmbra);
}

// Performs depth peeling by using the selected depth peeling algorithm
bool depthPeel(const float currentDepth, const float frontDepth, const int layer)
{
    // Minimum separation
    if (sRenderData.uiDepthPeelAlgorithm == DepthPeelAlgorithm_MinimumSeparation)
        return depthPeelMinimumSeparation(currentDepth, frontDepth);
    
    // Umbra thresholding
    else if (sRenderData.uiDepthPeelAlgorithm == DepthPeelAlgorithm_UmbraThresholding)
        return depthPeelUmbraThresholding(currentDepth, frontDepth);

    // Default to always keeping all values
    return true;
}

// Performs depth peeling by using the selected depth peeling algorithm
bool depthPeel(const sampler2DArray prevDepthBuffer, const vec3 position, const vec3 prevPosition, const int layer)
{
    // Compute the actual and reprojected depth values
    const vec2 depths = extractLayerDepths(prevDepthBuffer, position, prevPosition, layer);

    // Evaluate the depth fn.
    return depthPeel(depths[0], depths[1], layer);
}