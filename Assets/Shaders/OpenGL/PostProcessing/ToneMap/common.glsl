#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>

// Tone map uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform FilmicTonemapData
{
    uint uiExposureMethod;
    float fFixedExposure;
	float fExposureBias;
    float fLuminanceThresholdShadows;
    float fLuminanceThresholdHighlights;
    float fExposureBiasShadows;
    float fExposureBiasHighlights;
    uint uiKeyMethod;
    float fFixedKey;
    uint uiAdaptationMethod;
    float fAdaptationRate;
    float fMinAvgLuminance;
    float fNumMipLevels;
    float fLocalMipLevel;
    float fMaxLocalContribution;
    uint uiOperator;
	float fLinearWhite;
    float bHasColorTable;
    float fOperatorParams[8];
} sTonemapData;

// Common textures
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sLuminance;
layout (binding = TEXTURE_POST_PROCESS_2) uniform sampler3D sColorTable;

////////////////////////////////////////////////////////////////////////////////
// Adapted luminance computation
float computeAdaptedLuminance(const float currentLuminance, const float lastLuminance)
{
    // Compute the adapted luminance
    float adaptedLuminance;
    
    // Lerp adaptation
    if (sTonemapData.uiAdaptationMethod == AdaptationMethod_Lerp)
    {
        adaptedLuminance = max(0.001, lerp(lastLuminance, currentLuminance, sTonemapData.fAdaptationRate));
    }

    // Exponential adaptation
    else if (sTonemapData.uiAdaptationMethod == AdaptationMethod_Exponential)
    {
        const float weight = (1.0 - exp(-sRenderData.fScaledDeltaTime * sTonemapData.fAdaptationRate));
        adaptedLuminance = max(0.001, lastLuminance + (currentLuminance - lastLuminance) * weight);
    } 

    // Return the log of the computed luminance
    return packLuminance(adaptedLuminance);
}

////////////////////////////////////////////////////////////////////////////////
float extractLuminance(const ivec2 fragCoords)
{
    return unpackLuminance(texelFetch(sLuminance, fragCoords, 0).x);
}

////////////////////////////////////////////////////////////////////////////////
vec2 extractLuminance(const vec2 uv, const float mipLevel)
{
    return max(unpackLuminance(textureLodDR(sLuminance, uv, mipLevel).rg), sTonemapData.fMinAvgLuminance);
}

////////////////////////////////////////////////////////////////////////////////
// Simple clamping fn.
vec3 toneMapClamp(const vec3 color)
{
    return saturate(color);
}

////////////////////////////////////////////////////////////////////////////////
// Reinhard tone map function
vec3 toneMapReinhard(const vec3 color)
{
	// Compute the tonemapped luminance
    const float pixelLuminance = max(computeLuminance(color), 0.0001);
	const float toneMappedLuminance = pixelLuminance * (1.0f + pixelLuminance / (sTonemapData.fLinearWhite * sTonemapData.fLinearWhite)) / (1.0f + pixelLuminance);
    
	// Change the luminance
    return color * (toneMappedLuminance / pixelLuminance);
}

////////////////////////////////////////////////////////////////////////////////
// Uncharted 2 filmic tonemap function
vec3 toneMapFilmicFn(const vec3 color)
{
    const float A = sTonemapData.fOperatorParams[0];
    const float B = sTonemapData.fOperatorParams[1];
    const float C = sTonemapData.fOperatorParams[2];
    const float D = sTonemapData.fOperatorParams[3];
    const float E = sTonemapData.fOperatorParams[4];
    const float F = sTonemapData.fOperatorParams[5];
    
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

// Uncharted 2 filmic tonemap function
vec3 toneMapFilmic(const vec3 color)
{
    return toneMapFilmicFn(color) / toneMapFilmicFn(vec3(sTonemapData.fLinearWhite));
}

////////////////////////////////////////////////////////////////////////////////
// Aces tone map function
//
// SOURCE: https://github.com/dmnsgn/glsl-tone-map/blob/master/aces.glsl
vec3 toneMapAces(const vec3 color)
{
    const float a = sTonemapData.fOperatorParams[0];
    const float b = sTonemapData.fOperatorParams[1];
    const float c = sTonemapData.fOperatorParams[2];
    const float d = sTonemapData.fOperatorParams[3];
    const float e = sTonemapData.fOperatorParams[4];
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

////////////////////////////////////////////////////////////////////////////////
// Lottes
//
// SOURCE: https://github.com/dmnsgn/glsl-tone-map/blob/master/lottes.glsl
vec3 toneMapLottes(const vec3 color)
{
    const vec3 a = vec3(sTonemapData.fOperatorParams[0]);
    const vec3 d = vec3(sTonemapData.fOperatorParams[1]);
    const vec3 midIn = vec3(sTonemapData.fOperatorParams[2]);
    const vec3 midOut = vec3(sTonemapData.fOperatorParams[3]);
    const vec3 hdrMax = vec3(sTonemapData.fLinearWhite);

    const vec3 b =
        (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const vec3 c =
        (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(color, a) / (pow(color, a * d) * b + c);
}

////////////////////////////////////////////////////////////////////////////////
// Uchimura tone map function
//
// SOURCE: https://github.com/dmnsgn/glsl-tone-map/blob/master/uchimura.glsl
vec3 uchimura(vec3 x, float P, float a, float m, float l, float c, float b)
{
    const float l0 = ((P - m) * l) / a;
    const float L0 = m - m / a;
    const float L1 = m + (1.0 - m) / a;
    const float S0 = m + l0;
    const float S1 = m + a * l0;
    const float C2 = (a * P) / (P - S1);
    const float CP = -C2 / P;

    const vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
    const vec3 w2 = vec3(step(m + l0, x));
    const vec3 w1 = vec3(1.0 - w0 - w2);

    const vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
    const vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
    const vec3 L = vec3(m + a * (x - m));

    return T * w0 + L * w1 + S * w2;
}

vec3 toneMapUchimura(const vec3 color)
{
    const float P = sTonemapData.fOperatorParams[0];
    const float a = sTonemapData.fOperatorParams[1];
    const float m = sTonemapData.fOperatorParams[2];
    const float l = sTonemapData.fOperatorParams[3];
    const float c = sTonemapData.fOperatorParams[4];
    const float b = sTonemapData.fOperatorParams[5];

    return uchimura(color, P, a, m, l, c, b);
}

////////////////////////////////////////////////////////////////////////////////
// As explained here: https://knarkowicz.wordpress.com/2016/01/09/automatic-exposure/
// under 'Exposure compensation curve'
float getKeyAuto(const float avgLuminance)
{
    return 1.03 - (2.0 / (2.0 + log10(avgLuminance + 1.0)));
}

////////////////////////////////////////////////////////////////////////////////
float getKeyFixed(const float avgLuminance)
{
    return sTonemapData.fFixedKey;
}

////////////////////////////////////////////////////////////////////////////////
float getKey(const float avgLuminance)
{
    // Auto key method
    if (sTonemapData.uiKeyMethod == KeyMethod_AutoKey)
        return getKeyAuto(avgLuminance);

    // Fixed key method
    else if (sTonemapData.uiKeyMethod == KeyMethod_FixedKey)
        return getKeyFixed(avgLuminance);

    // Fall back to 0
    return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
float getExposureAuto(const float pixelLuminance, const float avgLuminance)
{
    // Compute the key value
    const float keyValue = getKey(avgLuminance);

    // Compute the linear exposure
    const float linearExposure = keyValue / avgLuminance;

    // Return the final results
    return exp2(log2(max(linearExposure, 0.0001)) + sTonemapData.fExposureBias);
}

////////////////////////////////////////////////////////////////////////////////
float getExposureFixed(const float pixelLuminance, const float avgLuminance)
{
    return max(sTonemapData.fFixedExposure, 0.0001);
}

////////////////////////////////////////////////////////////////////////////////
float getExposure(const float pixelLuminance, const float avgLuminance)
{
    // Auto key method
    if (sTonemapData.uiExposureMethod == ExposureMethod_AutoExposure)
        return getExposureAuto(pixelLuminance, avgLuminance);

    // Fixed key method
    else if (sTonemapData.uiExposureMethod == ExposureMethod_FixedExposure)
        return getExposureFixed(pixelLuminance, avgLuminance);

    // Fall back to 0
    return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
vec3 getExposedColor(const vec3 color, const float pixelLuminance, const float avgLuminance)
{
    return color * getExposure(pixelLuminance, avgLuminance);
}

////////////////////////////////////////////////////////////////////////////////
vec3 applyDodgeBurn(const vec3 color)
{
    // Compute the pixel's luminance
    const float pixelLuminance = max(computeLuminance(color), 0.0001);

    // Compute the exposure bias
    //const float shadowContrib = le(pixelLuminance, sTonemapData.fLuminanceThresholdShadows);
    //const float highlightContrib = ge(pixelLuminance, sTonemapData.fLuminanceThresholdHighlights);
    const float shadowContrib = (1.0 - linstep(0.0, sTonemapData.fLuminanceThresholdShadows, pixelLuminance));
    const float highlightContrib = linstep(sTonemapData.fLuminanceThresholdHighlights, 1.0, pixelLuminance);
    const float exposureBiasShadows = shadowContrib * sTonemapData.fExposureBiasShadows;
    const float exposureBiasHighlights = highlightContrib * sTonemapData.fExposureBiasHighlights;
    const float exposureBias = exposureBiasShadows + exposureBiasHighlights;

    // Return the final results
    return color * exp2(exposureBias);
}

////////////////////////////////////////////////////////////////////////////////
// Apply tone mapping to the input
vec3 toneMap(const vec3 color, const float avgLuminance)
{
    // Luminance of the pixel
    const float pixelLuminance = max(computeLuminance(color), 0.0001);

    // Compute the exposed color
    const vec3 exposedColor = getExposedColor(color, pixelLuminance, avgLuminance);

    // Operator: clamp
    if (sTonemapData.uiOperator == ToneMapOperator_Clamp)
        return toneMapClamp(exposedColor);
        
    // Operator: Reinhard
    else if (sTonemapData.uiOperator == ToneMapOperator_Reinhard)
        return toneMapReinhard(exposedColor);

    // Operator: Filmic
    else if (sTonemapData.uiOperator == ToneMapOperator_Filmic)
        return toneMapFilmic(exposedColor);
        
    // Operator: Aces
    else if (sTonemapData.uiOperator == ToneMapOperator_Aces)
        return toneMapAces(exposedColor);
        
    // Operator: Lottes
    else if (sTonemapData.uiOperator == ToneMapOperator_Lottes)
        return toneMapLottes(exposedColor);
        
    // Operator: Uchimura
    else if (sTonemapData.uiOperator == ToneMapOperator_Uchimura)
        return toneMapUchimura(exposedColor);

    // Invalid input
    return vec3(0.0);
}

////////////////////////////////////////////////////////////////////////////////
vec3 applyColorTable(const vec3 color)
{
    // Return the color unmodified without a LUT
    if (sTonemapData.bHasColorTable == 0.0) return color;

    // Apply the LUT texture
    const vec3 textureSize = textureSize(sColorTable, 0);
    const vec3 texelSize = 1.0 / textureSize;
    const vec3 scale = (textureSize - vec3(1.0)) * texelSize;
    const vec3 offset = 0.5 * texelSize;
    const vec3 uv = clamp(offset + color * scale, offset, vec3(1.0) - offset);

    return texture(sColorTable, uv).rgb;
}