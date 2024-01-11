#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/DepthPrepass/common.glsl>

// Input attribus
in GeometryData
{
    vec3 vPosition;
    vec3 vPrevPosition;
    vec2 vUv;
} fs_in;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
    colorBuffer = vec4(1.0);
}