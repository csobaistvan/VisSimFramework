#include <Shaders/OpenGL/Generated/resource_indices.glsl>

////////////////////////////////////////////////////////////////////////////////
// Render settings uniform buffer
layout (std140, binding = UNIFORM_BUFFER_RENDER) buffer RenderSettingsData
{
	vec2 vResolution;
	vec2 vMaxResolution;
	vec2 vUvScale;
	vec3 vWorldMin;
	vec3 vWorldMax;
	uint uiDepthPeelAlgorithm;
	uint uiBrdfModel;
	uint uiLightingTextureFiltering;
	uint uiVoxelDilationMode;
	uint uiVoxelShadingMode;
	float fDeltaTime;
	float fGlobalTime;
	float fScaledDeltaTime;
	float fScaledGlobalTime;
	float fMetersPerUnit;
	float fNitsPerUnit;
	int iMSAA;
	int iLayers;
	float fLayerDepthGap;
	float fUmbraScaling;
	float fUmbraMin;
	float fUmbraMax;
	float fSpecularPowerMin;
	float fSpecularPowerMax;
	float fGamma;
	uint uiNumVoxels;
	float fVoxelExtents;
	float fVoxelSize;
} sRenderData;

////////////////////////////////////////////////////////////////////////////////
// Converts from world units to meters
float meters(float u)
{
    return u * sRenderData.fMetersPerUnit;
}

// Converts wrom world units to millimeters
float millimeters(float u)
{
    return (u * sRenderData.fMetersPerUnit) * 1000.0;
}

// Converts from meters to world units
float units(float m)
{
    return m / sRenderData.fMetersPerUnit;
}