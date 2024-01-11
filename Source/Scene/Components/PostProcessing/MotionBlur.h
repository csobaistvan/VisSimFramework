#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace MotionBlur
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "MotionBlur";
	static constexpr const char* DISPLAY_NAME = "Motion Blur";
	static constexpr const char* CATEGORY = "Post Processing";

    ////////////////////////////////////////////////////////////////////////////////
	/** A motion blur component. */
	struct MotionBlurComponent
	{
		// Velocity scaling
		float m_velocityScaleFactor = 1.0f;

		// Maximum allowed velocity
		float m_maxVelocity = 1.0f;

		// Number of taps to take
		int m_numTaps = 32;

		// Size of the tile buffer
		int m_tileSize = 40;

		// Weight of the center sample
		float m_centerWeight = 0.5f;

		// Tile falloff factor
		float m_tileFalloff = 1.0f;

		// Interpolation threshold
		float m_interpolationThreshold = 1.5f;

		// Jitter scale factor
		float m_jitterScale = 0.95f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer layout. */
	struct UniformData
	{
		GLfloat m_velocityScaleFactor;
		GLfloat m_maxVelocity;
		GLint m_numTaps;
		GLint m_tileSize;
		GLfloat m_centerWeight;
		GLfloat m_tileFalloff;
		GLfloat m_interpolationThreshold;
		GLfloat m_jitterScale;
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
DECLARE_COMPONENT(MOTION_BLUR, MotionBlurComponent, MotionBlur::MotionBlurComponent)
DECLARE_OBJECT(MOTION_BLUR, COMPONENT_ID_MOTION_BLUR, COMPONENT_ID_EDITOR_SETTINGS)