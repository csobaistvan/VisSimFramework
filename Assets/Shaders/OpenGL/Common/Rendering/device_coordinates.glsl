#include <Shaders/OpenGL/Common/Rendering/camera.glsl>

// convert from [0, 1] to [-1, 1]
vec2 sreenToNdc(const vec2 v)
{
	return v * 2.0 - 1.0;
}

// convert from [0, 1] to [-1, 1]
vec3 sreenToNdc(const vec3 v)
{
	return v * 2.0 - 1.0;
}

// convert from [-1, 1] to [0, 1]
vec2 ndcToScreen(const vec2 v)
{
	return v * 0.5 + 0.5;
}

// convert from [-1, 1] to [0, 1]
vec3 ndcToScreen(const vec3 v)
{
	return v * 0.5 + 0.5;
}

// convert from [0, res - 1] to [0, 1]
vec2 fragmentToScreen(const ivec2 fc, const ivec2 res)
{
	return vec2(fc) / vec2(res - 1);
}

// Convert back from clip-space using the parameter inverse matrix
vec3 reconstructPosition(const vec2 v, const float d, const mat4 im)
{
    const vec4 p = im * vec4(sreenToNdc(vec3(v, d)), 1.0);
	return p.xyz / p.w;
}

// Convert from clip-space to world space
vec3 reconstructPositionWS(const vec2 v, const float d)
{
    return reconstructPosition(v, d, sCameraData.mInverseViewProjection);
}

// Convert from clip-space to camera space
vec3 reconstructPositionCS(const vec2 v, const float d)
{
    return reconstructPosition(v, d, sCameraData.mInverseProjection);
}

// Convert back from clip-space using the parameter inverse matrix
vec3 reconstructPosition(const ivec2 v, const ivec2 res, const float d, const mat4 im)
{
	return reconstructPosition(fragmentToScreen(v, res), d, im);
}

// Convert from clip-space to world space
vec3 reconstructPositionWS(const ivec2 v, const ivec2 res, const float d)
{
	return reconstructPositionWS(fragmentToScreen(v, res), d);
}

// Convert from clip-space to camera space
vec3 reconstructPositionCS(const ivec2 v, const ivec2 res, const float d)
{
	return reconstructPositionCS(fragmentToScreen(v, res), d);
}

// Returns the spherical coordinates of the input screen position
// Inputs:
// - screenPos: screen-space coordinates [0, resolution - 1]
// - depth: corresponding depth buffer value
//
// Returns:
// - result[0:1]: horizontal and vertical angles, in radians
// - result[2]:   distance, in meters
vec3 screenToSphericalCoordinates(const ivec2 screenPos, const ivec2 resolution, const float depth)
{
	const vec3 camPos = reconstructPositionCS(screenPos, resolution, depth);
	const vec2 angles = vec2(atan2(camPos.x, -camPos.z), atan2(camPos.y, length(camPos.zx))); // azimuth and elevation terms
	const float radius = meters(length(camPos));
	return vec3(angles, radius);
}

// Returns the spherical coordinates of the input screen position
vec3 screenToSphericalCoordinates(const ivec2 screenPos, const float depth)
{
    return screenToSphericalCoordinates(screenPos, ivec2(sRenderData.vResolution), depth);
}