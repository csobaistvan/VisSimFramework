
////////////////////////////////////////////////////////////////////////////////
vec4 normalWeightedSample(const vec3 weight, const vec4 sampleX, const vec4 sampleY, const vec4 sampleZ)
{
    return
        weight.x * sampleX + 
        weight.y * sampleY +
        weight.z * sampleZ;
}

////////////////////////////////////////////////////////////////////////////////
vec4 normalWeightedSample(const vec3 weight, const vec4 sampleXNeg, const vec4 sampleXPos, 
    const vec4 sampleYNeg, const vec4 sampleYPos, const vec4 sampleZNeg, const vec4 sampleZPos)
{
    return
        weight.x * max(sampleXNeg, sampleXPos) + 
        weight.y * max(sampleYNeg, sampleYPos) +
        weight.z * max(sampleZNeg, sampleZPos);
}

////////////////////////////////////////////////////////////////////////////////
vec3 voxelToWorld(const vec3 voxelPosition)
{
	return voxelPosition * sRenderData.fVoxelSize + sRenderData.vWorldMin;
}

////////////////////////////////////////////////////////////////////////////////
vec3 worldToVoxel(const vec3 worldPosition)
{
    return (worldPosition - sRenderData.vWorldMin) / sRenderData.fVoxelSize;
}

////////////////////////////////////////////////////////////////////////////////
vec3 normalizedVoxelToWorld(const vec3 voxelPosition)
{
	return voxelPosition * sRenderData.fVoxelExtents + sRenderData.vWorldMin;
}

////////////////////////////////////////////////////////////////////////////////
vec3 worldToNormalizedVoxel(const vec3 worldPosition)
{
    return (worldPosition - sRenderData.vWorldMin) / sRenderData.fVoxelExtents;
}