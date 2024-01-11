#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/PsfStack/common.glsl>

// Kernel size
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

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
FLOAT_ARRAY_BUFFER(std430, restrict writeonly, UNIFORM_BUFFER_GENERIC_3, sVnmInner_);
#define sVnmInner sVnmInner_.sData

////////////////////////////////////////////////////////////////////////////////
// Uniforms
layout (location = 0) uniform int iNumCoeffs;
layout (location = 1) uniform int iMaxOrder;
layout (location = 2) uniform int iMaxNumSamples;
layout (location = 3) uniform int iMaxNumTermsPerOrder;
layout (location = 4) uniform int iWklOffset;
layout (location = 5) uniform int iCoeffId;
layout (location = 6) uniform int iN;
layout (location = 7) uniform int iM;
layout (location = 8) uniform int iP;
layout (location = 9) uniform int iQ;

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
void main()
{
    // Extract the coefficient id, radius id and k
    const int radiusId = int(gl_GlobalInvocationID.x);
    const int k = int(gl_GlobalInvocationID.y);
    
    // Skip invalid invocations
    if (radiusId >= iMaxNumSamples || k >= iMaxOrder) return;

    // Compute vnm = sum(wkl * Jn)
    float result = 0;
    for (int l = max(0, max(k - iQ, iP - k)), li = 0; l <= k + iP; ++l, ++li)
        result += wkl(k, li) * cylindricalBessel(l, radiusId);

    // Write out the result
    const uint resultId = iCoeffId * iMaxOrder * iMaxNumSamples + k * iMaxNumSamples + radiusId;
    sVnmInner[resultId] = result;
}