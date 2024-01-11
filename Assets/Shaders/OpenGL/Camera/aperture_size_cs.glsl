#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

// Kernel size
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// Luminance texture
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sLuminance;

////////////////////////////////////////////////////////////////////////////////
// Computes the physical aperture size based on the mean luminance
float apertureSizePhysical()
{
    // Extract the avg. luminance
    const float avgLuminance = unpackLuminance(textureLodDR(sLuminance, vec2(0, 0), 99).r);
    
    // Conver the avg luminance to its physical value (cd/m2)
    const float fieldLuminance = avgLuminance * sRenderData.fNitsPerUnit;

    // Compute the mean pupil diameter
    const float meanDiameter = 4.9 - 3.0 * tanh(0.4 * log(fieldLuminance) - 0.00114);

    // Noise factor - three-octave value noise
    const float noise = valueNoise((sRenderData.fScaledGlobalTime) / meanDiameter, 
        sCameraData.iApertureNoiseOctaves, sCameraData.fApertureNoisePersistance) * sCameraData.fApertureNoiseAmplitude;

    // Compute and return the current pupil diameter
    return meanDiameter + noise * (sCameraData.fApertureMax / meanDiameter) * sqrt(1.0 - (meanDiameter / sCameraData.fApertureMax));
}

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Previous value
    const float prevAperture = sCameraData.fAperture;

    // Compute the physical aperture size
    if (sCameraData.uiApertureMethod == ApertureMethod_Physical)
    {
        // Compute the new aperture
        const float newAperture = clamp(apertureSizePhysical(), sCameraData.fApertureMin, sCameraData.fApertureMax);

        // Interpolate it
        const float weight = (1.0 - exp(-sRenderData.fScaledDeltaTime * sCameraData.fApertureAdaptationRate));
        sCameraData.fAperturePrev = prevAperture;
        sCameraData.fAperture = max(0.001, prevAperture + (newAperture - prevAperture) * weight);
    }

    // Clamp it
    sCameraData.fAperture = clamp(sCameraData.fAperture, sCameraData.fApertureMin, sCameraData.fApertureMax);
}