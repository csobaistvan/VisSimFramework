#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace Fxaa
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "FXAA";
	static constexpr const char* DISPLAY_NAME = "FXAA";
	static constexpr const char* CATEGORY = "Post Processing";

    ////////////////////////////////////////////////////////////////////////////////
	/** An FXAA component. */
	struct FxaaComponent
	{
		// Input color space
		meta_enum(ColorSpace, int, HDR, LDR);

		// Domain of computation
		meta_enum(ComputationDomain, int, AfterLighting, AfterPostprocessing);

		// Color space of operation
		ColorSpace m_colorSpace;

		// When to apply the FXAA
		ComputationDomain m_domain;

		// Minimum direction reduce.
		float m_dirReduceMin;

		// Direction reduction multiplier
		float m_dirReduceMultiplier;

		// Maximum blurring.
		float m_maxBlur;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the fxaa data. */
	struct UniformData
	{
		GLfloat m_dirReduceMin;
		GLfloat m_dirReduceMultiplier;
		GLfloat m_maxBlur;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostLightingHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostLightingLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostPostprocessingHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostPostprocessingLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(FXAA, FxaaComponent, Fxaa::FxaaComponent)
DECLARE_OBJECT(FXAA, COMPONENT_ID_FXAA, COMPONENT_ID_EDITOR_SETTINGS)