#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_2) uniform FragmentMergeData
{
    uint uiGroupId;
	uint uiGroupSize;       // k >= SORT_SHARED_ARRAY_SIZE
	uint uiCompareDistance; // j >= SORT_SHARED_ARRAY_SIZE / 2 && j < k
} sFragmentSortData;

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

    // Number of fragments in the current tile
    const uint fragmentCount = sTileParametersBuffer[countArrayIndex].uiNumFragmentsTotal;

    // Maximum number of elements that need to be sorted
    const uint maxSortElements = nextpow2(fragmentCount);

    // Skip irrelevant invocations
    if (groupStart >= fragmentCount || (maxSortElements < SORT_SHARED_ARRAY_SIZE << sFragmentSortData.uiGroupId))
        return;

    // Load unsorted elements from the buffer
    for (int batchId = 0; batchId < SORT_ELEMENTS_PER_THREAD; ++batchId)
    for (int elementId = 0; elementId < 2; ++elementId)
    {
        // Compute the array index for the sort index
        const uint sharedElementIndex = gl_LocalInvocationID.x * SORT_ELEMENTS_PER_THREAD + batchId + elementId * (SORT_SHARED_ARRAY_SIZE / 2);
        const uint elementIndex = groupStart + sharedElementIndex;

        // Fetch the sort index; unused elements must sort to the end
        sSortIndices[sharedElementIndex] = elementIndex < fragmentCount ? sTileSortBuffer[arrayIndex + elementIndex] : nullSortIndex();
    }

    memoryBarrierShared();
    barrier();

    // Inner sort iteration
    for (uint j = SORT_SHARED_ARRAY_SIZE / 2; j > 0; j /= 2)
    {
        for (int batchId = 0; batchId < SORT_ELEMENTS_PER_THREAD; ++batchId)
        {
            // Indices
            uint index2 = insertonebit(gl_LocalInvocationID.x * SORT_ELEMENTS_PER_THREAD + batchId, j);
            uint index1 = index2 ^ j;

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
        const uint elementIndex = gl_LocalInvocationID.x * SORT_ELEMENTS_PER_THREAD + batchId + elementId * (SORT_SHARED_ARRAY_SIZE / 2);

        // Write out valid indices
        if (groupStart + elementIndex < fragmentCount)
        {
            sTileSortBuffer[arrayIndex + groupStart + elementIndex] = sSortIndices[elementIndex];
        }
    }
}
