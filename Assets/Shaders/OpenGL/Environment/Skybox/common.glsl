
#include <Shaders/OpenGL/Common/Optional/depth_peel.glsl>

// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform SkyboxData
{
    vec3 vTint;
    float bHasTexture;
    float bRenderToAllLayers;
} sSkyboxData;

//The main skybox texture
layout (binding = TEXTURE_ALBEDO_MAP) uniform samplerCube sSkybox;

// The previous depth texture
layout (binding = TEXTURE_DEPTH) uniform sampler2DArray sPrevDepthBuffer;