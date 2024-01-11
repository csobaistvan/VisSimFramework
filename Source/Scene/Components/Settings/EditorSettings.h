#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace EditorSettings
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "EditorSettings";

	////////////////////////////////////////////////////////////////////////////////
	/** Represents an editor property with some additional helper functions. */
	struct EditorProperty
	{
		using ToStringFn = std::function<std::string(EditorProperty const&)>;

		ToStringFn m_toString;
		std::any m_value;
		bool m_persistent;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Component description the editor settings for the component. */
	struct EditorSettingsComponent
	{
		// The open status of the header.
		bool m_headerOpen = false;

		// Whether the object may be disabled and deleted or not.
		bool m_allowDisable = true;

		// Whether we allow multiples of these objects or not
		bool m_singular = true;

		// Key-value pairs, for widgets
		std::unordered_map<std::string, EditorProperty> m_properties;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	std::string sanitizePropertyName(std::string key);

	////////////////////////////////////////////////////////////////////////////////
	std::string storeEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name);

	////////////////////////////////////////////////////////////////////////////////
	bool parseEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name, std::string const& propertyValue);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::string propertyToString(EditorProperty const& property)
	{
		std::stringstream sstream;
		sstream.precision(12);
		sstream << std::fixed << std::boolalpha << std::any_cast<T>(property.m_value);
		return sstream.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void instantiateEditorProperty(EditorProperty& property, bool persistent = true, T const& defaultValue = T())
	{
		// Store the default value
		property.m_value = defaultValue;

		// Store the to-string function
		property.m_toString = EditorProperty::ToStringFn(&propertyToString<T>);

		// Store the persistence flag
		property.m_persistent = persistent;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void instantiateEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name, bool persistent = true, T const& defaultValue = T())
	{
		std::string const& key = sanitizePropertyName(name);
		instantiateEditorProperty(object->component<EditorSettings::EditorSettingsComponent>().m_properties[key], persistent, defaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool removeEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name);

	////////////////////////////////////////////////////////////////////////////////
	bool hasEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::optional<T> consumeEditorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name)
	{
		// Property list
		std::string const& key = sanitizePropertyName(name);
		auto& properties = object->component<EditorSettings::EditorSettingsComponent>().m_properties;

		// Attempt to consume the property
		if (auto it = properties.find(key); it != properties.end())
		{
			// Copy the contents of the property
			auto property = it->second;

			// Erase the property
			properties.erase(it);

			// Return the value of the consumed property
			return std::any_cast<T>(property.m_value);
		}

		// Return an empty optional if it wasn't present
		return std::optional<T>();
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T& editorProperty(Scene::Scene& scene, Scene::Object* object, std::string const& name, bool persistent = true, T const& defaultValue = T())
	{
		// Property list
		std::string const& key = sanitizePropertyName(name);
		auto& properties = object->component<EditorSettings::EditorSettingsComponent>().m_properties;

		// Create a new property if none is found
		if (properties.find(key) == properties.end())
			instantiateEditorProperty(object->component<EditorSettings::EditorSettingsComponent>().m_properties[key], persistent, defaultValue);

		// Return a reference to the stored value
		return std::any_cast<T&>(properties[key].m_value);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(EDITOR_SETTINGS, EditorSettingsComponent, EditorSettings::EditorSettingsComponent)