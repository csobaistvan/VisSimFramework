#include "PCH.h"
#include "Mover.h"

namespace Mover
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(MOVER);
	DEFINE_OBJECT(MOVER);
	REGISTER_OBJECT_UPDATE_CALLBACK(MOVER, BEFORE, ACTOR);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		object->component<Mover::MoverComponent>().m_currentOffset += simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledDeltaTime * object->component<Mover::MoverComponent>().m_moveSpeed;

		static const glm::vec3 AXES[] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };

		glm::vec3 center = scene.m_objects[object->component<Mover::MoverComponent>().m_center].component<Transform::TransformComponent>().m_position;
		glm::vec3 offset = object->component<Mover::MoverComponent>().m_moveRadius * AXES[object->component<Mover::MoverComponent>().m_moveAxis] * glm::sin(object->component<Mover::MoverComponent>().m_currentOffset);

		scene.m_objects[object->component<Mover::MoverComponent>().m_object].component<Transform::TransformComponent>().m_position = center + offset;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::InputText("Center Object", object->component<Mover::MoverComponent>().m_center, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::InputText("Moved Object", object->component<Mover::MoverComponent>().m_object, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::Combo("Move Axis", &object->component<Mover::MoverComponent>().m_moveAxis, MoverComponent::MoveAxis_meta);
		ImGui::SliderAngle("Move Speed", &object->component<Mover::MoverComponent>().m_moveSpeed, 0.0f, 360.0f);
		ImGui::SliderFloat("Move Radius", &object->component<Mover::MoverComponent>().m_moveRadius, 0.0f, 100000.0f);
	}
}