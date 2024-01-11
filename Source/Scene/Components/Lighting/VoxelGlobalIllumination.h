#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace VoxelGlobalIllumination
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "VoxelGlobalIllumination";
	static constexpr const char* DISPLAY_NAME = "Voxel Global Illumination";
	static constexpr const char* CATEGORY = "Lighting";
	
	////////////////////////////////////////////////////////////////////////////////
	/** Parameters for a single sample. */
	struct DiffuseSample
	{
		glm::vec3 m_direction;
		float m_weight;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A directional light component. */
	struct VoxelGlobalIlluminationComponent
	{
		// Backface culling mode
		meta_enum(LightContribution, int, Lambertian, BRDF);

		// How fast radiance decays
		float m_radianceDecayRate = 0.0f;

		// How fast the AO term decays
		float m_occlusionDecayRate = 1.0f;

		// Exponent to raise the AO term to
		float m_occlusionExponent = 1.0f;

		// Min AO term value
		float m_minOcclusion = 0.1f;

		// How many indirect bounces to perform
		int m_numDiffuseBounces = 1;

		// Voxel start offset
		float m_diffuseTraceNormalOffset = 0.5f;
		float m_specularTraceNormalOffset = 0.5f;
		float m_diffuseTraceStartOffset = 0.5f;
		float m_specularTraceStartOffset = 0.5f;

		// Maximum light trace distance, in meters
		float m_maxTraceDistance = 10.0f;

		// Diffuse and specular scale factors
		float m_diffuseIntensity = 1.0f;
		float m_specularIntensity = 1.0f;

		// Specular aperture
		float m_diffuseAperture = 60.0f;
		float m_specularApertureMin = 3.5f;
		float m_specularApertureMax = 16.0f;

		// Whether we want to use anisotropic sampling or not
		bool m_anisotropicDiffuse = false;
		bool m_anisotropicSpecular = false;
		
		// Light contribution methods
		LightContribution m_diffuseContribution = BRDF;
		LightContribution m_specularContribution = BRDF;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct UniformDataDiffuseTrace
	{
		GLint m_contribution;
		GLfloat m_intensity;
		GLfloat m_aperture;
		GLfloat m_mipOffset;
		GLfloat m_startOffset;
		GLfloat m_normalOffset;
		GLfloat m_maxTraceDistance;
		GLuint m_anisotropic;
		GLfloat m_radianceDecayRate;
		GLfloat m_occlusionDecayRate;
		GLfloat m_occlusionExponent;
		GLfloat m_minOcclusion;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct UniformDataSpecularTrace
	{
		GLuint m_contribution;
		GLfloat m_intensity;
		GLfloat m_apertureMin;
		GLfloat m_apertureMax;
		GLfloat m_mipLevelMin;
		GLfloat m_mipLevelMax;
		GLfloat m_startOffset;
		GLfloat m_normalOffset;
		GLfloat m_maxTraceDistance;
		GLuint m_anisotropic;
		GLfloat m_radianceDecayRate;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the indirect injection pass. */
	struct UniformDataInjectIndirectPass
	{
		UniformDataDiffuseTrace m_traceParams;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the GI pass. */
	struct UniformDataGIPass
	{
		alignas(sizeof(glm::vec4)) UniformDataDiffuseTrace m_diffuseTraceParams;
		alignas(sizeof(glm::vec4)) UniformDataSpecularTrace m_specularTraceParams;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool injectIndirectLightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void injectIndirectLightingOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool lightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void lightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void lightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void lightingOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(VOXEL_GLOBAL_ILLUMINATION, VoxelGlobalIlluminationComponent, VoxelGlobalIllumination::VoxelGlobalIlluminationComponent)
DECLARE_OBJECT(VOXEL_GLOBAL_ILLUMINATION, COMPONENT_ID_TRANSFORM, COMPONENT_ID_VOXEL_GLOBAL_ILLUMINATION, COMPONENT_ID_EDITOR_SETTINGS)