#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/DebugVisualization/VoxelGrid/common.glsl>

in vec4 voxelColor;

out vec4 fragColor;

void main()
{
	fragColor.rgb = pow(voxelColor.rgb, vec3(sDebugVisualizationData.fDisplayPower));
	fragColor.a = 1.0;
}