#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

// Font/other texture.
layout (binding = 0) uniform sampler2D sTexture;

// Attribs
in vec2 vUvVs;
in vec4 vColorVs;

// Output
out vec4 fragColor;

void main()
{
    fragColor = vColorVs * texture(sTexture, vUvVs.st);
}