#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace Rotator
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "Rotator";
	static constexpr const char* DISPLAY_NAME = "Rotator";
	static constexpr const char* CATEGORY = "Animation";

	////////////////////////////////////////////////////////////////////////////////
	/** Rotator component. */
	struct RotatorComponent
	{
		// Axes of rotation available
		meta_enum(RotationAxis, int, X, Y, Z);

		// Center object
		std::string m_center;

		// Object to move
		std::string m_object;

		// Axis to rotate about
		RotationAxis m_rotationAxis = X;

		// Speed of rotation
		float m_rotationSpeed = glm::radians(45.0f);

		// Radius of rotation
		float m_rotationRadius = 2000.0f;

		// ---- Private members

		// Current offset value
		float m_currentRotation = 0.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(ROTATOR, RotatorComponent, Rotator::RotatorComponent)
DECLARE_OBJECT(ROTATOR, COMPONENT_ID_ROTATOR, COMPONENT_ID_EDITOR_SETTINGS)