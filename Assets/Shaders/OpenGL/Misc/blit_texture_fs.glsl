#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

// Texture to display
layout(binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sTexture;

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    // Write out the result
    colorBuffer.rgb = texelFetch(sTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    colorBuffer.a = 1.0;
}