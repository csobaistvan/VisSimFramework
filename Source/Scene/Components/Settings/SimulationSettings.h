#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "EditorSettings.h"

namespace SimulationSettings
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "SimulationSettings";
	static constexpr const char* DISPLAY_NAME = "Simulation Settings";
	static constexpr const char* CATEGORY = "Settings";

	////////////////////////////////////////////////////////////////////////////////
	/** Represents one object group. */
	struct ObjectGroup
	{
		// Name of the group
		std::string m_name = "";

		// Category of the group
		std::string m_category = "";

		// Is the group enabled
		bool m_enabled = true;

		// Id of the group
		int m_groupId = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Simulation settings component. */
	struct SimulationSettingsComponent
	{
		// Is the simulation even running or not
		bool m_simulationStarted = false;

		// Time when the app was started
		time_t m_startupDate;

		// Time when simulation started
		time_t m_simulationStartDate;

		// Whether the window is in a focused state or not.
		bool m_simulateInBackground = false;

		// Simulation time dilation
		float m_timeDilation = 1.0f;

		// Maximum FPS
		float m_maxFps = 60.0f;

		// Current frame identifier
		int m_frameId;

		// Time of last update
		double m_lastUpdateTime;

		// Global time since actual simulation started
		double m_globalTime;

		// Elapsed time since last FPS
		double m_deltaTime;

		// Fixed delta time
		float m_fixedDeltaTime;

		// Scaled delta time
		double m_scaledDeltaTime;

		// Scaled global time
		double m_scaledGlobalTime;

		// Start of the last second
		double m_lastUpdateSecond;

		// Number of frames since start of the last second
		int m_numFrames;

		// FPS during last frame
		int m_lastFPS;

		// Current FPS
		double m_smoothFPS;

		// Whether the window is in a focused state or not.
		bool m_focused;

		// Time since startup when a specific frame started
		std::map<int, double> m_frameStartTimepoint;

		// All the groups in the scene
		std::vector<ObjectGroup> m_groups;

		// ---- Private members

		// List of enabled groups
		unsigned long long m_enabledGroups = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateEnabledGroupList(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void createGroup(Scene::Scene& scene, Scene::Object* object, std::string const& category, std::string const& groupName, bool enabled = false);

	////////////////////////////////////////////////////////////////////////////////
	void createGroup(Scene::Scene& scene, std::string const& category, std::string const& groupName, bool enabled = false);

	////////////////////////////////////////////////////////////////////////////////
	void deleteGroup(Scene::Scene& scene, Scene::Object* object, std::string const& groupName);

	////////////////////////////////////////////////////////////////////////////////
	void deleteGroup(Scene::Scene& scene, std::string const& groupName);

	////////////////////////////////////////////////////////////////////////////////
	ObjectGroup* getGroupByName(Scene::Scene& scene, Scene::Object* object, std::string const& groupName);

	////////////////////////////////////////////////////////////////////////////////
	ObjectGroup* getGroupByName(Scene::Scene& scene, std::string const& groupName);

	////////////////////////////////////////////////////////////////////////////////
	bool groupExists(Scene::Scene& scene, Scene::Object* object, std::string const& groupName);

	////////////////////////////////////////////////////////////////////////////////
	bool groupExists(Scene::Scene& scene, std::string const& groupName);

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, Scene::Object* object, std::string const& group);

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, std::string const& group);

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, Scene::Object* object, std::vector<std::string> const& groups);

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, std::vector<std::string> const& groups);

	////////////////////////////////////////////////////////////////////////////////
	void enableGroup(Scene::Scene& scene, Scene::Object* object, std::string const& groupName, bool enabled);

	////////////////////////////////////////////////////////////////////////////////
	void enableGroup(Scene::Scene& scene, std::string const& groupName, bool enabled);

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectInGroup(Scene::Scene& scene, Scene::Object* object, ObjectGroup const& group);

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectInGroup(Scene::Scene& scene, Scene::Object* object, std::string const& group);

	////////////////////////////////////////////////////////////////////////////////
	void addObjectToGroup(Scene::Scene& scene, Scene::Object* object, ObjectGroup const& group);

	////////////////////////////////////////////////////////////////////////////////
	void addObjectToGroup(Scene::Scene& scene, Scene::Object* object, std::string const& group);

	////////////////////////////////////////////////////////////////////////////////
	void removeObjectFromGroup(Scene::Scene& scene, Scene::Object* object, ObjectGroup const& group);

	////////////////////////////////////////////////////////////////////////////////
	void removeObjectFromGroup(Scene::Scene& scene, Scene::Object* object, std::string const& group);

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabledByGroups(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabledByGroups(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabled(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabled(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	std::string getActiveGroup(Scene::Scene& scene, Scene::Object* simulationSettings, std::string const& category);

	////////////////////////////////////////////////////////////////////////////////
	std::string getActiveGroup(Scene::Scene& scene, std::string const& category);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(SIMULATION_SETTINGS, SimulationSettingsComponent, SimulationSettings::SimulationSettingsComponent)
DECLARE_OBJECT(SIMULATION_SETTINGS, COMPONENT_ID_SIMULATION_SETTINGS, COMPONENT_ID_EDITOR_SETTINGS)