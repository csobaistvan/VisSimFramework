#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

/* sum(rgb * a, a) */
uniform sampler2D sAccumTexture;

/* prod(1 - a) */
uniform sampler2D sRevealageTexture;

// Input attribus
in GeometryData
{
    vec2 vUv;
} g_out;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    // Sample coordinates
    const ivec2 texCoords = ivec2(gl_FragCoord.xy);

    // Get the fragment's revealage factor
    const float revealage = texelFetch(sRevealageTexture, texCoords, 0).r;

    // Ignore opaque fragments
    if (revealage == 1.0) discard;

    // Read the accummulated translucent pixels
    const vec4 accum = texelFetch(sAccumTexture, texCoords, 0);

    // Normalize the result
    const vec3 averageColor = accum.rgb / max(accum.a, 0.00001);

    // Blend into the opaque buffer, using the 'revealage' as the alpha
    colorBuffer = vec4(averageColor, 1.0 - revealage);
}