#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace SkyBox
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "Skybox";
	static constexpr const char* DISPLAY_NAME = "Skybox";
	static constexpr const char* CATEGORY = "Actor";

	////////////////////////////////////////////////////////////////////////////////
	/** A skybox component. */
	struct SkyBoxComponent
	{
		// Tint color (or background color
		glm::vec3 m_tint{ 1.0f };

		// Name of the cube map texture.
		std::string m_textureName;

		// Whether it should be rendered to all layers or not
		bool m_renderToAllLayers;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for skybox data. */
	struct UniformData
	{
		glm::vec3 m_color;
		GLfloat m_hasTexture;
		GLfloat m_renderToAllLayers;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(SKYBOX, SkyBoxComponent, SkyBox::SkyBoxComponent)
DECLARE_OBJECT(SKYBOX, COMPONENT_ID_SKYBOX, COMPONENT_ID_EDITOR_SETTINGS)