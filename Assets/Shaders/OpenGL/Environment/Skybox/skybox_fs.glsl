#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Environment/Skybox/common.glsl>

// Input attribus
in GeometryData
{
    vec3 vPosition;
    vec3 vUv;
} fs_in;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
	// Resulting color
	vec3 result = sSkyboxData.vTint;

	// Sample the cubemap, if any
	if (sSkyboxData.bHasTexture == 1.0)
		result *= srgbToLinear(texture(sSkybox, fs_in.vUv).rgb);

	// Apply the background layer mask
	//if (gl_Layer > 0 && (sSkyboxData.bRenderToAllLayers != 1.0 || !depthPeel(sPrevDepthBuffer, fs_in.vPosition, fs_in.vPosition, gl_Layer - 1)))
	if (gl_Layer > 0 && !depthPeel(sPrevDepthBuffer, fs_in.vPosition, fs_in.vPosition, gl_Layer - 1))
		result *= 0.0;

	// Write out the result
	colorBuffer = vec4(result, 1.0);
}