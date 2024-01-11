#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

// Various texture maps
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler2DArray sTexture;

// Copy parameters
layout (location = 0) uniform vec2 vUvScale = vec2(1.0);
layout (location = 1) uniform vec2 vUvMin = vec2(0.0);
layout (location = 2) uniform vec2 vUvMax = vec2(1.0);

// Input attribus
in GeometryData
{
    vec2 vUv;
} g_out;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    // Compute the texture UV
    const vec2 uv = vUvMin + vUvScale * g_out.vUv;

    // Copy the buffer
    colorBuffer = texture(sTexture, vec3(uv, gl_Layer));
}