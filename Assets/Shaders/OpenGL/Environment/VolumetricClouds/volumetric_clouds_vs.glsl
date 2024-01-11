#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Environment/VolumetricClouds/common.glsl>

// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec2 vPosition;

// Output attribs
out VertexData
{
    vec3 vWorldPos;
} v_out;

// Entry point
void main()
{
    // Calculate the cubemap texture coordinates
    v_out.vWorldPos = clipToWorldSpace(vec3(vPosition, 1)).xyz;

    // Write out the positions
    gl_Position = vec4(vPosition.xy, 1, 1);
}