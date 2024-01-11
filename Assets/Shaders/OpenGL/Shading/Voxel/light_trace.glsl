
#include <Shaders/OpenGL/Shading/BRDF/brdf.glsl>
#include <Shaders/OpenGL/Shading/Voxel/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/cone_trace.glsl>

////////////////////////////////////////////////////////////////////////////////
struct TraceDiffuseConeParams
{
    uint contribution;
    float intensity;
    float aperture;
    float mipOffset;
    float startOffset;
    float normalOffset;
    float maxTraceDistance;
    uint anisotropic;
    float radianceDecayRate;
    float occlusionDecayRate;
    float occlusionExponent;
    float minOcclusion;
};

////////////////////////////////////////////////////////////////////////////////
const uint NUM_DIFFUSE_SAMPLES = 6;
const vec3 vDiffuseConeDirections[NUM_DIFFUSE_SAMPLES] =
{
    vec3( 0.0,       1.0,  0.0),
    vec3( 0.0,       0.5,  0.866025),
    vec3( 0.823639,  0.5,  0.267617),
    vec3(-0.823639,  0.5,  0.267617),
    vec3( 0.509037,  0.5, -0.700629),
    vec3(-0.509037,  0.5, -0.700629),
};
const float fDiffuseConeWeights[NUM_DIFFUSE_SAMPLES] =
{
    1.0 * PI /  4.0,
    3.0 * PI / 20.0,
    3.0 * PI / 20.0,
    3.0 * PI / 20.0,
    3.0 * PI / 20.0,
    3.0 * PI / 20.0,
};

////////////////////////////////////////////////////////////////////////////////
// Helper for producing a guide vector
const vec3 guideVector(const vec3 normal)
{
    return (abs(dot(normal, vec3(0.0, 1.0, 0.0))) >= 0.99) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
}

////////////////////////////////////////////////////////////////////////////////
const mat3 getTangentMatrix(const vec3 direction)
{
    // Find a tangent and a bitangent
    const vec3 guide = guideVector(direction);
    const vec3 right = normalize(guide - dot(direction, guide) * direction);
    const vec3 up = cross(right, direction);
    return mat3(right, direction, up);
}

////////////////////////////////////////////////////////////////////////////////
const vec3 diffuseContributionLambertian(const SurfaceInfo surface, const MaterialInfo material, 
    const ConeTraceParameters traceParameters, const ConeTraceResult traceSample)
{
    return traceSample.color * max(dot(surface.normal, traceParameters.direction), 0.0) * material.albedo;
}

////////////////////////////////////////////////////////////////////////////////
const vec3 diffuseContributionBRDF(const SurfaceInfo surface, const MaterialInfo material, 
    const ConeTraceParameters traceParameters, const ConeTraceResult traceSample)
{
    // Create a dummy light model
    LightInfo light;
    light.toLight = traceParameters.direction;
    light.dist = 0.0;
    light.ambient = vec3(0.0);
    light.diffuse = traceSample.color * PI; 
    light.specular = vec3(0.0);
    light.attenuation = 1.0;
    light.shadow = 1.0;

    // Return the evaluated BRDF
    return applyBrdfIndirect(surface, material, light);
}

////////////////////////////////////////////////////////////////////////////////
const vec3 diffuseContribution(const uint method, const SurfaceInfo surface, const MaterialInfo material, 
    const ConeTraceParameters traceParameters, const ConeTraceResult traceSample)
{
    // Contribution: Lambertian
    if (method == LightContribution_Lambertian)
        return diffuseContributionLambertian(surface, material, traceParameters, traceSample);
    
    // Contribution: BRDF
    if (method == LightContribution_BRDF)
        return diffuseContributionBRDF(surface, material, traceParameters, traceSample);

    // Failure case
    return vec3(0.0);
}

////////////////////////////////////////////////////////////////////////////////// 
// Calculates indirect lighting for the input surface location
vec4 traceDiffuseCone(const SurfaceInfo surface, const MaterialInfo material, const TraceDiffuseConeParams lightTraceParams)
{
    // Find a suitable local basis
    const mat3 tbn = getTangentMatrix(surface.normal);

    // Init the cone trace parameters
    ConeTraceParameters traceParameters;
    traceParameters.normal = surface.normal;
    traceParameters.position = surface.position;
    traceParameters.aperture = lightTraceParams.aperture;
    traceParameters.mipOffset = lightTraceParams.mipOffset;
    traceParameters.traceStartOffset = lightTraceParams.startOffset;
    traceParameters.traceNormalOffset = lightTraceParams.normalOffset;
    traceParameters.maxTraceDistance = lightTraceParams.maxTraceDistance;
    traceParameters.anisotropic = lightTraceParams.anisotropic == 1;
    traceParameters.radianceDecay = lightTraceParams.radianceDecayRate;
    traceParameters.occlusionDecay = lightTraceParams.occlusionDecayRate;
    traceParameters.firstHitOnly = true;

    // Accummulate the samples
    vec3 diffuseTrace = vec3(0.0);
    float aoTrace = 0.0;
    for (int i = 0; i < NUM_DIFFUSE_SAMPLES; i++)
    {
        // Direction in which to trace
        traceParameters.direction = normalize(tbn * vDiffuseConeDirections[i]);
        
        // Perform the cone tracing
        const ConeTraceResult traceSample = traceCone(traceParameters);

        // Accummulate the AO
        diffuseTrace += diffuseContribution(lightTraceParams.contribution, surface, material, traceParameters, traceSample);
        aoTrace += traceSample.occlusion * fDiffuseConeWeights[i];
    }

    // Compute the final AO term
    //aoTrace = clamp(pow(1.0 - aoTrace, lightTraceParams.occlusionExponent), lightTraceParams.minOcclusion, 1.0);
    aoTrace = clamp(pow(1.0 - saturate(aoTrace), lightTraceParams.occlusionExponent), lightTraceParams.minOcclusion, 1.0);

    // Saturate and return
    return vec4(lightTraceParams.intensity * diffuseTrace, aoTrace);
}

////////////////////////////////////////////////////////////////////////////////
struct TraceSpecularConeParams
{
    uint contribution;
    float intensity;
    float apertureMin;
    float apertureMax;
    float mipLevelMin;
    float mipLevelMax;
    float startOffset;
    float normalOffset;
    float maxTraceDistance;
    uint anisotropic;
    float radianceDecayRate;
};

////////////////////////////////////////////////////////////////////////////////
const vec3 specularContributionLambertian(const SurfaceInfo surface, const MaterialInfo material, 
    const ConeTraceParameters traceParameters, const ConeTraceResult traceSample)
{
    return traceSample.color * max(dot(surface.normal, traceParameters.direction), 0.0);
}

////////////////////////////////////////////////////////////////////////////////
const vec3 specularContributionBRDF(const SurfaceInfo surface, const MaterialInfo material, 
    const ConeTraceParameters traceParameters, const ConeTraceResult traceSample)
{
    // Find a suitable local basis
    const mat3 tbn = getTangentMatrix(traceParameters.direction);

    // Angle of the specular cone
    const float coneAngle = atan(traceParameters.aperture);

    // Directions on the perimeter of the cone
    const vec3 directions[4] = 
    {
        normalize(tbn * sph2cart(PI * 0.0, coneAngle, 1.0)),
        normalize(tbn * sph2cart(PI * 0.5, coneAngle, 1.0)),
        normalize(tbn * sph2cart(PI * 1.0, coneAngle, 1.0)),
        normalize(tbn * sph2cart(PI * 1.5, coneAngle, 1.0)),
    };
    
    // Create a dummy light object
    LightInfo light;
    light.dist = 0.0;
    light.ambient = vec3(0.0);
    light.diffuse = vec3(0.0); 
    light.specular = traceSample.color;
    light.attenuation = 1.0;
    light.shadow = 1.0;
    
    // Apply the result by approximating the integral over the cone surface
    vec3 result = vec3(0.0);
    for (int i = 0; i < 4; ++i) 
    {
        light.toLight = directions[i];
        result += applyBrdfIndirect(surface, material, light);
    }

    // Return the normalized result
    return result / 4.0;
}

////////////////////////////////////////////////////////////////////////////////
const vec3 specularContribution(const uint method, const SurfaceInfo surface, const MaterialInfo material, 
    const ConeTraceParameters traceParameters, const ConeTraceResult traceSample)
{
    // Contribution: Lambertian
    if (method == LightContribution_Lambertian)
        return specularContributionLambertian(surface, material, traceParameters, traceSample);
    
    // Contribution: BRDF
    if (method == LightContribution_BRDF)
        return specularContributionBRDF(surface, material, traceParameters, traceSample);

    // Failure case
    return vec3(0.0);
}

////////////////////////////////////////////////////////////////////////////////
// Calculates indirect specular lighting
vec3 traceSpecularCone(const SurfaceInfo surface, const MaterialInfo material, const TraceSpecularConeParams lightTraceParams)
{
    // Calculate the incident light dir
    const vec3 incidentDir = surface.position - sCameraData.vEye;

    // Common trace settings
    ConeTraceParameters traceParameters;
    traceParameters.position = surface.position;
    traceParameters.normal = surface.normal;
    traceParameters.direction = normalize(reflect(incidentDir, surface.normal));
    traceParameters.aperture = mix(lightTraceParams.apertureMin, lightTraceParams.apertureMax, material.roughness);
    traceParameters.mipOffset = mix(lightTraceParams.mipLevelMin, lightTraceParams.mipLevelMax, material.roughness);
    traceParameters.traceStartOffset = lightTraceParams.startOffset;
    traceParameters.traceNormalOffset = lightTraceParams.normalOffset;
    traceParameters.maxTraceDistance = lightTraceParams.maxTraceDistance;
    traceParameters.anisotropic = lightTraceParams.anisotropic == 1;
    traceParameters.radianceDecay = lightTraceParams.radianceDecayRate;
    traceParameters.firstHitOnly = true;
        
    // Perform the cone tracing
    const ConeTraceResult traceSample = traceCone(traceParameters);
    if (!traceSample.anyHit) return vec3(0.0);
    
    // Return the weighted sample
    return lightTraceParams.intensity * specularContribution(lightTraceParams.contribution, surface, material, traceParameters, traceSample);
}