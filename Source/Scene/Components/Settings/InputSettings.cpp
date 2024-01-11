#include "PCH.h"
#include "InputSettings.h"

namespace InputSettings
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(INPUT);
	DEFINE_OBJECT(INPUT);
	REGISTER_OBJECT_UPDATE_CALLBACK(INPUT, AFTER, BEGIN);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		enableInput(scene, &object, object.component<InputComponent>().m_input);
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void invokeObjectInputHandlers(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input)
	{
		// Generate GUI elements for the various object types
		for (auto it : Scene::objectInputFunctions())
		{
			// Extaxt the data
			auto [objectType, objectInputFunction] = it;

			Profiler::ScopedCpuPerfCounter perfCounter(scene, Scene::objectNames()[objectType]);

			// Go through each object of the corresponding object type
			for (auto object : Scene::filterObjects(scene, objectType, true))
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

				// Invoke its input callback
				objectInputFunction(scene, simulationSettings, input, object);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

		// Update the mouse coordinates
		double mouseX, mouseY;
		glfwGetCursorPos(scene.m_context.m_window, &mouseX, &mouseY);

		object->component<InputSettings::InputComponent>().m_prevMouseCoordinates = 
			(!object->component<InputSettings::InputComponent>().m_input || object->component<InputSettings::InputComponent>().m_currentMouseCoordinates == glm::vec2(-1.0f)) ? 
			glm::vec2(mouseX, mouseY) : 
			object->component<InputSettings::InputComponent>().m_currentMouseCoordinates;
		object->component<InputSettings::InputComponent>().m_currentMouseCoordinates = glm::vec2(mouseX, mouseY);

		// Update the key states
		for (auto key : InputSettings::s_allKeys)
		{
			if (glfwGetKey(scene.m_context.m_window, key) == GLFW_PRESS)
			{
				++object->component<InputSettings::InputComponent>().m_keys[key];
			}
			else
			{
				object->component<InputSettings::InputComponent>().m_keys[key] = 0;
			}
		}

		// Invoke the object input handlers
		invokeObjectInputHandlers(scene, simulationSettings, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object)
	{
		// Turn the input mode
		if (object->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_ESCAPE] == 1)
		{
			enableInput(scene, object, !object->component<InputSettings::InputComponent>().m_input);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void enableInput(Scene::Scene& scene, Scene::Object* object, bool enabled)
	{
		object->component<InputSettings::InputComponent>().m_input = enabled;
		glfwSetInputMode(scene.m_context.m_window, GLFW_CURSOR, object->component<InputSettings::InputComponent>().m_input ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	////////////////////////////////////////////////////////////////////////////////
	void enableInput(Scene::Scene& scene, bool enabled)
	{
		enableInput(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_INPUT), enabled);
	}
}