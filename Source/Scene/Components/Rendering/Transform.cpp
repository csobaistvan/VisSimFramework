#include "PCH.h"
#include "Transform.h"

namespace Transform
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(TRANSFORM);
	DEFINE_OBJECT(ACTOR);
	REGISTER_OBJECT_UPDATE_CALLBACK(ACTOR, BEFORE, INPUT);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Check if the object moved since the last frame or not
		object->component<Transform::TransformComponent>().m_transformChanged = false;
		object->component<Transform::TransformComponent>().m_transformChanged |= object->component<Transform::TransformComponent>().m_prevPosition != object->component<Transform::TransformComponent>().m_position;
		object->component<Transform::TransformComponent>().m_transformChanged |= object->component<Transform::TransformComponent>().m_prevOrientation != object->component<Transform::TransformComponent>().m_orientation;
		object->component<Transform::TransformComponent>().m_transformChanged |= object->component<Transform::TransformComponent>().m_prevScale != object->component<Transform::TransformComponent>().m_scale;

		// Store the old position
		object->component<Transform::TransformComponent>().m_prevPosition = object->component<Transform::TransformComponent>().m_position;
		object->component<Transform::TransformComponent>().m_prevOrientation = object->component<Transform::TransformComponent>().m_orientation;
		object->component<Transform::TransformComponent>().m_prevScale = object->component<Transform::TransformComponent>().m_scale;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		bool transformChanged = false;
		transformChanged |= ImGui::DragFloat3("Position", glm::value_ptr(object->component<Transform::TransformComponent>().m_position), 10.0f);
		float scale = object->component<Transform::TransformComponent>().m_scale.x;
		if (ImGui::DragFloat("Scale", &scale, 0.01f))
		{
			object->component<Transform::TransformComponent>().m_scale = glm::vec3(scale);
			transformChanged = true;
		}
		ImGui::PushID("Individual");
		transformChanged |= ImGui::DragFloat3("Scale", glm::value_ptr(object->component<Transform::TransformComponent>().m_scale), 0.01f);
		ImGui::PopID();
		transformChanged |= ImGui::DragFloatAngle3("Rotation", glm::value_ptr(object->component<Transform::TransformComponent>().m_orientation), 0.1f);
		return transformChanged;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getRotationMatrix(glm::vec3 orientation)
	{
		// Compute the pitch-yaw-roll quaternions
		const glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
		const glm::quat pitch = glm::normalize(glm::angleAxis(orientation.x, right));
		const glm::vec3 up = glm::normalize(pitch * glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::quat yaw = glm::normalize(glm::angleAxis(orientation.y, up));
		const glm::vec3 forward = glm::normalize(yaw * pitch * glm::vec3(0.0f, 0.0f, -1.0f));
		const glm::quat roll = glm::normalize(glm::angleAxis(orientation.z, forward));

		// Multiply together to obtain the result
		return glm::mat4_cast(roll * yaw * pitch);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getModelMatrix(glm::vec3 pos, glm::vec3 orientation, glm::vec3 scale)
	{
		return glm::translate(pos) *
			glm::rotate(orientation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(orientation.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(orientation.z, glm::vec3(0.0f, 0.0f, 1.0f)) *
			glm::scale(scale);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 lookAt(Scene::Object* object, glm::vec3 at)
	{
		glm::vec3 direction = at - object->component<TransformComponent>().m_position;

		glm::vec3 result(0.0f);
		result.x = glm::atan(direction.y, glm::sqrt(direction.x * direction.x + direction.z * direction.z));
		result.y = -(glm::atan(direction.z, direction.x) + glm::half_pi<float>());
		result.z = 0.0f;
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getModelMatrix(Scene::Object* object)
	{
		return getModelMatrix(
			object->component<Transform::TransformComponent>().m_position, 
			object->component<Transform::TransformComponent>().m_orientation, 
			object->component<Transform::TransformComponent>().m_scale);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevModelMatrix(Scene::Object* object)
	{
		return getModelMatrix(
			object->component<Transform::TransformComponent>().m_prevPosition, 
			object->component<Transform::TransformComponent>().m_prevOrientation, 
			object->component<Transform::TransformComponent>().m_prevScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getNormalMatrix(glm::mat4 const& matrix)
	{
		return glm::inverseTranspose(matrix);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getNormalMatrix(Scene::Object* object)
	{
		return getNormalMatrix(getModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevNormalMatrix(Scene::Object* object)
	{
		return getNormalMatrix(getPrevModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getForwardVector(glm::mat4 const& matrix)
	{
		return glm::normalize(glm::vec3(matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getRightVector(glm::mat4 const& matrix)
	{
		return glm::normalize(glm::vec3(matrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getUpVector(glm::mat4 const& matrix)
	{
		return glm::normalize(glm::vec3(matrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getForwardVector(Scene::Object* object)
	{
		return getForwardVector(Transform::getModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getRightVector(Scene::Object* object)
	{
		return getRightVector(Transform::getModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getUpVector(Scene::Object* object)
	{
		return getUpVector(Transform::getModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevForwardVector(Scene::Object* object)
	{
		return getForwardVector(Transform::getPrevModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevRightVector(Scene::Object* object)
	{
		return getRightVector(Transform::getPrevModelMatrix(object));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevUpVector(Scene::Object* object)
	{
		return getUpVector(Transform::getPrevModelMatrix(object));
	}
}