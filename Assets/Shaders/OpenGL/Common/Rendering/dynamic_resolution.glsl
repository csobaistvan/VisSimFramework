#include <Shaders/OpenGL/Generated/resource_indices.glsl>
#include <Shaders/OpenGL/Common/color_spaces.glsl>

// Dynamic resolution texture lookup
vec4 textureDR(sampler2D tex, vec2 uv, vec2 scale)
{
    return texture(tex, vec2(uv * scale));
}

vec4 textureDR(sampler2D tex, vec2 uv)
{
    return textureDR(tex, uv, sRenderData.vUvScale);
}

vec4 textureDR(sampler2DArray tex, vec2 uv, vec2 scale, int layer)
{
    return texture(tex, vec3(uv * scale, float(layer)));
}

vec4 textureDR(sampler2DArray tex, vec2 uv, int layer)
{
    return textureDR(tex, uv, sRenderData.vUvScale, layer);
}

vec4 textureLodDR(sampler2D tex, vec2 uv, vec2 scale, float lod)
{
    return textureLod(tex, vec2(uv * scale), lod);
}

vec4 textureLodDR(sampler2D tex, vec2 uv, float lod)
{
    return textureLodDR(tex, uv, sRenderData.vUvScale, lod);
}

vec4 textureLodDR(sampler2DArray tex, vec2 uv, vec2 scale, int layer, float lod)
{
    return textureLod(tex, vec3(uv * scale, float(layer)), lod);
}

vec4 textureLodDR(sampler2DArray tex, vec2 uv, int layer, float lod)
{
    return textureLodDR(tex, uv, sRenderData.vUvScale, layer, lod);
}