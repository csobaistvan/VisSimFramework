#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/PsfStack/common.glsl>

// Kernel size
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

////////////////////////////////////////////////////////////////////////////////
// Cylindrical Bessel values
FLOAT_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_1, sVnmCylindricalBessels_);
#define sVnmCylindricalBessels sVnmCylindricalBessels_.sData

////////////////////////////////////////////////////////////////////////////////
// W_kl terms
FLOAT_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_2, sVnmWkls_);
#define sVnmWkls sVnmWkls_.sData

////////////////////////////////////////////////////////////////////////////////
// Sum of W_kl * jl terms
FLOAT_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_3, sVnmInner_);
#define sVnmInner sVnmInner_.sData

////////////////////////////////////////////////////////////////////////////////
// Spherical Bessel values
//   - stored as exp(1/2f) * (2k + 1) * jk * i^k
VEC2_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_4, sVnmSphericalBessels_);
#define sVnmSphericalBessels sVnmSphericalBessels_.sData

////////////////////////////////////////////////////////////////////////////////
// Vnm results texture
layout(binding = 0, VNM_TEXTURE_FORMAT) uniform restrict writeonly image2D sVnmResult;

////////////////////////////////////////////////////////////////////////////////
// Uniforms
layout (location =  0) uniform int iNumPsfs;
layout (location =  1) uniform int iNumCoeffs;
layout (location =  2) uniform int iNumOrders;
layout (location =  3) uniform int iNumSamples;
layout (location =  4) uniform int iMaxOrder;
layout (location =  5) uniform int iMaxNumSamples;
layout (location =  6) uniform int iMaxNumTermsPerOrder;
layout (location =  7) uniform int iWklOffset;
layout (location =  8) uniform int iPsfId;
layout (location =  9) uniform int iCoeffId;
layout (location = 10) uniform int iN;
layout (location = 11) uniform int iM;
layout (location = 12) uniform int iP;
layout (location = 13) uniform int iQ;

////////////////////////////////////////////////////////////////////////////////
float cylindricalBessel(const int l, const int r)
{
    const uint idx = (iM + 2 * l + 1) * iMaxNumSamples + r;
    return sVnmCylindricalBessels[idx];
}

////////////////////////////////////////////////////////////////////////////////
float wkl(const int k, const int li)
{
    const uint idx = iWklOffset + k * iMaxNumTermsPerOrder + li;
    return float(sVnmWkls[idx]);
}

////////////////////////////////////////////////////////////////////////////////
vec2 sphericalBessel(const int k)
{
    //const uint idx = iPsfId * iMaxOrder + k;
    const uint idx = k * iNumPsfs + iPsfId;
    return vec2(sVnmSphericalBessels[idx]);
}

////////////////////////////////////////////////////////////////////////////////
float vnmInnerSum(const int k, const int r)
{    
    const uint idx = iCoeffId * iMaxOrder * iMaxNumSamples + k * iMaxNumSamples + r;
    return float(sVnmInner[idx]);
}

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Index of the radius
    const int radiusId = int(gl_GlobalInvocationID.x);

    // Skip invalid pixels
    if (radiusId >= iNumSamples) return;

    // Compute vnm = sum(wkl * jk * Jn), using the cached sum(wkl * jk) terms
    vec2 vnm = vec2(0.0);
    for (int k = 0; k < iNumOrders; ++k)
    {
    #if USE_CACHED_INNER_TERM == 0
        float cylindricalSum = 0;
        for (int l = max(0, max(k - iQ, iP - k)), li = 0; l <= k + iP; ++l, ++li)
            cylindricalSum += wkl(k, li) * cylindricalBessel(l, radiusId);
        vnm += sphericalBessel(k) * cylindricalSum;
    #elif USE_CACHED_INNER_TERM == 1
        vnm += sphericalBessel(k) * vnmInnerSum(k, radiusId);
    #endif
    }
    imageStore(sVnmResult, ivec2(radiusId, iCoeffId), vec4(vnm, 0, 0));
}