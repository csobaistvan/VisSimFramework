#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // Extract the total number of fragments
    const uint numFragments = sTileDispatchBuffer[0].uiIndex;;

    // Compute the number of groups required
    const uint numGroups = ROUNDED_DIV(numFragments, SPLAT_GROUP_SIZE);

    // Write out the dispatch parameters
    sTileDispatchBuffer[0].vNumGroups = uvec3(numGroups, 1, 1);
}