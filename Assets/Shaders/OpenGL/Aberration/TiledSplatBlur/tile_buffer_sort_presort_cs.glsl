#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = SORT_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

// Groupshared sort key array
shared SortIndex sSortIndices[SORT_SHARED_ARRAY_SIZE];

void main()
{
	// Compute the coordinates of the current tile
    const ivec2 tileId = ivec2(gl_WorkGroupID.yz);

    // Index into the count/dispatch buffer
    const uint arrayIndex = tileArrayIndex(tileId.xy);
    const uint countArrayIndex = tileCountArrayIndex(tileId.xy);

    // Group start index
    const uint groupStart = gl_WorkGroupID.x * SORT_SHARED_ARRAY_SIZE;
    const uint threadStart = gl_LocalInvocationID.x * SORT_ELEMENTS_PER_THREAD;

    // Number of fragments in the current tile
    const uint fragmentCount = sTileParametersBuffer[countArrayIndex].uiNumFragmentsTotal;

    // Compute the maximum necessary group size
    const uint maxGroupSize = min(int(nextpow2(fragmentCount - groupStart)), SORT_SHARED_ARRAY_SIZE);

    // Skip irrelevant invocations
    if (groupStart >= fragmentCount || threadStart >= maxGroupSize)
        return;

    // Load unsorted elements from the buffer
    for (int batchId = 0; batchId < SORT_ELEMENTS_PER_THREAD; ++batchId)
    for (int elementId = 0; elementId < 2; ++elementId)
    {
        // Compute the array index for the sort index
        const uint sharedElementIndex = threadStart + batchId + elementId * (maxGroupSize / 2);
        const uint elementIndex = groupStart + sharedElementIndex;

        // Fetch the sort index; unused elements must sort to the end
        sSortIndices[sharedElementIndex] = elementIndex < fragmentCount ? sTileSortBuffer[arrayIndex + elementIndex] : nullSortIndex();
    }

    memoryBarrierShared();
    barrier();
    
    // Inner sort iteration
    for (uint k = 2; k <= maxGroupSize; k <<= 1)
    for (uint j = k / 2; j > 0; j /= 2)
    {
        for (int batchId = 0; batchId < SORT_ELEMENTS_PER_THREAD; ++batchId)
        {
            // Indices
            uint index2 = insertonebit(threadStart + batchId, j);
            uint index1 = index2 ^ (k == 2 * j ? k - 1 : j);

            // Sort the elements
            sortElements(sSortIndices[index1], sSortIndices[index2]);
        }

		memoryBarrierShared();
        barrier();
    }
    
    // Write sorted results to memory
    for (int batchId = 0; batchId < SORT_ELEMENTS_PER_THREAD; ++batchId)
    for (int elementId = 0; elementId < 2; ++elementId)
    {
        // Compute the array index for the sort index
        const uint sharedElementIndex = threadStart + batchId + elementId * (maxGroupSize / 2);
        const uint elementIndex = groupStart + sharedElementIndex;

        // Write out valid indices
        if (elementIndex < fragmentCount)
        {
            sTileSortBuffer[arrayIndex + elementIndex] = sSortIndices[sharedElementIndex];
        }
    }
}