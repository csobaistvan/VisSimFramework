
// Includes
#include <Shaders/OpenGL/Shading/LightTypes/common.glsl>

// Parameters for a single spot light source
struct LightParameters
{
    vec4 vPosition;
    vec4 vDirection;
    vec4 vColor;
    float fDiffuseIntensity;
    float fSpecularIntensity;
    float fRadius;
    float fCosInnerAngle;
    float fCosOuterAngle;
    mat4 mLightTransform;
    vec2 vShadowMapOffset;
};

// Light uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform LightData
{
    int iNumSources;
    uint uiAttenuationMethod;
    vec4 vAttenuationFactor;
    float fCastsShadow;
    uint uiShadowMapAlgorithm;
    uint uiShadowMapPrecision;
    float fShadowMapNear;
    float fShadowMapFar;
    float fShadowDepthBias;
    float fShadowMinVariance;
    float fShadowLightBleedBias;
    float fShadowMomentsBias;
    vec2 vShadowExponentialConstants;
    vec2 vShadowMapUvScale;
    LightParameters sLightSources[LIGHT_SOURCES_PER_BATCH];
} sLightData;

#define sLightSource sLightData.sLightSources[l]

// Other base-pass related uniforms
layout (binding = TEXTURE_SHADOW_MAP) uniform sampler2D sShadowMap;

////////////////////////////////////////////////////////////////////////////////
// Define for the 'polymorphic' light computation function
#define computeLightFn computeSpotLight

////////////////////////////////////////////////////////////////////////////////
// Invoke the proper attenuation method
float attenuation(const float radius, const float dist)
{
    if (sLightData.uiAttenuationMethod == AttenuationMethod_Quadratic)
        return attenuationQuadratic(radius, dist, sLightData.vAttenuationFactor.xyz);
    else if (sLightData.uiAttenuationMethod == AttenuationMethod_Realistic)
        return attenuationRealistic(radius, dist);
    return 1.0;
}

////////////////////////////////////////////////////////////////////////////////
// Lighting computation
vec4 computeSpotLight(const SurfaceInfo surface, const MaterialInfo material, const float posOffset,
                      const float diffuseScale, const float specularScale, const float ambientScale)
{
    // Go over the light list and evaluate them
    vec4 result = vec4(0.0);
    for (int l = 0; l < sLightData.iNumSources; ++l)
    {
        // Vector pointing towards the light source
        const vec3 toLight = sLightSource.vPosition.xyz - surface.position;
        const float dist = length(toLight);
        const vec3 toLightNorm = toLight / dist;
        const float theta = dot(toLightNorm, normalize(-sLightSource.vDirection.xyz));

        // Make sure the fragment is lit by the light
        if (dist > sLightSource.fRadius || theta < sLightSource.fCosOuterAngle)
            continue;
        
        // Calculate the light attenuation   
        const float attenuation = attenuation(sLightSource.fRadius, dist);

        // Calculate the light falloff
        const float falloff = saturate((theta - sLightSource.fCosOuterAngle) / (sLightSource.fCosInnerAngle - sLightSource.fCosOuterAngle));
        
        // Light-space position of the pixel
        const vec4 posLS = sLightSource.mLightTransform * vec4(surface.position, 1.0);
        const vec3 positionLS = saturate(ndcToScreen(posLS.xyz / posLS.w));
        const vec2 uv = sLightSource.vShadowMapOffset + positionLS.xy * sLightData.vShadowMapUvScale;
        const float depth = linearDepth(positionLS.z, sLightData.fShadowMapNear, sLightData.fShadowMapFar);

        // Shadow map parameters
        ShadowParameters shadowParameters;
        shadowParameters.uv = uv;
        shadowParameters.depth = depth;
        shadowParameters.castsShadow = sLightData.fCastsShadow != 0.0;
        shadowParameters.algorithm = sLightData.uiShadowMapAlgorithm;
        shadowParameters.smPrecision = sLightData.uiShadowMapPrecision;
        shadowParameters.depthBias = sLightData.fShadowDepthBias;
        shadowParameters.lightBleedBias = sLightData.fShadowLightBleedBias;
        shadowParameters.momentsBias = sLightData.fShadowMomentsBias;
        shadowParameters.minVariance = sLightData.fShadowMinVariance;
        shadowParameters.exponentialConstants = sLightData.vShadowExponentialConstants;

        // Construct the description of the light
        LightInfo light;
        light.toLight = toLightNorm;
        light.dist = dist;
        light.ambient = ambientScale * vec3(0.0);
        light.diffuse = diffuseScale * sLightSource.vColor.rgb * max(sLightSource.fDiffuseIntensity, 0.0); 
        light.specular = specularScale * sLightSource.vColor.rgb * max(sLightSource.fSpecularIntensity, 0.0);
        light.attenuation = falloff * attenuation;
        light.shadow = sampleShadowMap(sShadowMap, shadowParameters);

        // Evaluate the light model and sum the result
        result += vec4(applyBrdfDirect(surface, material, light), light.shadow);
    }

    // Return the result
    return vec4(result.rgb, result.a / sLightData.iNumSources);
}