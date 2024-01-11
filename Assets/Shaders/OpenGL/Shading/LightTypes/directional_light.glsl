
// Includes
#include <Shaders/OpenGL/Shading/LightTypes/common.glsl>

// Light uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform LightData
{
    vec4 vDirection;
    vec4 vColor;
    float fAmbientIntensity;
    float fDiffuseIntensity;
    float fSpecularIntensity;
    float fCastsShadow;
    uint uiShadowMapAlgorithm;
    uint uiShadowMapPrecision;
    float fShadowDepthBias;
    float fShadowMinVariance;
    float fShadowLightBleedBias;
    float fShadowMomentsBias;
    vec2 vShadowExponentialConstants;
    mat4 mLightTransform;
} sLightData;

// Other base-pass related uniforms
layout (binding = TEXTURE_SHADOW_MAP) uniform sampler2D sShadowMap;

////////////////////////////////////////////////////////////////////////////////
// Define for the 'polymorphic' light computation function
#define computeLightFn computeDirectionalLight

////////////////////////////////////////////////////////////////////////////////
// Lighting computation
vec4 computeDirectionalLight(const SurfaceInfo surface, const MaterialInfo material, const float posOffset,
                             const float diffuseScale, const float specularScale, const float ambientScale)
{
    // Light-space position of the pixel
    const vec4 posLS = sLightData.mLightTransform * vec4(surface.position, 1.0);
    const vec3 positionLS = ndcToScreen(posLS.xyz / posLS.w);

    // Shadow map parameters
    ShadowParameters shadowParameters;
    shadowParameters.uv = positionLS.xy;
    shadowParameters.depth = positionLS.z;
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
    light.toLight = -sLightData.vDirection.xyz;
    light.dist = 0.0;
    light.ambient = ambientScale * sLightData.vColor.rgb * max(sLightData.fAmbientIntensity, 0.0);
    light.diffuse = diffuseScale * sLightData.vColor.rgb * max(sLightData.fDiffuseIntensity, 0.0); 
    light.specular = specularScale * sLightData.vColor.rgb * max(sLightData.fSpecularIntensity, 0.0);
    light.attenuation = 1.0;
    light.shadow = sampleShadowMap(sShadowMap, shadowParameters);

    // Evaluate the light model and return the result
    return vec4(applyBrdfDirect(surface, material, light), light.shadow);
}