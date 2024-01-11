
// Include the common headers
#include <Shaders/OpenGL/Shading/BRDF/brdf.glsl>
#include <Shaders/OpenGL/Shading/Shadows/shadow_maps.glsl>

////////////////////////////////////////////////////////////////////////////////
// Attenuation methods

// Quadratic attenuation
float attenuationQuadratic(const float radius, const float dist, const vec3 attenuationFactor)
{
    return saturate(1.0 / dot(vec3(1.0f, dist, dist * dist) * attenuationFactor, vec3(1.0)));
}

// 'realistic' attenuation
float attenuationRealistic(const float radius, const float dist)
{
    const float distNorm = saturate(dist / radius);
    const float attenuation = pow(1.0 - pow(distNorm, 4.0), 2.0);
    return (dist * dist) * saturate(attenuation) / (dist * dist + 1);
}
