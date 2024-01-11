#include "PCH.h"
#include "Rotator.h"

namespace Rotator
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(ROTATOR);
	DEFINE_OBJECT(ROTATOR);
	REGISTER_OBJECT_UPDATE_CALLBACK(ROTATOR, BEFORE, ACTOR);

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
		object->component<Rotator::RotatorComponent>().m_currentRotation += simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledDeltaTime * object->component<Rotator::RotatorComponent>().m_rotationSpeed;

		static const glm::vec3 AXES[] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
		static const glm::vec3 FORWARDS[] = { glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };

		glm::vec3 axis = AXES[object->component<Rotator::RotatorComponent>().m_rotationAxis];
		glm::vec3 forward = FORWARDS[object->component<Rotator::RotatorComponent>().m_rotationAxis];
		glm::mat4 rotation = glm::rotate(object->component<Rotator::RotatorComponent>().m_currentRotation, axis);
		glm::vec3 center = scene.m_objects[object->component<Rotator::RotatorComponent>().m_center].component<Transform::TransformComponent>().m_position;
		glm::vec3 offset = object->component<Rotator::RotatorComponent>().m_rotationRadius * glm::vec3(rotation * glm::vec4(forward, 1.0));

		scene.m_objects[object->component<Rotator::RotatorComponent>().m_object].component<Transform::TransformComponent>().m_position = center + offset;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::InputText("Center Object", object->component<Rotator::RotatorComponent>().m_center, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::InputText("Rotated Object", object->component<Rotator::RotatorComponent>().m_object, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::Combo("Rotation Axis", &object->component<Rotator::RotatorComponent>().m_rotationAxis, RotatorComponent::RotationAxis_meta);
		ImGui::SliderAngle("Rotation Speed", &object->component<Rotator::RotatorComponent>().m_rotationSpeed, 0.0f, 360.0f);
		ImGui::SliderFloat("Rotation Radius", &object->component<Rotator::RotatorComponent>().m_rotationRadius, 0.0f, 100000.0f);
	}
}