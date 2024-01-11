#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/GlobalIllumination/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec3 vPosition;

////////////////////////////////////////////////////////////////////////////////
// Output attribs
out VertexData
{
	vec2 vUv;
} v_out;

////////////////////////////////////////////////////////////////////////////////
// Entry point
void main()
{
    v_out.vUv = vPosition.xy * 0.5 + 0.5;
    
    gl_Position = vec4(vPosition.xy, 1, 1);
}