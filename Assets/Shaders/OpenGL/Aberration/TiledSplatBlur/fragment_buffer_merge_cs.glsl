#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/TiledSplatBlur/common.glsl>

// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_2) uniform FragmentMergeData
{    
	uint uiBlockSize;
	float fColorSimilarityThreshold;
	float fColorContrastThreshold;
	float fDepthSimilarityThreshold;
	float fMinBlurRadiusThreshold;
} sFragmentMergeData;

// Kernel size
layout(local_size_x = FRAGMENT_MERGE_GROUP_SIZE, local_size_y = FRAGMENT_MERGE_GROUP_SIZE, local_size_z = 1) in;

void main()
{
    // Fragment block size for the current and next passes
    const int nextBlockSize = int(sFragmentMergeData.uiBlockSize);
    const int currentBlockSize = nextBlockSize / 2;

	// Compute the coordinates of the pixel
	const ivec2 fragmentCoord = ivec2(gl_GlobalInvocationID.xy) * nextBlockSize;

	// Output array index
	const uint arrayIndex = fragmentArrayIndex(fragmentCoord.xy);

	// Skip if we are outside the actual image
	if (any(greaterThanEqual(fragmentCoord.xy, sTiledSplatBlurData.vResolution)))
		return;

    // Fragment array indices for the 4 fragments to process
    const uint fragmentIndices[4] = 
    {
        fragmentArrayIndex(fragmentCoord.xy + ivec2(0, 0) * currentBlockSize),
        fragmentArrayIndex(fragmentCoord.xy + ivec2(1, 0) * currentBlockSize),
        fragmentArrayIndex(fragmentCoord.xy + ivec2(0, 1) * currentBlockSize),
        fragmentArrayIndex(fragmentCoord.xy + ivec2(1, 1) * currentBlockSize)
    };

    // List of frontmost fragments
    FragmentData fragments[4] =
    {
         unpackFragmentData(sFragmentBuffer[fragmentIndices[0]]),
         unpackFragmentData(sFragmentBuffer[fragmentIndices[1]]),
         unpackFragmentData(sFragmentBuffer[fragmentIndices[2]]),
         unpackFragmentData(sFragmentBuffer[fragmentIndices[3]]),
    };

    // Minimum blur radius threshold
    const float minBlurRadius = min
    (
        min(minBlurRadii(fragments[0].vPsfIndex)[2], minBlurRadii(fragments[1].vPsfIndex)[2]),
        min(minBlurRadii(fragments[2].vPsfIndex)[2], minBlurRadii(fragments[3].vPsfIndex)[2])
    );

    // Thresholds
    const float blurSizeThreshold = step(sFragmentMergeData.fMinBlurRadiusThreshold, minBlurRadius);
    const float colorThreshold = blurSizeThreshold * minBlurRadius * sFragmentMergeData.fColorSimilarityThreshold;
    const float contrastThreshold = blurSizeThreshold * minBlurRadius * sFragmentMergeData.fColorContrastThreshold;
    const float depthThreshold = blurSizeThreshold * minBlurRadius * sFragmentMergeData.fDepthSimilarityThreshold;

    // Color similarity - per-channel
    const bool mergeCondColorSimilarity =
    (
        abs(fragments[0].vColor.x - fragments[1].vColor.x) < colorThreshold &&
        abs(fragments[0].vColor.x - fragments[2].vColor.x) < colorThreshold &&
        abs(fragments[0].vColor.x - fragments[3].vColor.x) < colorThreshold &&
        abs(fragments[1].vColor.x - fragments[2].vColor.x) < colorThreshold &&
        abs(fragments[1].vColor.x - fragments[3].vColor.x) < colorThreshold &&
        abs(fragments[2].vColor.x - fragments[3].vColor.x) < colorThreshold &&

        abs(fragments[0].vColor.y - fragments[1].vColor.y) < colorThreshold &&
        abs(fragments[0].vColor.y - fragments[2].vColor.y) < colorThreshold &&
        abs(fragments[0].vColor.y - fragments[3].vColor.y) < colorThreshold &&
        abs(fragments[1].vColor.y - fragments[2].vColor.y) < colorThreshold &&
        abs(fragments[1].vColor.y - fragments[3].vColor.y) < colorThreshold &&
        abs(fragments[2].vColor.y - fragments[3].vColor.y) < colorThreshold &&

        abs(fragments[0].vColor.z - fragments[1].vColor.z) < colorThreshold &&
        abs(fragments[0].vColor.z - fragments[2].vColor.z) < colorThreshold &&
        abs(fragments[0].vColor.z - fragments[3].vColor.z) < colorThreshold &&
        abs(fragments[1].vColor.z - fragments[2].vColor.z) < colorThreshold &&
        abs(fragments[1].vColor.z - fragments[3].vColor.z) < colorThreshold &&
        abs(fragments[2].vColor.z - fragments[3].vColor.z) < colorThreshold
    );

    // Color contrast
    const bool mergeCondColorContrast = maxElem
    (
        max
        (
            max
            (
                max
                (
                    abs(fragments[0].vColor - fragments[1].vColor),
                    abs(fragments[0].vColor - fragments[2].vColor)
                ),
                max
                (
                    abs(fragments[0].vColor - fragments[3].vColor),
                    abs(fragments[1].vColor - fragments[2].vColor)
                )
            ),
            max
            (
                abs(fragments[1].vColor - fragments[3].vColor),
                abs(fragments[2].vColor - fragments[3].vColor)
            )
        )
    ) <= contrastThreshold;

    // We can only merge if they have similar depth values
    const bool mergeCondDefocus =
    (
        abs(fragments[0].vPsfIndex.z - fragments[1].vPsfIndex.z) +
        abs(fragments[0].vPsfIndex.z - fragments[2].vPsfIndex.z) +
        abs(fragments[0].vPsfIndex.z - fragments[3].vPsfIndex.z) +
        abs(fragments[1].vPsfIndex.z - fragments[2].vPsfIndex.z) +
        abs(fragments[1].vPsfIndex.z - fragments[3].vPsfIndex.z) +
        abs(fragments[2].vPsfIndex.z - fragments[3].vPsfIndex.z)
    ) <= depthThreshold * 6.0;

    // We can only merge if the total size of each fragment is equal to
    // 4 times the current block size
    const bool mergeCondFragmentSize = 
    (
        fragments[0].uiFragmentSize + 
        fragments[1].uiFragmentSize + 
        fragments[2].uiFragmentSize + 
        fragments[3].uiFragmentSize
    ) == (currentBlockSize * 4);

    // Update the merged fragment count
    uint numOutFragments = 4;

    // Merge the fragment if we can
    if (mergeCondFragmentSize && mergeCondColorSimilarity && mergeCondColorContrast && mergeCondDefocus)
    {
        // Update the merged fragment count
        numOutFragments = 1;

        // Average the colors
        fragments[0].vColor = 
        (
            fragments[0].vColor + 
            fragments[1].vColor + 
            fragments[2].vColor + 
            fragments[3].vColor
        ) * 0.25;
        
        // Average the screen positions
        fragments[0].vScreenPosition = 
        (
            fragments[0].vScreenPosition + 
            fragments[1].vScreenPosition + 
            fragments[2].vScreenPosition + 
            fragments[3].vScreenPosition
        ) * 0.25;
        
        // Average the PSF coordinates
        fragments[0].vPsfIndex = 
        (
            fragments[0].vPsfIndex + 
            fragments[1].vPsfIndex + 
            fragments[2].vPsfIndex + 
            fragments[3].vPsfIndex
        ) * 0.25;
        
        // Double the fragment size
        fragments[0].uiFragmentSize *= 2;

        // Recompute the blur radius
        const float blurRadius = maxBlurRadii(fragments[0].vPsfIndex)[2] + calcFragmentSizeOffset(fragments[0].uiFragmentSize);
        fragments[0].uiBlurRadius = uint(ceil(blurRadius));
    }

    // Write out the fragments
    for (int i = 0; i < numOutFragments; ++i)
        sFragmentBuffer[arrayIndex + i] = packFragmentData(fragments[i]);
}