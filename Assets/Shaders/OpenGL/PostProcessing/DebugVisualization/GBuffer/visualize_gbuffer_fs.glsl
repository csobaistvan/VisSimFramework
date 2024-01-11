#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/DebugVisualization/GBuffer/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
	// Container for the result
	vec3 result = vec3(0.0);
	for (int i = 0; i < max(1, sRenderData.iMSAA); ++i)
	{
		// Depth component
		if (sDebugVisualizationData.iComponent == BufferComponent_Depth)
		{
			result += vec3(gbufferLinearDepth(vUv, sDebugVisualizationData.iLayer, i));
		}

		// Color component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Albedo)
		{
			result += gbufferAlbedo(vUv, sDebugVisualizationData.iLayer, i);
		}
		
		// Revealage component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Occlusion)
		{
			result += gbufferRevealage(vUv, sDebugVisualizationData.iLayer, i);
		}
		
		// Normal component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Normal)
		{
			result += visualizeNormal(gbufferNormal(vUv, sDebugVisualizationData.iLayer, i));
		}
		
		// Metallic component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Metallic)
		{
			result += vec3(gbufferMetallic(vUv, sDebugVisualizationData.iLayer, i));
		}
		
		// Roughness component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Roughness)
		{
			result += vec3(gbufferRoughness(vUv, sDebugVisualizationData.iLayer, i));
		}
		
		// Specular component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Specular)
		{
			result += vec3(gbufferSpecular(vUv, sDebugVisualizationData.iLayer, i));
		}
		
		// Velocity component
		else if (sDebugVisualizationData.iComponent == BufferComponent_Velocity)
		{
			result += vec3(gbufferVelocity(vUv, sDebugVisualizationData.iLayer, i), 0.0);
		}
	}
    
    // Write out the result
    colorBuffer.rgb = pow(result / float(max(1, sRenderData.iMSAA)), vec3(sDebugVisualizationData.fDisplayPower));
    colorBuffer.a = 1.0;
}