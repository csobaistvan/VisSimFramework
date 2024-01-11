////////////////////////////////////////////////////////////////////////////////
// Material uniforms
layout (location = 0) uniform vec4 bFlagsA;  // albedo, normal, specular, alpha
layout (location = 1) uniform vec4 bFlagsB;  // alpha, displacement, 0, 0
layout (location = 2) uniform vec3 vAlbedoTint;
layout (location = 3) uniform vec3 vEmissiveColor;
layout (location = 4) uniform float fOpacity;
layout (location = 5) uniform float fMetallic;
layout (location = 6) uniform float fRoughness;
layout (location = 7) uniform float fSpecular;
layout (location = 8) uniform float fNormalMapStrength;
layout (location = 9) uniform float fDisplacement;
layout (location = 10) uniform uint uiNormalMappingAlgorithm;
layout (location = 11) uniform uint uiDisplacementMappingAlgorithm;
layout (location = 12) uniform vec4 vSpecularMapSpecularMask;
layout (location = 13) uniform vec4 vSpecularMapRoughnessMask;
layout (location = 14) uniform vec4 vSpecularMapMetallicMask;

////////////////////////////////////////////////////////////////////////////////
// Various texture maps
layout (binding = TEXTURE_ALBEDO_MAP) uniform sampler2D sAlbedoMap;
layout (binding = TEXTURE_NORMAL_MAP) uniform sampler2D sNormalMap;
layout (binding = TEXTURE_SPECULAR_MAP) uniform sampler2D sSpecularMap;
layout (binding = TEXTURE_ALPHA_MAP) uniform sampler2D sAlphaMap;
layout (binding = TEXTURE_DISPLACEMENT_MAP) uniform sampler2D sDisplacementMap;

////////////////////////////////////////////////////////////////////////////////
bool isTwoSided()         { return bFlagsA.x == 1.0; }
bool hasAlbedoMap()       { return bFlagsA.y == 1.0; }
bool hasNormalMap()       { return bFlagsA.z == 1.0; }
bool hasSpecularMap()     { return bFlagsA.w == 1.0; }
bool hasAlphaMap()        { return bFlagsB.x == 1.0; }
bool hasDisplacementMap() { return bFlagsB.y == 1.0; }
bool hasMapByMask(const vec4 mask) { return any(greaterThan(mask, vec4(0.0))); }

////////////////////////////////////////////////////////////////////////////////
// Parallax mapping
vec2 parallaxMap(const vec2 uv, const vec3 viewDirection)
{
    // Initial values: basic parallax mapping
    float currentDepthMapValue = texture2D(sDisplacementMap, uv).r;

    // Basic parallax mapping ends here
    if (uiDisplacementMappingAlgorithm == DisplacementMappingMethod_BasicParallaxMapping)
        return uv - viewDirection.xy * (currentDepthMapValue * fDisplacement);

    // Initial values: steep parallax mapping
    vec2  currentTexCoords  = uv;
    float currentLayerDepth = 0.0;

    // The amount to shift the texture coordinates per layer (from vector P)
    const vec2 P = (viewDirection.xy / viewDirection.z) * fDisplacement; 

    // Depth layer parameters
    const float minLayers = 8;
    const float maxLayers = 32; // TODO: make these uniforms
    const float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDirection)));  
    const float layerDepth = 1.0 / numLayers;
    const vec2 deltaTexCoords = P / numLayers;
    
    // Iterate over the depth layers
    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get displacement value at current texture coordinates
        currentDepthMapValue = texture2D(sDisplacementMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }

    if (uiDisplacementMappingAlgorithm == DisplacementMappingMethod_SteepParallaxMapping)
        return currentTexCoords;
    
    // Get texture coordinates before collision (reverse operations)
    const vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // Get depth after and before collision for linear interpolation
    const float afterDepth  = currentDepthMapValue - currentLayerDepth;
    const float beforeDepth = texture2D(sDisplacementMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // Interpolation of texture coordinates
    const float weight = afterDepth / (afterDepth - beforeDepth);
    const vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec2 applyDisplacementMap(const vec2 uv, const vec3 viewDirection)
{
    // Parallax mapping
    if (uiDisplacementMappingAlgorithm == DisplacementMappingMethod_BasicParallaxMapping ||
        uiDisplacementMappingAlgorithm == DisplacementMappingMethod_SteepParallaxMapping ||
        uiDisplacementMappingAlgorithm == DisplacementMappingMethod_ParallaxOcclusionMapping)
    {
        return parallaxMap(uv, viewDirection);
    }

    // Simply return the uv unmodified
    return uv;
}

////////////////////////////////////////////////////////////////////////////////
// Material normal function
vec3 applyNormalMap(const vec2 uv, const vec3 normal, const mat3 tbn)
{
    const vec3 normalMap = 2.0 * texture2D(sNormalMap, uv).rgb - 1.0;
    return lerp(normal, normalize(tbn * normalMap), fNormalMapStrength);
}

////////////////////////////////////////////////////////////////////////////////
struct MaterialInfo
{
    vec2 uv;
    vec3 albedo;
    vec3 emission;
    float opacity;
    vec3 normal;
    float metallic;
    float roughness;
    float specular;
};

////////////////////////////////////////////////////////////////////////////////
MaterialInfo evaluateMaterialFn(const vec2 uv, const vec3 normal, const mat3 tbn, const vec3 viewDirection, const bool frontFacing)
{
    MaterialInfo material;

    ////////////////////////////////////////////////////////////////////////////////
    // UV
    ////////////////////////////////////////////////////////////////////////////////
    material.uv = uv;
    if (hasDisplacementMap()) material.uv = applyDisplacementMap(material.uv, viewDirection);

    ////////////////////////////////////////////////////////////////////////////////
    // Albedo & opacity
    ////////////////////////////////////////////////////////////////////////////////
    material.albedo = vAlbedoTint.rgb;
    material.opacity = fOpacity;

    // Sample the albedo map
    if (hasAlbedoMap()) 
    {
        const vec4 albedoMap = srgbToLinear(texture2D(sAlbedoMap, material.uv));
        material.albedo *= albedoMap.rgb;
        material.opacity *= albedoMap.a;
    }
    
    // Sample the mask texture
    if (hasAlphaMap()) material.opacity *= texture2D(sAlphaMap, material.uv).r;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Emissive
    ////////////////////////////////////////////////////////////////////////////////
    material.emission = vEmissiveColor;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Normal
    ////////////////////////////////////////////////////////////////////////////////
    material.normal = normalize(normal);
    if (hasNormalMap()) material.normal = applyNormalMap(material.uv, material.normal, tbn);
    if (isTwoSided() && !frontFacing) material.normal *= -1.0;

    ////////////////////////////////////////////////////////////////////////////////
    // Metallic, roughness, specular, ao
    ////////////////////////////////////////////////////////////////////////////////
    material.metallic = fMetallic;
    material.roughness = fRoughness;
    material.specular = fSpecular;

    // Sample the specular map
    if (hasSpecularMap())
    {
        const vec4 specularMap = texture2D(sSpecularMap, material.uv);

        if (hasMapByMask(vSpecularMapMetallicMask))  material.metallic = dot(specularMap, vSpecularMapMetallicMask);
        if (hasMapByMask(vSpecularMapRoughnessMask)) material.roughness = dot(specularMap, vSpecularMapRoughnessMask);
        if (hasMapByMask(vSpecularMapSpecularMask))  material.specular = dot(specularMap, vSpecularMapSpecularMask);
    }

    return material;
}

////////////////////////////////////////////////////////////////////////////////
MaterialInfo evaluateMaterialFn(const vec2 uv)
{
    return evaluateMaterialFn(uv, vec3(0, 0, 1), mat3(1.0), vec3(0, 0, 1), true);
}