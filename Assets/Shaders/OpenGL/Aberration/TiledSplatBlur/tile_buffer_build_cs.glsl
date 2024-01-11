#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = MERGED_TILE_SIZE, local_size_y = MERGED_TILE_SIZE, local_size_z = 1) in;

// Tile array index
shared uint uiNumFragments = 0;

// Splat array index
shared uint uiSplatArrayIndex = 0;

void main()
{
	// Compute the coordinates of the current tile
    const ivec2 tileId = ivec2(gl_WorkGroupID.xy);
	const ivec2 threadId = ivec2(gl_LocalInvocationID.xy);

    // Compute the current fragment's array index
    const ivec2 fragmentCoord = mergedFragmentCoords(tileId, threadId);
    const uint fragmentArrayIndex = fragmentArrayIndex(fragmentCoord.xy);

	// Skip if we are outside the actual image
	if (any(greaterThanEqual(fragmentCoord.xy, sTiledSplatBlurData.vResolution)))
		return;

    // Number of fragments in the merged fragment array
    const uint numFragments = calcNumFragments(unpackFragmentSize(sFragmentBuffer[fragmentArrayIndex]));
    //const uint numFragments = 1;
    
	// Compute the array index for the current tile data
    const uint tileArrayIndex = tileArrayIndex(tileId.xy);
    const uint countArrayIndex = tileCountArrayIndex(tileId.xy);

    // Clear the dispatch buffer and the fragment counter    
    sTileDispatchBuffer[countArrayIndex].vNumGroups = uvec3(0);
    uiNumFragments = 0;
    memoryBarrierShared(); // Wait until each thread sees the cleared counter
    barrier();

    // Add the fragment count to the total counter
    const uint tileArrayStartIndex = atomicAdd(uiNumFragments, numFragments);
    memoryBarrierShared(); // Wait until each thread adds its fragment count to the group count
    barrier();

    // The first thread increments the fragment counter and generates a splat key
    if (threadId == ivec2(0))
    {
        // Write out the number of fragments
        sTileParametersBuffer[countArrayIndex].uiNumFragmentsTile = uiNumFragments;
        sTileParametersBuffer[countArrayIndex].uiNumFragmentsTotal = uiNumFragments;

        // Index into the splat array
        uiSplatArrayIndex = atomicAdd(sTileDispatchBuffer[0].uiIndex, uiNumFragments);
    }
    memoryBarrierShared(); // Wait until each thread sees the splat array index
    barrier();

    // Generate splat indices for the merged fragments
    for (uint fragmentId = 0; fragmentId < numFragments; ++fragmentId)
    {
        // Index of the current fragment
        const uint fragmentIndex = fragmentArrayIndex + fragmentId;

        // Unpack the fragment data
        const FragmentData fragmentData = unpackFragmentData(sFragmentBuffer[fragmentIndex]);

        // Make splat and sort indices
        const SplatIndex splatIndex = makeSplatIndex(fragmentIndex, tileId, fragmentData);
        const SortIndex sortIndex = makeSortIndex(splatIndex);

        // Write out the indices
        const uint elementIndex = tileArrayStartIndex + fragmentId;
        sTileSplatBuffer[uiSplatArrayIndex + elementIndex] = splatIndex;
        sTileSortBuffer[tileArrayIndex + elementIndex] = sortIndex;

        // Store the result
        sFragmentBuffer[fragmentIndex] = packFragmentData(convertToTileFragmentData(fragmentData));
    }
}