#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>
#include <Shaders/OpenGL/PostProcessing/DebugVisualization/common.glsl>

// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform DebugVisualization
{
	int iComponent;
    int iLayer;
    int iLodLevel;
    float fDisplayPower;
} sDebugVisualizationData;