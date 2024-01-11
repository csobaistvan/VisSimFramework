#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/DepthPrepass/common.glsl>

// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec3 vPosition;
layout (location = VERTEX_ATTRIB_UV) in vec2 vUv;

// Output attribs
out VertexData
{
    vec3 vPosition;
    vec3 vPrevPosition;
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
    v_out.vPosition = vec3(mModel * vec4(getVertexPosition(), 1));
    v_out.vPrevPosition = vec3(sCameraData.mPrevView * mPrevModel * vec4(getVertexPosition(), 1));
    v_out.vUv = vUv;
    
    gl_Position = sCameraData.mViewProjection * vec4(v_out.vPosition, 1);
}