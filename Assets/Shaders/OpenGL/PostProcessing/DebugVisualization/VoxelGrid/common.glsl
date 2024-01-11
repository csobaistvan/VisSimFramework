#include <Shaders/OpenGL/Shading/Voxel/common.glsl>
#include <Shaders/OpenGL/Shading/Voxel/sample_grid.glsl>
#include <Shaders/OpenGL/PostProcessing/DebugVisualization/common.glsl>

// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform DebugVisualization
{
	int iComponent;
    int iLodLevel;
    int iVoxelFace;
    float fDisplayPower;
} sDebugVisualizationData;