#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Environment/VolumetricClouds/common.glsl>

// Required extensions
#extension GL_NV_viewport_array2 : enable

////////////////////////////////////////////////////////////////////////////////
// Input layout
layout (triangles, invocations = NUM_GS_INVOCATIONS_FOR_LAYERS) in;
layout (triangle_strip, max_vertices = 3) out;

////////////////////////////////////////////////////////////////////////////////
// Also pass through the other attribs
in VertexData
{
    vec3 vWorldPos;
} gs_in[];

////////////////////////////////////////////////////////////////////////////////
// Also pass through the other attribs
out GeometryData
{
    vec3 vWorldPos;
} gs_out;

////////////////////////////////////////////////////////////////////////////////
void main()
{
    if (shouldSkipLayerInvocation()) return;

    for (int i = 0; i < 3; ++i)
    {
        handleLayerMask();
        gs_out.vWorldPos = gs_in[i].vWorldPos;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}