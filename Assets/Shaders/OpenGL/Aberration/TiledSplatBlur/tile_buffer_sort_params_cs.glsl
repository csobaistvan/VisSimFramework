#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = NUM_SORT_ITERATIONS, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // Id of the current group
    int groupId = int(gl_LocalInvocationID.x);

    // Only generate commands for the necessary group sizes
    if (groupId >= sTiledSplatBlurData.uiNumSortIterations)
        return;

	// Compute the coordinates of the current tile
    const ivec2 tileId = ivec2(gl_WorkGroupID.xy);
    
    // Index into the count/dispatch buffer
    const uint countArrayIndex = tileCountArrayIndex(tileId.xy);

    // Number of fragments in the current tile
    const uint fragmentCount = sTileParametersBuffer[countArrayIndex].uiNumFragmentsTotal;

    // Current group size
    const uint k = SORT_SHARED_ARRAY_SIZE << groupId;

    // Maximum number of elements that need to be sorted
    const uint maxSortElements = max(nextpow2(fragmentCount), SORT_SHARED_ARRAY_SIZE);

    // Only generate sort commands when needed
    const uint sortFragmentCount = k > maxSortElements ? 0 : fragmentCount;

    // Current dispatch element index
    // We need one more iteration every time the number of thread groups doubles
    uint dispatchElementIndex = (groupId * (groupId + 1)) / 2;

    // Generate outer sort dispatch arguments
    for (uint j = k / 2; j > SORT_SHARED_ARRAY_SIZE / 2; j /= 2)
    {
        // All of the groups of size 2j that are full
        const uint completeGroups = (sortFragmentCount & ~(2 * j - 1)) / (SORT_GROUP_SIZE * 2);

        // Number of remaining fragments
        const uint remainingFragments = max(int(sortFragmentCount - completeGroups * SORT_GROUP_SIZE * 2 - j), 0);

        // Remaining items must only be sorted if there are more than j of them
        const uint partialGroups = ROUNDED_DIV(remainingFragments, SORT_GROUP_SIZE);

        // Store the dispatch parameters
        atomicMax(sTileDispatchBuffer[dispatchElementIndex].vNumGroups.x, completeGroups + partialGroups);
        sTileDispatchBuffer[dispatchElementIndex].vNumGroups.y = sTiledSplatBlurData.vNumTiles.x;
        sTileDispatchBuffer[dispatchElementIndex].vNumGroups.z = sTiledSplatBlurData.vNumTiles.y;

        // Increment the element index
        ++dispatchElementIndex;
    }

    // The inner sort always sorts all groups (rounded up to multiples of SORT_SHARED_ARRAY_SIZE)
    const uint innerGroups = ROUNDED_DIV(sortFragmentCount, SORT_SHARED_ARRAY_SIZE);
    atomicMax(sTileDispatchBuffer[dispatchElementIndex].vNumGroups.x, innerGroups);
    sTileDispatchBuffer[dispatchElementIndex].vNumGroups.y = sTiledSplatBlurData.vNumTiles.x;
    sTileDispatchBuffer[dispatchElementIndex].vNumGroups.z = sTiledSplatBlurData.vNumTiles.y;
}