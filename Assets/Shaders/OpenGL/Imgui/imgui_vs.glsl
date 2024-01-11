#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

// Projection matrix
layout (location = 0) uniform mat4 mProj;

// Input attribs.
layout (location = VERTEX_ATTRIB_POSITION) in vec2 vPos;
layout (location = VERTEX_ATTRIB_UV) in vec2 vUv;
layout (location = VERTEX_ATTRIB_COLOR) in vec4 vColor;

// Output attribs.
out vec2 vUvVs;
out vec4 vColorVs;

void main()
{
	vUvVs = vUv;
	vColorVs = vColor;
    
	gl_Position = mProj * vec4(vPos.xy, 0, 1);
}