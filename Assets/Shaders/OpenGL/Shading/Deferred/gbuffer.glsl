#include <Shaders/OpenGL/Shading/surface_descriptors.glsl>

////////////////////////////////////////////////////////////////////////////////
// No MSAA
#if !defined(MSAA) || MSAA == 0
    #define SAMPLER_TYPE sampler2DArray
    #define sampleGbuffer(smpler, uv, layerId, sampleId) textureDR(smpler, uv, layerId)
    #define fetchGbuffer(smpler, uv, layerId, sampleId) texelFetch(smpler, ivec3(uv, layerId), 0)

////////////////////////////////////////////////////////////////////////////////
// MSAA
#elif MSAA == 1
    #define SAMPLER_TYPE sampler2DMSArray
    #define sampleGbuffer(smpler, uv, layerId, sampleId) texelFetch(smpler, ivec3(uv * sRenderData.vResolution, layerId), sampleId)
    #define fetchGbuffer(smpler, uv, layerId, sampleId) texelFetch(smpler, ivec3(uv, layerId), sampleId)
#endif

////////////////////////////////////////////////////////////////////////////////
// Various texture maps
layout (binding = TEXTURE_ALBEDO_MAP) uniform SAMPLER_TYPE sSceneColor;
layout (binding = TEXTURE_NORMAL_MAP) uniform SAMPLER_TYPE sSceneNormal;
layout (binding = TEXTURE_SPECULAR_MAP) uniform SAMPLER_TYPE sSceneSpecular;
layout (binding = TEXTURE_DEPTH) uniform SAMPLER_TYPE sSceneDepth;

////////////////////////////////////////////////////////////////////////////////
#define DECLARE_COMPONENT(NAME, TYPE, TEXTURE, CHANNELS, FN) \
    TYPE CONCAT(gbuffer, NAME)(const vec2 uv, const int layerId, const int sampleId) \
    { return FN(sampleGbuffer(TEXTURE, uv, layerId, sampleId).CHANNELS); } \
    TYPE CONCAT(gbuffer, NAME)(const vec2 uv, const int sampleId) \
    { return FN(sampleGbuffer(TEXTURE, uv, 0, sampleId).CHANNELS); } \
    TYPE CONCAT(gbuffer, NAME)(const vec2 uv) \
    { return FN(sampleGbuffer(TEXTURE, uv, 0, 0).CHANNELS); } \
    TYPE CONCAT(gbuffer, NAME)(const ivec2 tc, const int layerId, const int sampleId) \
    { return FN(fetchGbuffer(TEXTURE, tc, layerId, sampleId).CHANNELS); } \
    TYPE CONCAT(gbuffer, NAME)(const ivec2 tc, const int sampleId) \
    { return FN(fetchGbuffer(TEXTURE, tc, 0, sampleId).CHANNELS); } \
    TYPE CONCAT(gbuffer, NAME)(const ivec2 tc) \
    { return FN(fetchGbuffer(TEXTURE, tc, 0, 0).CHANNELS); }

////////////////////////////////////////////////////////////////////////////////
DECLARE_COMPONENT(Depth, float, sSceneDepth, r, )
DECLARE_COMPONENT(LinearDepth, float, sSceneDepth, r, linearDepth)
DECLARE_COMPONENT(Albedo, vec3, sSceneColor, rgb, )
DECLARE_COMPONENT(Revealage, float, sSceneColor, a, )
DECLARE_COMPONENT(Normal, vec3, sSceneNormal, rgb, normalize)
DECLARE_COMPONENT(Specular, float, sSceneNormal, a, )
DECLARE_COMPONENT(Metallic, float, sSceneSpecular, r, )
DECLARE_COMPONENT(Roughness, float, sSceneSpecular, g, )
DECLARE_COMPONENT(Velocity, vec2, sSceneSpecular, ba, )

////////////////////////////////////////////////////////////////////////////////
#undef DECLARE_COMPONENT
#undef SAMPLER_TYPE
#undef sampleGbuffer

////////////////////////////////////////////////////////////////////////////////
// Extract the surface information from the GBuffer
SurfaceInfo sampleGbufferSurface(const vec2 uv, const int layerId, const int sampleId)
{
    SurfaceInfo surface;
    surface.position = reconstructPositionWS(uv, gbufferDepth(uv, layerId, sampleId));
    surface.normal = gbufferNormal(uv, layerId, sampleId);
    return surface;
}

////////////////////////////////////////////////////////////////////////////////
// Extract the material information from the GBuffer
MaterialInfo sampleGbufferMaterial(const vec2 uv, const int layerId, const int sampleId)
{
    MaterialInfo material;
    material.albedo = gbufferAlbedo(uv, layerId, sampleId);
    material.roughness = gbufferRoughness(uv, layerId, sampleId);
    material.metallic = gbufferMetallic(uv, layerId, sampleId);
    material.specular = gbufferSpecular(uv, layerId, sampleId);
    return material;
}