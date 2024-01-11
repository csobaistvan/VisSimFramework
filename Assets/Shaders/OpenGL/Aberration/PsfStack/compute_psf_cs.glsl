#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/PsfStack/common.glsl>

// Kernel size
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

////////////////////////////////////////////////////////////////////////////////
// PSF properties
struct PsfProperties
{    
    int iNumCoeffs;
    int iNumTotalCoeffs;
    int iNumSamples;
    float fPsfPixelSize;
    float fPsfHalfExtent;
    vec2 vVnmUvOffset;
    vec2 vVnmUvScale;
};
TYPED_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_5, sPsfProperties_, PsfProperties);
#define sPsfProperties sPsfProperties_.sData[iPsfId]

////////////////////////////////////////////////////////////////////////////////
// PSF beta values
struct PsfBeta
{
    int iCoeffId;
    int iN;
    int iM[2];
    vec2 vBeta[2];
};
TYPED_ARRAY_BUFFER(std430, restrict readonly, UNIFORM_BUFFER_GENERIC_6, sPsfBetas_, PsfBeta);
#define sPsfBeta(coeffId) sPsfBetas_.sData[iPsfId * sPsfProperties.iNumTotalCoeffs + coeffId]

////////////////////////////////////////////////////////////////////////////////
// PSF results buffer
FLOAT_ARRAY_BUFFER(std430, restrict writeonly, UNIFORM_BUFFER_GENERIC_7, sPsfResult_);
#define sPsfResult sPsfResult_.sData

////////////////////////////////////////////////////////////////////////////////
// Vnm sampler texture
layout(binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sVnmTexture;

////////////////////////////////////////////////////////////////////////////////
// Uniforms
layout (location = 0) uniform int iPsfId;

////////////////////////////////////////////////////////////////////////////////
vec2 computeVnm(const int coeffId, const float radius)
{
    const vec2 sampleCoords = sPsfProperties.vVnmUvOffset + vec2(radius, coeffId) * sPsfProperties.vVnmUvScale;
    return texture(sVnmTexture, sampleCoords).rg;
}

////////////////////////////////////////////////////////////////////////////////
const vec2 cart2pol(const vec2 cartCoords)
{
    return vec2(length(cartCoords), atan(cartCoords.y, cartCoords.x));
}

////////////////////////////////////////////////////////////////////////////////
const vec2 psfCartCoords(const vec2 psfCoords)
{
    return -sPsfProperties.fPsfHalfExtent + vec2(psfCoords) * sPsfProperties.fPsfPixelSize;
}

////////////////////////////////////////////////////////////////////////////////
const vec2 psfPolarCoords(const vec2 psfCoords)
{
    return cart2pol(psfCartCoords(psfCoords));
}

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Cartesian and polar coordinates of the current sample
    const ivec2 psfCoords = ivec2(gl_GlobalInvocationID.xy);
    const vec2 polarCoords = psfPolarCoords(psfCoords);

    // Skip threads that fall out of the actual PSF area
    if (any(greaterThanEqual(psfCoords, ivec2(sPsfProperties.iNumSamples)))) return;

    // Sum the coefficient contributions
    vec2 U = vec2(0.0);
    for (int coeffId = 0; coeffId < sPsfProperties.iNumCoeffs; ++coeffId)
    {
        const PsfBeta beta = sPsfBeta(coeffId);
        const vec2 vnm = computeVnm(beta.iCoeffId, polarCoords[0]);
        for (int k = 0; k < 2; ++k) // Handle both -m and m
        {
            const vec2 Z = cmpxexp(vec2(0, polarCoords[1] * beta.iM[k]));
            U += cmpxmul(beta.vBeta[k], cmpxmul(vnm, Z));
        }
    }
    
    // Filter out the ring outside the main PSF
    if (polarCoords.r > sPsfProperties.fPsfHalfExtent + 0.01) U = vec2(0);

	// Write out the result
    const int idx = psfCoords.y * sPsfProperties.iNumSamples + psfCoords.x;
    sPsfResult[idx] = cmpxabs2(U);
}