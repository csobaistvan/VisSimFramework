#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Kernel size
layout(local_size_x = SPLAT_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // Index into the sort buffer
    const uint splatArrayIndex = gl_GlobalInvocationID.x;

    // Extract the sort index entry
    const SplatIndex splatIndex = sTileSplatBuffer[splatArrayIndex];

	// Compute the coordinates of the current tile
    const ivec2 tileId = ivec2(unpackUint2x16(splatIndex.uiTileId));
    
    // Go throuch each neighbor
    const int numTilesSplat = int(ROUNDED_DIV(sTiledSplatBlurData.uiMaxBlurRadiusCurrent, TILE_SIZE));
    for (int i = -numTilesSplat; i <= numTilesSplat; ++i)
    for (int j = -numTilesSplat; j <= numTilesSplat; ++j)
    {
        // Compute the neighbor's offset and tile ID
        const ivec2 neighborOffset = ivec2(i, j);
        const ivec2 neighborTileId = tileId + neighborOffset;
        
        // Skip irrelevant tiles
        if (neighborOffset == ivec2(0)) continue; // Ourselves
        if (!isValidTileId(neighborTileId)) continue; // Invalid neighbor IDs
        if (!overlapsTile(neighborTileId, splatIndex.vScreenPosition, splatIndex.fBlurRadius)) continue; // Non-overlapping tiles

        // Compute the array index for the current tile
        const uint neighborTileArrayIndex = tileArrayIndex(neighborTileId);
        const uint neighborCountArrayIndex = tileCountArrayIndex(neighborTileId);

        // Append the sort index into the
        const uint neighborElementIndex = atomicAdd(sTileParametersBuffer[neighborCountArrayIndex].uiNumFragmentsTotal, 1);
        sTileSortBuffer[neighborTileArrayIndex + neighborElementIndex] = makeSortIndex(splatIndex);
    }
}