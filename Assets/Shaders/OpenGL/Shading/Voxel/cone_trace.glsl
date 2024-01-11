#include <Shaders/OpenGL/Shading/Voxel/sample_grid.glsl>

////////////////////////////////////////////////////////////////////////////////
struct ConeTraceParameters
{
    vec3 position;
    vec3 normal;
    vec3 direction;
    float aperture;
    float mipOffset;
    float traceStartOffset;
    float traceNormalOffset;
    float maxTraceDistance;
    bool anisotropic;
    float occlusionDecay;
    float radianceDecay;
    bool firstHitOnly;
};

////////////////////////////////////////////////////////////////////////////////
struct ConeTraceResult
{
    vec3 color;
    vec3 position;
    float dist;
    float occlusion;
    bool anyHit;
};

////////////////////////////////////////////////////////////////////////////////
float coneTraceWeightDistance(const float dist, const float decayRate)
{
    return (1.0 / (1.0 + dist * decayRate));
}

////////////////////////////////////////////////////////////////////////////////
ConeTraceResult traceCone(const ConeTraceParameters traceParams)
{
    // Result of the trace
    ConeTraceResult result;
    result.anyHit = false;
    result.color = vec3(0.0);
    result.occlusion = float(0.0);
    result.position = vec3(0.0);

    // Determine which faces are visible
    const uvec3 visibleFace = uvec3
    (
        (traceParams.direction.x < 0.0) ? 0 : 1,
        (traceParams.direction.y < 0.0) ? 2 : 3,
        (traceParams.direction.z < 0.0) ? 4 : 5
    );

    // Weight per axis for anisotropic sampling
    const vec3 weight = traceParams.direction * traceParams.direction;

    // Falloff factors
    const float radianceFalloff =  traceParams.radianceDecay;
    const float occlusionFalloff = 0.5 * traceParams.occlusionDecay;

    // Initial voxel size, with the mip offset taken into account
    const float startVoxelSize = sRenderData.fVoxelSize * exp2(traceParams.mipOffset);

    // Init the cone parameters
    float dst = (traceParams.traceStartOffset) * startVoxelSize;
    float alpha = 0.0;

    // Trace starting position
    const vec3 startPos = traceParams.position + (traceParams.traceNormalOffset) * startVoxelSize * traceParams.normal;

    // accumulate the samples
    while (alpha < 1.0 && dst <= traceParams.maxTraceDistance)
    {
        // Compute the current diameter and sample position
        const float diameter = 2.0 * dst * traceParams.aperture;
        const vec3 samplePos = worldToNormalizedVoxel(startPos + dst * traceParams.direction);
        const float mipLevel = max(log2(diameter / sRenderData.fVoxelSize) + traceParams.mipOffset, traceParams.mipOffset);

        // Sample the voxel grid        
        const vec4 sampleRadiance = traceParams.anisotropic ? 
            sampleVoxelRadianceAnisotropic(samplePos, mipLevel, visibleFace, weight) : 
            sampleVoxelRadianceIsotropic(samplePos, mipLevel);
        
        // Unpack the sample
        const vec3 radiance = sampleRadiance.rgb;
        const float occlusion = sampleRadiance.w;

        // Accumulate sampling
        result.color += (1.0 - alpha) * radiance * coneTraceWeightDistance(dst, radianceFalloff);
        result.occlusion += (1.0 - result.occlusion) * occlusion * coneTraceWeightDistance(diameter, occlusionFalloff);
        alpha += (1.0 - alpha) * occlusion;

        // Register the first hit
        if (occlusion > 0.0 && !result.anyHit) 
        {
            result.anyHit = true;
            result.position = normalizedVoxelToWorld(samplePos);
            result.dist = dst;
        }

        // Move the cone forward
        dst += max(diameter, sRenderData.fVoxelSize);
    }

    // Return the result
    return result;
}