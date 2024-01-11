
#include <Shaders/OpenGL/Common/Rendering/camera.glsl>

// Computes the umbra size, in meters
float computeUmbra(float zM, float fov, float numPixels, float lensRadiusM, float pixSizeMultiplier)
{
    const float pixelSizeM = pixSizeMultiplier * 0.5 * pixelSize(zM, fov, numPixels);
	return ((zM * pixelSizeM) / (lensRadiusM - pixelSizeM));
}

// Computes the extended umbra size, in meters
float computeExtendedUmbra(float zM, float fov, float numPixels, float lensRadiusM)
{
	return computeUmbra(zM, fov, numPixels, lensRadiusM, 2.0);
}

// Computes the umbra size, in meters
float computeExtendedUmbra(float zM, ivec2 resolution)
{
	return computeExtendedUmbra(zM, sCameraData.vFov.y, resolution.y, getApertureRadiusM());
}

// Computes the umbra size, in meters
float computeExtendedUmbra(float zM)
{
	return computeExtendedUmbra(zM, sCameraData.vFov.y, sRenderData.vResolution.y, getApertureRadiusM());
}

// Computes the umbra size, in meters
float computeUmbra(float zM, float fov, float numPixels, float lensRadiusM)
{
	return computeUmbra(zM, fov, numPixels, lensRadiusM, 1.0);
}

// Computes the umbra size, in meters
float computeUmbra(float zM, ivec2 resolution)
{
	return computeUmbra(zM, sCameraData.vFov.y, resolution.y, getApertureRadiusM());
}

// Computes the umbra size, in meters
float computeUmbra(float zM)
{
	return computeUmbra(zM, sCameraData.vFov.y, sRenderData.vResolution.y, getApertureRadiusM());
}