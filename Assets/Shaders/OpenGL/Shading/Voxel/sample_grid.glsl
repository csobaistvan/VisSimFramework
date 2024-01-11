#include <Shaders/OpenGL/Shading/surface_descriptors.glsl>
#include <Shaders/OpenGL/Shading/Voxel/common.glsl>

////////////////////////////////////////////////////////////////////////////////
layout(binding = TEXTURE_POST_PROCESS_1) uniform sampler3D sVoxelAlbedo;
layout(binding = TEXTURE_POST_PROCESS_2) uniform sampler3D sVoxelNormal;
layout(binding = TEXTURE_POST_PROCESS_3) uniform sampler3D sVoxelSpecular;
layout(binding = TEXTURE_POST_PROCESS_4) uniform sampler3D sVoxelRadiance;
layout(binding = TEXTURE_POST_PROCESS_5) uniform sampler3D sVoxelRadianceAnisotropic[6];

////////////////////////////////////////////////////////////////////////////////
bool isVoxelEmpty(const ivec3 voxelPos)
{
    return texelFetch(sVoxelAlbedo, voxelPos, 0).a == 0.0;
}

////////////////////////////////////////////////////////////////////////////////
bool isVoxelEmpty(const vec3 texCoords)
{
    return texture(sVoxelAlbedo, texCoords).a == 0.0;
}

////////////////////////////////////////////////////////////////////////////////
vec3 sampleVoxelAlbedo(const ivec3 voxelPos)
{
    return texelFetch(sVoxelAlbedo, voxelPos, 0).rgb;
}

////////////////////////////////////////////////////////////////////////////////
vec3 sampleVoxelAlbedo(const vec3 texCoords)
{
    return texture(sVoxelAlbedo, texCoords, 0).rgb;
}

////////////////////////////////////////////////////////////////////////////////
vec3 sampleVoxelNormal(ivec3 voxelPos)
{
    return normalize(texelFetch(sVoxelNormal, voxelPos, 0).rgb * 2.0 - 1.0);
}

////////////////////////////////////////////////////////////////////////////////
vec3 sampleVoxelNormal(const vec3 texCoords)
{
    return normalize(texture(sVoxelNormal, texCoords).rgb * 2.0 - 1.0);
}

////////////////////////////////////////////////////////////////////////////////
vec3 sampleVoxelSpecular(ivec3 voxelPos)
{
    return texelFetch(sVoxelSpecular, voxelPos, 0).rgb;
}

////////////////////////////////////////////////////////////////////////////////
vec3 sampleVoxelSpecular(const vec3 texCoords)
{
    return texture(sVoxelSpecular, texCoords).rgb;
}

////////////////////////////////////////////////////////////////////////////////
vec4 sampleVoxelRadianceIsotropic(const vec3 texCoords, const float mipLevel)
{
    return textureLod(sVoxelRadiance, texCoords, mipLevel);
}

////////////////////////////////////////////////////////////////////////////////
vec4 sampleVoxelRadianceAnisotropic(const vec3 texCoords, const float mipLevel, const uvec3 visibleFace, const vec3 weight)
{
    // Take the anisotropic sample
    const float anistropicMipLevel = max(mipLevel - 1.0, 0.0);
    const vec4 anisotropicSample = normalWeightedSample(weight,
        textureLod(sVoxelRadianceAnisotropic[visibleFace.x], texCoords, anistropicMipLevel),
        textureLod(sVoxelRadianceAnisotropic[visibleFace.y], texCoords, anistropicMipLevel),
        textureLod(sVoxelRadianceAnisotropic[visibleFace.z], texCoords, anistropicMipLevel)
    );

    // Sample return if we are purely in the mip levels
    if (mipLevel >= 1.0) return anisotropicSample;
    
    // Manually blend with the base level otherwise
    const vec4 baseSample = texture(sVoxelRadiance, texCoords);
    return mix(baseSample, anisotropicSample, saturate(mipLevel));
}

////////////////////////////////////////////////////////////////////////////////
SurfaceInfo sampleVoxelSurface(const ivec3 voxelPos)
{
    SurfaceInfo result;
    result.position = voxelToWorld(voxelPos);
    result.normal = sampleVoxelNormal(voxelPos);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
SurfaceInfo sampleVoxelSurface(const vec3 wsPos)
{
    SurfaceInfo result;
    result.position = wsPos;
    result.normal = sampleVoxelNormal(worldToNormalizedVoxel(wsPos));
    return result;
}

////////////////////////////////////////////////////////////////////////////////
MaterialInfo sampleVoxelMaterial(const ivec3 voxelPos)
{
    MaterialInfo result;

    result.albedo = sampleVoxelAlbedo(voxelPos);
    const vec3 specular = sampleVoxelSpecular(voxelPos);
    result.metallic = specular.r;
    result.roughness = specular.g;
    result.specular = specular.b;
    return result;
}

////////////////////////////////////////////////////////////////////////////////
MaterialInfo sampleVoxelMaterial(const vec3 wsPos)
{
    MaterialInfo result;

    const vec3 texCoords = worldToNormalizedVoxel(wsPos);
    result.albedo = sampleVoxelAlbedo(texCoords);
    const vec3 specular = sampleVoxelSpecular(texCoords);
    result.metallic = specular.r;
    result.roughness = specular.g;
    result.specular = specular.b;
    return result;
}