#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "ShadowMap.h"

namespace DirectionalLight
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "DirectionalLight";
	static constexpr const char* DISPLAY_NAME = "Directional Light";
	static constexpr const char* CATEGORY = "Lighting";

	////////////////////////////////////////////////////////////////////////////////
	// Number of light sources to render per batch
	static const size_t LIGHT_SOURCES_PER_BATCH = 32;

	////////////////////////////////////////////////////////////////////////////////
	/** A directional light component. */
	struct DirectionalLightComponent
	{
		// Light color.
		glm::vec3 m_color{ 1.0f };

		// Ambient intensity.
		float m_ambientIntensity = 0.0f;

		// Diffuse intensity.
		float m_diffuseIntensity = 1.0f;

		// Specular intensity.
		float m_specularItensity = 1.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the directional light data. */
	struct UniformData
	{
		glm::vec3 m_direction;
		alignas(sizeof(glm::vec4)) glm::vec3 m_color;
		alignas(sizeof(glm::vec4)) GLfloat m_ambientIntensity;
		GLfloat m_diffuseIntensity;
		GLfloat m_specularIntensity;
		GLfloat m_castsShadow;
		GLuint m_shadowAlgorithm;
		GLuint m_shadowPrecision;
		GLfloat m_shadowDepthBias;
		GLfloat m_shadowMinVariance;
		GLfloat m_shadowLightBleedBias;
		GLfloat m_shadowMomentsBias;
		alignas(sizeof(glm::vec2)) glm::vec2 m_shadowExponentialConstants;
		alignas(sizeof(glm::vec4)) glm::mat4 m_lightSpaceTransform;
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
	void updateShadowMapSlices(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMapTexture(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool voxelLightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingOpengl(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

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
DECLARE_COMPONENT(DIRECTIONAL_LIGHT, DirectionalLightComponent, DirectionalLight::DirectionalLightComponent)
DECLARE_OBJECT(DIRECTIONAL_LIGHT, COMPONENT_ID_TRANSFORM, COMPONENT_ID_DIRECTIONAL_LIGHT, COMPONENT_ID_SHADOW_MAP, COMPONENT_ID_EDITOR_SETTINGS)