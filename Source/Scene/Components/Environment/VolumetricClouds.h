#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace VolumetricClouds
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "VolumetricClouds";
	static constexpr const char* DISPLAY_NAME = "Volumetric Clouds";
	static constexpr const char* CATEGORY = "Actor";

	////////////////////////////////////////////////////////////////////////////////
	/** A volumetric cloud rendering component. */
	struct VolumetricCloudsComponent
	{
		float m_density = 0.05f;
		float m_cloudCoverage = 0.25f;

		float m_rayleigh = 2.0f;
		glm::vec3 m_rayleighColor{ 0.26f, 0.41f, 0.58f };
		float m_mie = 0.005f;
		float m_mieEccentricity = 0.8f;
		glm::vec3 m_mieColor{ 0.63f, 0.77f, 0.92f };

		float m_turbidity = 10.0f;
		float m_sunEnergyScale = 50.0f;
		float m_sunDiskScale = 1.0f;
		glm::vec3 m_groundColor{ 1.0f };
		float m_exposure = 0.1f;

		// Whether it should be rendered to all layers or not
		bool m_renderToAllLayers;

		// ========= Internal state ===============
		std::string m_sunName;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the volumetric clouds data. */
	struct UniformData
	{
		GLfloat m_renderToAllLayers;
		GLfloat m_density;
		GLfloat m_cloudCoverage;
		GLfloat m_rayleigh;
		GLfloat m_mie;
		GLfloat m_mieEccentricity;
		GLfloat m_turbidity;
		GLfloat m_sunEnergyScale;
		GLfloat m_sunDiskScale;
		GLfloat m_exposure;
		GLfloat m_mainLightEnergy;
		alignas(sizeof(glm::vec4))glm::vec4 m_rayleighColor;
		alignas(sizeof(glm::vec4))glm::vec4 m_mieColor;
		alignas(sizeof(glm::vec4))glm::vec4 m_groundColor;
		alignas(sizeof(glm::vec4))glm::vec4 m_mainLightColor;
		alignas(sizeof(glm::vec4))glm::vec4 m_mainLightDir;
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
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(VOLUMETRTIC_CLOUDS, VolumetricCloudsComponent, VolumetricClouds::VolumetricCloudsComponent)
DECLARE_OBJECT(VOLUMETRTIC_CLOUDS, COMPONENT_ID_VOLUMETRTIC_CLOUDS, COMPONENT_ID_EDITOR_SETTINGS)