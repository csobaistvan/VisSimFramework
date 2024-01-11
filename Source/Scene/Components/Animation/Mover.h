#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace Mover
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "Mover";
	static constexpr const char* DISPLAY_NAME = "Mover";
	static constexpr const char* CATEGORY = "Animation";

	////////////////////////////////////////////////////////////////////////////////
	/** Mover component. */
	struct MoverComponent
	{
		// Axes of move available
		meta_enum(MoveAxis, int, X, Y, Z);

		// Center object
		std::string m_center;

		// Object to move
		std::string m_object;

		// Axis to move on
		MoveAxis m_moveAxis = X;

		// Speed of movement
		float m_moveSpeed = glm::radians(45.0f);

		// Radius of movement
		float m_moveRadius = 2000.0f;

		// ---- Private members

		// Current offset value
		float m_currentOffset = 0.0f;
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
DECLARE_COMPONENT(MOVER, MoverComponent, Mover::MoverComponent)
DECLARE_OBJECT(MOVER, COMPONENT_ID_MOVER, COMPONENT_ID_EDITOR_SETTINGS)