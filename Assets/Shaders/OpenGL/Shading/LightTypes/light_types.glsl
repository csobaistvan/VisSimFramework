
////////////////////////////////////////////////////////////////////////////////
// Define the various light types available
#define DIRECTIONAL 0
#define POINT 1
#define SPOT 2

////////////////////////////////////////////////////////////////////////////////
// Include the common headers
#include <Shaders/OpenGL/Shading/LightTypes/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// Include the appropriate light header
#includeif(LIGHT_TYPE == DIRECTIONAL) <Shaders/OpenGL/Shading/LightTypes/directional_light.glsl>
#includeif(LIGHT_TYPE == POINT) <Shaders/OpenGL/Shading/LightTypes/point_light.glsl>
#includeif(LIGHT_TYPE == SPOT) <Shaders/OpenGL/Shading/LightTypes/spot_light.glsl>

////////////////////////////////////////////////////////////////////////////////
// Light computation overloads

vec4 computeLight(const SurfaceInfo surface, const MaterialInfo material, const float posOffset,
                  const float diffuseScale, const float specularScale, const float ambientScale)
{
    return computeLightFn(surface, material, posOffset, diffuseScale, specularScale, ambientScale);
}

vec4 computeLight(const SurfaceInfo surface, const MaterialInfo material)
{
    return computeLightFn(surface, material, 0.0, 1.0, 1.0, 1.0);
}