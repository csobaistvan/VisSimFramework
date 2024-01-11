#include "PCH.h"
#include "EditorSettings.h"

namespace EditorSettings
{
	////////////////////////////////////////////////////////////////////////////////
	DEFINE_COMPONENT(EDITOR_SETTINGS);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	std::string sanitizePropertyName(std::string key)
	{
		std::replace(key.begin(), key.end(), ' ', '_');
		return key;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool hasEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name)
	{
		std::string const& key = sanitizePropertyName(name);
		return object->component<EditorSettings::EditorSettingsComponent>().m_properties.find(key) != object->component<EditorSettings::EditorSettingsComponent>().m_properties.end();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool removeEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name)
	{
		// Property list
		std::string const& key = sanitizePropertyName(name);
		auto& properties = object->component<EditorSettings::EditorSettingsComponent>().m_properties;
		
		// Erase the property if it is present
		if (auto it = properties.find(key); properties.find(key) != properties.end())
		{
			properties.erase(it);
			return true;
		}

		// Nothing was found
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string storeEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name)
	{
		std::string const& key = sanitizePropertyName(name);
		auto const& property = object->component<EditorSettings::EditorSettingsComponent>().m_properties[key];
		return property.m_toString(property);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	bool tryParseEditorProperty(EditorProperty& property, std::string const& value)
	{
		// Try to parse the value
		T result;
		std::istringstream iss(value);
		iss >> std::boolalpha >> result;

		// Store it if we succeeded
		if ((iss.rdstate() & iss.failbit) == 0 && iss.get() == EOF)
		{
			instantiateEditorProperty(property, true, result);
			return true;
		}

		// Couldn't be parsed
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool parseEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name, std::string const& propertyValue)
	{
		// Reference to the property
		std::string const& key = sanitizePropertyName(name);
		auto& property = object->component<EditorSettingsComponent>().m_properties[key];

		// Result
		bool result = false;

		// TODO: redo this once we have better reflection/serialization support
		// Try some common types
		result = result || tryParseEditorProperty<int>(property, propertyValue);
		result = result || tryParseEditorProperty<float>(property, propertyValue);
		result = result || tryParseEditorProperty<double>(property, propertyValue);
		result = result || tryParseEditorProperty<bool>(property, propertyValue);

		// Fall back to a string property-note that this will eat everything
		if (result == false)
		{
			instantiateEditorProperty(property, true, propertyValue);
			result = true;
		}

		// Return the result
		return result;
	}
}