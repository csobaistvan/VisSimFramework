#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>

// Tone map uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform ComplexBlurData
{
    vec2 vResolution;
    vec2 vDilatedResolution;
    vec2 vUvScale;
    vec2 vHorizontalDir;
    vec2 vVerticalDir;
    uint uiOutputMode;
    uint uiDilationSteps;
    uint uiDilationSearchRadius;
    uint uiDilatedTileSize;
    uint uiKernelTaps;
    float fMaxBlur;
    float fSampleSize;
    float fEllipseRotation;
    float fEllipseRatio;
    float fEllipseContraction;
	uint uiNumObjectDistances;
	float fObjectDistancesMin;
	float fObjectDistancesMax;
	float fObjectDistancesStep;
    vec4 vBlurSizes[MAX_OBJECT_DISTANCES];
} sComplexBlurData;

layout (std140, binding = UNIFORM_BUFFER_GENERIC_2) buffer ComplexBlurWeights
{
    vec4 vWeights[MAX_COMPONENTS];
    vec4 vBracketsHorizontal[MAX_COMPONENTS];
    vec4 vBracketsVertical[MAX_COMPONENTS];
    vec4 vKernelHorizontal[MAX_COMPONENTS * MAX_KERNEL_SIZE];
    vec4 vKernelVertical[MAX_COMPONENTS * MAX_KERNEL_SIZE];
} sComplexBlurWeights;

// Various texture maps
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sDownscaledBuffer;
layout (binding = TEXTURE_POST_PROCESS_2) uniform sampler2D sDilatedBuffer;
layout (binding = TEXTURE_POST_PROCESS_3) uniform sampler2D sResultsBuffer;
layout (binding = TEXTURE_POST_PROCESS_4) uniform sampler2DArray sBlurBuffersNear;
layout (binding = TEXTURE_POST_PROCESS_5) uniform sampler2DArray sBlurBuffersFar;

// Computes the maximum blur radius
float computeMaxBlurRadius()
{
    return sComplexBlurData.fMaxBlur * sComplexBlurData.fEllipseRatio;
}

// Computes the blur radius of a given pixel
float computeBlurRadius(const vec2 fragCoord, const float z)
{
    // Convert depth to dioptres
    const float depth = screenToSphericalCoordinates(ivec2(fragCoord), ivec2(sComplexBlurData.vResolution), z).z;
    const float dioptres = 1.0 / depth;

    // Calculate the PSF index corresponding to the depth
    const float depthIdUc = (dioptres - sComplexBlurData.fObjectDistancesMin) / sComplexBlurData.fObjectDistancesStep;
    const float depthId = clamp(depthIdUc, 0.0, float(sComplexBlurData.uiNumObjectDistances - 1));

    // Interpolate the blur sizes
    return sComplexBlurData.fEllipseRatio * lerp
    (
        sComplexBlurData.vBlurSizes[uint(floor(depthId))].x,
        sComplexBlurData.vBlurSizes[uint(ceil(depthId))].x,
        fract(depthId)
    );
}

// Extract the horizontal kernel for component #c at sample #s
vec2 horizontalKernel(const uint c, const uint s)
{
    return sComplexBlurWeights.vKernelHorizontal[c * KERNEL_SIZE_HORIZONTAL + s].xy;
    //return sComplexBlurWeights.vKernelHorizontal[c * KERNEL_SIZE_HORIZONTAL + s].zw;
}

// Extract the vertical kernel for component #c at sample #s
vec2 verticalKernel(const uint c, const uint s)
{
    return sComplexBlurWeights.vKernelVertical[c * KERNEL_SIZE_VERTICAL + s].xy;
}

// Removes the horizontal bracketing
vec2 removeHorizontalBracketing(const uint c, const vec2 sampleVal, const float sampleSum)
{
    return sampleVal;
    //return sampleVal * sComplexBlurWeights.vBracketsHorizontal[c].zw + sampleSum * sComplexBlurWeights.vBracketsHorizontal[c].xy;
}

// Removes the vertical bracketing
vec2 removeVerticalBracketing(const uint c, const vec2 sampleVal, const float sampleSum)
{
    return sampleVal * sComplexBlurWeights.vBracketsVertical[c].zw + sampleSum * sComplexBlurWeights.vBracketsVertical[c].xy;
}

// returns the dilated blur radius
vec2 dilatedBlurRadius(const vec2 fc)
{
    const ivec2 texCoords = ivec2(fc.xy) / int(sComplexBlurData.uiDilatedTileSize);
    return texelFetch(sDilatedBuffer, texCoords, 0).xy;
}

// Calculates the near blur radius
float nearBlurRadius(const vec2 rd_c)
{
    return rd_c.y;
    //return -rd_c.x;
    //return max(max(abs(rd_c.x), abs(rd_c.y)), 1.0);
}

// Calculates the far blur radius
float farBlurRadius(const vec2 rd_c)
{
    return rd_c.y;
    //return -rd_c.x;
    //return max(max(abs(rd_c.x), abs(rd_c.y)), 1.0);
}

// Convers the input radius to a gather radius
float getGatherRadius(const float r)
{
    return max(abs(r), 1);
}

// whether the sample is in the near field or not
bool inNearField(const float radiusPixels)
{
    return radiusPixels > 0.01;
}

// compute the nearfieldness of a sample using its radius
float nearFieldness(const float r)
{
    return saturate(r * 4.0);
}

// compute the nearfieldness of a sample using its radius
float farFieldness(const float r)
{
    return saturate(-r * 4.0);
}

// calculates the distance of the sample from the center
float calcSampleDist(const float t, const float r)
{
    return t * r / float(sComplexBlurData.uiKernelTaps);
}

// calculates the weight for the far plane
float nearBlurWeight(const float sampleDist, const float r_c, const float rd_c, const float r_s, const float rd_s)
{
    const float OFFSET = 0.5;
    //return le(abs(sampleDist), abs(rd_s) + OFFSET) * ge(r_s, r_c - 0.15);
    return le(abs(sampleDist), abs(rd_s) + OFFSET);
    //return le(abs(sampleDist), abs(r_s) + OFFSET);
}

// calculates the weight for the far plane
float farBlurWeight(const float sampleDist, const float r_c, const float rd_c, const float r_s, const float rd_s)
{
    const float OFFSET = 0.5;
    //return le(abs(sampleDist), abs(r_s) + OFFSET) * ge(r_c, r_s - 0.15);
    return le(abs(sampleDist), abs(rd_s) + OFFSET);
    //return le(abs(sampleDist), abs(r_s) + OFFSET);
}