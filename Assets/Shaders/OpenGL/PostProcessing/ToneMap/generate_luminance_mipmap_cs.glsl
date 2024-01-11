#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/ToneMap/common.glsl>

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Input and output textures
layout(binding = 0, rg16f) uniform writeonly image2D sLuminanceMip;

// Mipmap parameters
layout (location = 0) uniform ivec2 vMipDimension;
layout (location = 1) uniform uint uiMipLevel;

void main()
{
    // Only process valid coordinates
	if (gl_GlobalInvocationID.x >= vMipDimension.x ||
		gl_GlobalInvocationID.y >= vMipDimension.y) return;

    // Source and target positions
	const ivec2 writePos = ivec2(gl_GlobalInvocationID.xy);
	const ivec2 sourcePos = writePos * 2;

    // Fetch buffer
	vec2 values[4] =
    {
        texelFetch(sLuminance, sourcePos + ivec2(0, 0), int(uiMipLevel - 1)).rg,
        texelFetch(sLuminance, sourcePos + ivec2(1, 0), int(uiMipLevel - 1)).rg,
        texelFetch(sLuminance, sourcePos + ivec2(0, 1), int(uiMipLevel - 1)).rg,
        texelFetch(sLuminance, sourcePos + ivec2(1, 1), int(uiMipLevel - 1)).rg
    };
    
    // Compute the avg and max luminances
    const float L_avg = (values[0].r + values[1].r + values[2].r + values[3].r) * 0.25;
    const float L_max = max(max(values[0].g, values[1].g), max(values[2].g, values[3].g));
	imageStore(sLuminanceMip, writePos, vec4(L_avg, L_max, 0.0, 0.0));
}