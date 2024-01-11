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

void main()
{
	// Compute the coordinates of the current tile
    const ivec2 tileId = ivec2(gl_WorkGroupID.yz);
    
    // Index into the count/dispatch buffer
    const uint arrayIndex = tileArrayIndex(tileId.xy);
    const uint countArrayIndex = tileCountArrayIndex(tileId.xy);

    // Number of fragments in the current tile
    const uint fragmentCount = sTileParametersBuffer[countArrayIndex].uiNumFragmentsTotal;

    // Maximum number of elements that need to be sorted
    const uint maxSortElements = nextpow2(fragmentCount);
    
    // Skip irrelevant invocations
    if (maxSortElements < (SORT_SHARED_ARRAY_SIZE << sFragmentSortData.uiGroupId))
        return;

    // Form unique index pair from dispatch thread ID
    const uint index2 = insertonebit(gl_GlobalInvocationID.x, sFragmentSortData.uiCompareDistance);
    const uint index1 = index2 ^ (sFragmentSortData.uiGroupSize == 2 * sFragmentSortData.uiCompareDistance ? sFragmentSortData.uiGroupSize - 1 : sFragmentSortData.uiCompareDistance);

    // Sort the elements, if the indices are valid
    if (index1 < fragmentCount && index2 < fragmentCount)
    {
        sortElements(sTileSortBuffer[arrayIndex + index1], sTileSortBuffer[arrayIndex + index2]);
    }
}