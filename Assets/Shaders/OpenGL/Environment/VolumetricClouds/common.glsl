
#include <Shaders/OpenGL/Common/Optional/depth_peel.glsl>

// Based on: https://github.com/clayjohn/godot-volumetric-cloud-demo

////////////////////////////////////////////////////////////////////////////////
// Uniforms
////////////////////////////////////////////////////////////////////////////////

// Uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform SkyboxData
{
    float bRenderToAllLayers;
    float fDensity;
    float fCloudCoverage;
    float fRayleigh;
    float fMie;
    float fMieEccentricity;
    float fTurbidity;
    float fSunEnergyScale;
    float fSunDiskScale;
    float fExposure;
    float fMainLightEnergy;
    vec4 vRayleighColor;
    vec4 vMieColor;
    vec4 vGroundColor;
    vec4 vMainLightColor;
    vec4 vMainLightDir;
} sVolumetricCloudsData;

////////////////////////////////////////////////////////////////////////////////
// Textures and images
////////////////////////////////////////////////////////////////////////////////

// The necessary noise and weather maps
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler3D sWorlNoise;
layout (binding = TEXTURE_POST_PROCESS_2) uniform sampler3D sPerlWorlNoise;
layout (binding = TEXTURE_POST_PROCESS_3) uniform sampler2D sWeatherMap;

// The previous depth texture
layout (binding = TEXTURE_DEPTH) uniform sampler2DArray sPrevDepthBuffer;

////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////

// Approximately earth sizes
const float g_radius = 6000000.0; //ground radius
const float sky_b_radius = 6001500.0;//bottom of cloud layer
const float sky_t_radius = 6004000.0;//top of cloud layer

// Up vector
const vec3 UP = vec3( 0.0, 1.0, 0.0 );

// Sun constants
const float SOL_SIZE = 0.00872663806;

// optical length at zenith for molecules
const float rayleigh_zenith_size = 8.4e3;
const float mie_zenith_size = 1.25e3;

////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////

// From: https://www.shadertoy.com/view/4sfGzS credit to iq
float hash(vec3 p)
{
	p  = fract( p * 0.3183099 + 0.1 );
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Utility function that maps a value from one range to another. 
float remap(float originalValue,  float originalMin,  float originalMax,  float newMin,  float newMax)
{
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

// Phase function
float henyey_greenstein(float cos_theta, float g)
{
	const float k = 0.0795774715459;
	return k * (1.0 - g * g) / (pow(1.0 + g * g - 2.0 * g * cos_theta, 1.5));
}

// Simple Analytic sky. In a real project you should use a texture
vec3 atmosphere(vec3 eye_dir)
{
	float zenith_angle = clamp( dot(UP, normalize(sVolumetricCloudsData.vMainLightDir.xyz)), -1.0, 1.0 );
	float sun_energy = max(0.0, 1.0 - exp(-((PI * 0.5) - acos(zenith_angle)))) * sVolumetricCloudsData.fSunEnergyScale * sVolumetricCloudsData.fMainLightEnergy;
	float sun_fade = 1.0 - clamp(1.0 - exp(sVolumetricCloudsData.vMainLightDir.y), 0.0, 1.0);

	// Rayleigh coefficients.
	float rayleigh_coefficient = sVolumetricCloudsData.fRayleigh - ( 1.0 * ( 1.0 - sun_fade ) );
	vec3 rayleigh_beta = rayleigh_coefficient * sVolumetricCloudsData.vRayleighColor.rgb * 0.0001;
	// mie coefficients from Preetham
	vec3 mie_beta = sVolumetricCloudsData.fTurbidity * sVolumetricCloudsData.fMie * sVolumetricCloudsData.vMieColor.rgb * 0.000434;

	// optical length
	float zenith = acos(max(0.0, dot(UP, eye_dir)));
	float optical_mass = 1.0 / (cos(zenith) + 0.15 * pow(93.885 - degrees(zenith), -1.253));
	float rayleigh_scatter = rayleigh_zenith_size * optical_mass;
	float mie_scatter = mie_zenith_size * optical_mass;

	// light extinction based on thickness of atmosphere
	vec3 extinction = exp(-(rayleigh_beta * rayleigh_scatter + mie_beta * mie_scatter));

	// in scattering
	float cos_theta = dot(eye_dir, normalize(sVolumetricCloudsData.vMainLightDir.xyz));

	float rayleigh_phase = (3.0 / (16.0 * PI)) * (1.0 + pow(cos_theta * 0.5 + 0.5, 2.0));
	vec3 betaRTheta = rayleigh_beta * rayleigh_phase;

	float mie_phase = henyey_greenstein(cos_theta, sVolumetricCloudsData.fMieEccentricity);
	vec3 betaMTheta = mie_beta * mie_phase;

	vec3 Lin = pow(sun_energy * ((betaRTheta + betaMTheta) / (rayleigh_beta + mie_beta)) * (1.0 - extinction), vec3(1.5));
	// Hack from https://github.com/mrdoob/three.js/blob/master/examples/jsm/objects/Sky.js
	Lin *= mix(vec3(1.0), pow(sun_energy * ((betaRTheta + betaMTheta) / (rayleigh_beta + mie_beta)) * extinction, vec3(0.5)), clamp(pow(1.0 - zenith_angle, 5.0), 0.0, 1.0));

	// Hack in the ground color
	Lin  *= mix(sVolumetricCloudsData.vGroundColor.rgb, vec3(1.0), smoothstep(-0.1, 0.1, dot(UP, eye_dir)));

	// Solar disk and out-scattering
	float sunAngularDiameterCos = cos(SOL_SIZE * sVolumetricCloudsData.fSunDiskScale);
	float sunAngularDiameterCos2 = cos(SOL_SIZE * sVolumetricCloudsData.fSunDiskScale*0.5);
	float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos2, cos_theta);
	vec3 L0 = (sun_energy * 1900.0 * extinction) * sundisk * sVolumetricCloudsData.vMainLightColor.rgb;
	// Note: Add nightime here: L0 += night_sky * extinction

	vec3 color = (Lin + L0) * 0.04;
	color = pow(color, vec3(1.0 / (1.2 + (1.2 * sun_fade))));
	color *= sVolumetricCloudsData.fExposure;
	return color;
}

float GetHeightFractionForPoint(float inPosition)
{ 
	float height_fraction = (inPosition -  sky_b_radius) / (sky_t_radius - sky_b_radius); 
	return clamp(height_fraction, 0.0, 1.0);
}

vec4 mixGradients(float cloudType)
{
	const vec4 STRATUS_GRADIENT = vec4(0.02f, 0.05f, 0.09f, 0.11f);
	const vec4 STRATOCUMULUS_GRADIENT = vec4(0.02f, 0.2f, 0.48f, 0.625f);
	const vec4 CUMULUS_GRADIENT = vec4(0.01f, 0.0625f, 0.78f, 1.0f);
	float stratus = 1.0f - clamp(cloudType * 2.0f, 0.0, 1.0);
	float stratocumulus = 1.0f - abs(cloudType - 0.5f) * 2.0f;
	float cumulus = clamp(cloudType - 0.5f, 0.0, 1.0) * 2.0f;
	return STRATUS_GRADIENT * stratus + STRATOCUMULUS_GRADIENT * stratocumulus + CUMULUS_GRADIENT * cumulus;
}

float densityHeightGradient(float heightFrac, float cloudType)
{
	vec4 cloudGradient = mixGradients(cloudType);
	return smoothstep(cloudGradient.x, cloudGradient.y, heightFrac) - smoothstep(cloudGradient.z, cloudGradient.w, heightFrac);
}

float intersectSphere(vec3 pos, vec3 dir,float r) 
{
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, pos);
    float c = dot(pos, pos) - (r * r);
	float d = sqrt((b*b) - 4.0*a*c);
	float p = -b - d;
	float p2 = -b + d;
    return max(p, p2) / (2.0 * a);
}

// Returns density at a given point
// Heavily based on method from Schneider
float density(vec3 pip, vec3 weather, float mip)
{
	float time = mod(sRenderData.fScaledGlobalTime, 100.0);
	vec3 p = pip;
	p.x += time * 10.0;
	float height_fraction = GetHeightFractionForPoint(length(p));
	vec4 n = textureLod(sPerlWorlNoise, p.xyz*0.00008, mip-2.0);
	float fbm = n.g*0.625+n.b*0.25+n.a*0.125;
	float g = densityHeightGradient(height_fraction, weather.r);
	float base_cloud = remap(n.r, -(1.0-fbm), 1.0, 0.0, 1.0);
	float weather_coverage = sVolumetricCloudsData.fCloudCoverage*weather.b;
	base_cloud = remap(base_cloud*g, 1.0-(weather_coverage), 1.0, 0.0, 1.0);
	base_cloud *= weather_coverage;
	p.xy -= time * 20.0;
	vec3 hn = textureLod(sWorlNoise, p*0.001, mip).rgb;
	float hfbm = hn.r*0.625+hn.g*0.25+hn.b*0.125;
	hfbm = mix(hfbm, 1.0-hfbm, clamp(height_fraction*4.0, 0.0, 1.0));
	base_cloud = remap(base_cloud, hfbm*0.4 * height_fraction, 1.0, 0.0, 1.0);
	return pow(clamp(base_cloud, 0.0, 1.0), (1.0 - height_fraction) * 0.8 + 0.5);
}

vec4 march(vec3 pos,  vec3 end, vec3 dir, int depth) 
{
	const vec3 RANDOM_VECTORS[6] = {vec3( 0.38051305f,  0.92453449f, -0.02111345f),vec3(-0.50625799f, -0.03590792f, -0.86163418f),vec3(-0.32509218f, -0.94557439f,  0.01428793f),vec3( 0.09026238f, -0.27376545f,  0.95755165f),vec3( 0.28128598f,  0.42443639f, -0.86065785f),vec3(-0.16852403f,  0.14748697f,  0.97460106f)};
	float T = 1.0;
	float alpha = 0.0;
	float ss = length(dir);
	dir = normalize(dir);
	vec3 p = pos + hash(pos * 10.0) * ss;
	const float t_dist = sky_t_radius-sky_b_radius;
	float lss = (t_dist / 36.0);
	vec3 ldir = normalize(sVolumetricCloudsData.vMainLightDir.xyz);
	vec3 L = vec3(0.0);
	int count=0;
	float t = 1.0;
	float costheta = dot(ldir, dir);
	// Stack multiple phase functions to emulate some backscattering
	float phase = max(max(henyey_greenstein(costheta, 0.6), henyey_greenstein(costheta, (0.4 - 1.4 * ldir.y))), henyey_greenstein(costheta, -0.2));
	// Precalculate sun and ambient colors
	// This should really come from a uniform or texture for performance reasons
	vec3 atmosphere_sun = atmosphere(sVolumetricCloudsData.vMainLightDir.xyz) * sVolumetricCloudsData.fMainLightEnergy * ss * 0.1;
	vec3 atmosphere_ambient = atmosphere(normalize(vec3(1.0, 1.0, 0.0)));
	vec3 atmosphere_ground = atmosphere(normalize(vec3(1.0, -1.0, 0.0)));
	
	const float weather_scale = 0.00006;
	
	for (int i = 0; i < depth; i++) {
		p += dir * ss;
		vec3 weather_sample = texture(sWeatherMap, p.xz * weather_scale + 0.5).xyz;
		float height_fraction = GetHeightFractionForPoint(length(p));

		t = density(p, weather_sample, 0.0);
		float dt = exp(-sVolumetricCloudsData.fDensity*t*ss);
		T *= dt;
		vec3 lp = p;
		float lt = 1.0;
		float cd = 0.0;

		if (t > 0.0) { //calculate lighting, but only when we are in the cloud
			float lheight_fraction = 0.0;
			for (int j = 0; j < 6; j++) {
				lp += (ldir + RANDOM_VECTORS[j]*float(j))*lss;
				lheight_fraction = GetHeightFractionForPoint(length(lp));
				vec3 lweather = texture(sWeatherMap, lp.xz * weather_scale + 0.5).xyz;
				lt = density(lp, lweather, float(j));
				cd += lt;
			}
			
			// Take a single distant sample
			lp = p + ldir * 18.0 * lss;
			lheight_fraction = GetHeightFractionForPoint(length(lp));
			vec3 lweather = texture(sWeatherMap, lp.xz * weather_scale + 0.5).xyz;
			lt = pow(density(lp, lweather, 5.0), (1.0 - lheight_fraction) * 0.8 + 0.5);
			cd += lt;
			
			// captures the direct lighting from the sun
			float beers = exp(-sVolumetricCloudsData.fDensity * cd * lss);
			float beers2 = exp(-sVolumetricCloudsData.fDensity * cd * lss * 0.25) * 0.7;
			float beers_total = max(beers, beers2);

			vec3 ambient = mix(atmosphere_ground, vec3(1.0), smoothstep(0.0, 1.0, height_fraction)) * sVolumetricCloudsData.fDensity * mix(atmosphere_ambient, vec3(1.0), 0.4) * (sVolumetricCloudsData.vMainLightDir.y);
			alpha += (1.0 - dt) * (1.0 - alpha);
			L += (ambient + beers_total * atmosphere_sun * phase * alpha) * T * t;
		}
	}
	return clamp(vec4(L, alpha), 0.0, 1.0);
}