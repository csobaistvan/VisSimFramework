#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/GBufferBasepass/common.glsl>

// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec3 vPosition;
layout (location = VERTEX_ATTRIB_NORMAL) in vec3 vNormal;
layout (location = VERTEX_ATTRIB_TANGENT) in vec3 vTangent;
layout (location = VERTEX_ATTRIB_BITANGENT) in vec3 vBitangent;
layout (location = VERTEX_ATTRIB_UV) in vec2 vUv;

// Output attribs
out VertexData
{
    vec3 vPosition;
    vec3 vPrevPosition;
    vec3 vNormal;
    vec2 vUv;
    mat3 mTBN;
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
    v_out.vPrevPosition = vec3(mPrevModel * vec4(getVertexPosition(), 1));
    v_out.vNormal = normalize(vec3(mNormal * vec4(vNormal, 0)));
    v_out.vUv = vUv;
    v_out.mTBN = mat3
    (
        normalize(vec3(mNormal * vec4(vTangent, 0))),
        normalize(vec3(mNormal * vec4(vBitangent, 0))),
        normalize(vec3(mNormal * vec4(vNormal, 0)))
    );
    
    gl_Position = sCameraData.mViewProjection * vec4(v_out.vPosition, 1);
}