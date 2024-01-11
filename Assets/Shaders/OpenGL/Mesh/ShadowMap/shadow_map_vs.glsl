#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/ShadowMap/common.glsl>

// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec3 vPosition;
layout (location = VERTEX_ATTRIB_UV) in vec2 vUv;

// Output attribs
out VertexData
{
    vec2 vUv;
} v_out;

// Computes the mapped vertex position
vec3 getVertexPosition()
{
    return lerp(vPosition, vec3(vUv, 0.0), fMeshToUV);
}

// Entry point
void main()
{
    v_out.vUv = vUv;
    gl_Position = mLight * mModel * vec4(getVertexPosition(), 1);
}