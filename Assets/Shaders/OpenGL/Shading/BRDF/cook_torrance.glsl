////////////////////////////////////////////////////////////////////////////////
// Normal-distribution function (D): Trowbridge-Reitz GGX
float distributionGGX(const float NdotH, const float a)
{
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    //return num;
    return num / denom;
}
////////////////////////////////////////////////////////////////////////////////
// Normal-distribution function (D): Beckmann
float distributionBeckmann(const float NdotH, const float a)
{
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;
    float NdotH4 = NdotH2 * NdotH2;
    
    float num = exp((NdotH2 - 1.0) / max((a2 * NdotH2), 0.0001));
    float denom = max(PI * a2 * NdotH4, 0.0001);

    return num / denom;
}

////////////////////////////////////////////////////////////////////////////////
// Geometry function (G): GGX/Schlick-Beckmann
float geometrySchlickGGX(const float NdotV, const float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

////////////////////////////////////////////////////////////////////////////////
// Geometry function (G): Smith's method
float geometrySmith(const float NdotV, const float NdotL, const float roughness)
{
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

////////////////////////////////////////////////////////////////////////////////
// Fresnel's function (F): Shlick's approximation
vec3 fresnelSchlick(const float HdotV, const vec3 F0)
{
    //return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);

    // do this manually as 'pow(1.0 - HdotV, 5.0)' tends to lead to NaN's
    const float OmHdotV = (1.0 - HdotV);
    const float OmHdotV2 = OmHdotV * OmHdotV;

    return F0 + (1.0 - F0) * OmHdotV2 * OmHdotV2 * OmHdotV;
}

////////////////////////////////////////////////////////////////////////////////
// Fresnel's function (F): Shlick's approximation
vec3 fresnelSchlickRoughness(const float NdotV, const vec3 F0, const float roughness)
{
    //return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - NdotV, 0.0), 5.0);

    // do this manually as 'pow(1.0 - HdotV, 5.0)' tends to lead to NaN's
    const float OmNdotV = (1.0 - NdotV);
    const float OmNdotV2 = OmNdotV * OmNdotV;

    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * OmNdotV2 * OmNdotV2 * OmNdotV;
} 

////////////////////////////////////////////////////////////////////////////////
// Based on: https://learnopengl.com/PBR/Lighting
vec3 brdfCookTorranceDirect(const SurfaceInfo surface, const MaterialInfo material, const LightInfo light)
{
    // Surface normal and to-eye vectors
    const vec3 N = surface.normal;
    const vec3 L = light.toLight;
    const vec3 V = normalize(sCameraData.vEye - surface.position);
    const vec3 H = normalize(V + L);

    // Dot products
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float HdotV = max(dot(H, V), 0.0);

    // Alpha (roughness * roughness)
    const float roughness = max(material.roughness, 0.05);
    const float roughness2 = roughness * roughness;

    // Calculate the radiance
    const vec3 radiance = vec3(light.attenuation * light.shadow);
    
    // Cook-Torrance BRDF
    //const float NDF = distributionGGX(NdotH, roughness2);
    const float NDF = distributionBeckmann(NdotH, roughness2);
    const float G   = geometrySmith(NdotV, NdotL, roughness);
    const vec3  F0  = mix(vec3(0.04), material.albedo * material.specular, material.metallic);
    const vec3  F   = fresnelSchlick(HdotV, F0);

    // Diffuse and specular 
    const vec3 kS = F;
    const vec3 kD = (vec3(1.0) - kS) * (1.0 - material.metallic);

    const vec3 diffuse  = light.diffuse * kD * material.albedo / PI;
    const vec3 specular = light.specular * (NDF * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    
    // Add to outgoing radiance Lo
    const vec3 directLight = (diffuse + specular) * radiance * NdotL;

    // Compute the ambient term
    const vec3 ambient = light.ambient * material.albedo;
    return directLight + ambient;
}

////////////////////////////////////////////////////////////////////////////////
vec3 brdfCookTorranceIndirect(const SurfaceInfo surface, const MaterialInfo material, const LightInfo light)
{
    // Surface normal and to-eye vectors
    const vec3 N = surface.normal;
    const vec3 L = light.toLight;
    const vec3 V = normalize(sCameraData.vEye - surface.position);
    const vec3 H = normalize(V + L);

    // Dot products
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float HdotV = max(dot(H, V), 0.0);

    // Alpha (roughness * roughness)
    const float roughness = material.roughness;
    const float roughness2 = roughness * roughness;

    // Calculate the radiance
    const vec3 radiance = vec3(light.attenuation * light.shadow);
    
    // Cook-Torrance BRDF
    //const float NDF = distributionGGX(NdotH, roughness2);
    const float NDF = distributionBeckmann(NdotH, roughness2);
    const float G   = geometrySmith(NdotV, NdotL, roughness);
    const vec3  F0  = mix(vec3(0.04), material.albedo * material.specular, material.metallic);
    const vec3  F   = fresnelSchlickRoughness(NdotV, F0, roughness);

    // Diffuse and specular 
    const vec3 kS = F;
    const vec3 kD = (vec3(1.0) - kS) * (1.0 - material.metallic);

    const vec3 diffuse  = light.diffuse * kD * material.albedo / PI;
    const vec3 specular = light.specular * (NDF * G * F) / max(4 * NdotV * NdotL, 0.001);
    
    // Add to outgoing radiance Lo
    const vec3 directLight = (diffuse + specular) * radiance * NdotL;

    // Compute the ambient term
    const vec3 ambient = light.ambient * material.albedo;
    return directLight + ambient;
}