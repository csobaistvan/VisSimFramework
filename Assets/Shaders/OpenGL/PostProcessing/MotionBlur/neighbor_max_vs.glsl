#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/MotionBlur/common.glsl>

// Input attribs
layout (location = VERTEX_ATTRIB_POSITION) in vec3 vPosition;

// Output attribs
out vec2 vUv;

// Entry point
void main()
{
    vUv = vPosition.xy * 0.5 + 0.5;
    
    gl_Position = vec4(vPosition, 1);
}