#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Environment/Skybox/common.glsl>

// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec2 vPosition;

// Output attribs
out VertexData
{
    vec3 vPosition;
	vec3 vUv;
} v_out;

// Entry point
void main()
{
    // Calculate the cubemap texture coordinates
    const vec3 wsPosition = clipToWorldSpace(vec3(vPosition, 1)).xyz;
    v_out.vUv = wsPosition * vec3(1, -1, -1);
    v_out.vPosition = wsPosition;

    // Write out the positions
    gl_Position = vec4(vPosition.xy, 1, 1);
}