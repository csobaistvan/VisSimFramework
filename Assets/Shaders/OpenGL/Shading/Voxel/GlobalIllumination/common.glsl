// Include the common headers
#include <Shaders/OpenGL/Shading/BRDF/brdf.glsl>
#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>
#include <Shaders/OpenGL/Shading/Voxel/light_trace.glsl>

// Light uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform LightData
{
    TraceDiffuseConeParams sDiffuseTraceParams;
    TraceSpecularConeParams sSpecularTraceParams;
} sLightData;

////////////////////////////////////////////////////////////////////////////////
vec4 calculateIndirectLighting(const SurfaceInfo surface, const MaterialInfo material)
{
    // Calculate the diffuse term
    const vec4 diffuse = traceDiffuseCone(surface, material, sLightData.sDiffuseTraceParams);

    // Calculate the specular term
    const vec3 specular = traceSpecularCone(surface, material, sLightData.sSpecularTraceParams);
        
    // Combine and return the final result
    return vec4(diffuse.rgb * diffuse.a + specular, diffuse.a);
}