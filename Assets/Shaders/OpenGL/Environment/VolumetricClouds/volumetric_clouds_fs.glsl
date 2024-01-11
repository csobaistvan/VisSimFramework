#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Environment/VolumetricClouds/common.glsl>

// Input attribus
in GeometryData
{
    vec3 vWorldPos;
} fs_in;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{
	//vec3 dir = fs_in.vEyeDir;
	vec3 dir = normalize(fs_in.vWorldPos - sCameraData.vEye);

	vec4 col = vec4(0.0);
	if (dir.y>0.0)
	{
		vec3 camPos = vec3(0.0, g_radius, 0.0);
		vec3 start = camPos + dir * intersectSphere(camPos, dir, sky_b_radius);
		vec3 end = camPos + dir * intersectSphere(camPos, dir, sky_t_radius);
		float shelldist = (length(end-start));
		// Take fewer steps towards horizon
		float steps = (mix(96.0, 54.0, clamp(dot(dir, vec3(0.0, 1.0, 0.0)), 0.0, 1.0)));
		vec3 raystep = dir * shelldist / steps;
		vec4 volume = march(start, end, raystep, int(steps));
		vec3 background = atmosphere(dir);
		// Draw cloud shape
		col = vec4(background*(1.0-volume.a)+volume.xyz, 1.0);
		// Blend distant clouds into the sky
		col.xyz = mix(clamp(col.xyz, vec3(0.0), vec3(1.0)), clamp(background, vec3(0.0), vec3(1.0)), smoothstep(0.6, 1.0, 1.0-dir.y));
	} 
	else
	{
		col = vec4(atmosphere(dir), 1.0);
	}
	
	// Apply the background layer mask
	//if (gl_Layer > 0 && (sVolumetricCloudsData.bRenderToAllLayers != 1.0 || !depthPeel(sPrevDepthBuffer, fs_in.vPosition, fs_in.vPosition, gl_Layer - 1)))
	if (gl_Layer > 0 && !depthPeel(sPrevDepthBuffer, fs_in.vWorldPos, fs_in.vWorldPos, gl_Layer - 1))
		col *= 0.0;

	// Write out the result
	colorBuffer = col;
}