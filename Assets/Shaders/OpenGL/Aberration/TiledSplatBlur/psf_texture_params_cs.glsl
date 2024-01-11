#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = INTERPOLATION_GROUP_SIZE, local_size_y = INTERPOLATION_GROUP_SIZE, local_size_z = 1) in;

void main()
{
    interpolatePsfParams();
}