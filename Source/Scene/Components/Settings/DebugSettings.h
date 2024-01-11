#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "EditorSettings.h"

namespace DebugSettings
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "DebugSettings";
	static constexpr const char* DISPLAY_NAME = "Debug Settings";
	static constexpr const char* CATEGORY = "Settings";

	////////////////////////////////////////////////////////////////////////////////
	/** Debug component. */
	struct DebugSettingsComponent
	{
		// Whether we are monitoring the CPU performance or not.
		bool m_profileCpu = true;

		// Whether we are monitoring the GPU performance or not.
		bool m_profileGpu = true;

		// Whether the profiler should collect previous values or not
		bool m_profilerStoreValues = true;

		// How many frames to keep when purging the profiler history buffer
		int m_profilerHistoryNumPrevFrames = 512;

		// How frequently to purge the profiler frame history
		int m_profilerHistoryPurgeFrequency = 2048;

		// Whether logging to memory buffers should be enabled or not.
		Debug::LogChannels m_logToMemory;

		// Whether logging to files should be enabled or not.
		Debug::LogChannels m_logToFile;

		// Whether logging to the console should be enabled or not.
		Debug::LogChannels m_logConsole;

		// Length of a costly task node prefix (per depth)
		int m_costlyTaskLogNodeLength = 1;

		// Threshold for logging long tasks
		float m_costlyTaskLogThreshold = 1000.0f;

		// Crt debug settings
		struct CrtDebugSettings
		{
			bool m_debugMemory;
		} m_crtSettings;

		//  ----------------- Private members ------------------------------

		// When was the profiler history buffer last purged
		int m_profilerHistoryLastPurgeFrame = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateLoggerStates(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(DEBUG_SETTINGS, DebugSettingsComponent, DebugSettings::DebugSettingsComponent)
DECLARE_OBJECT(DEBUG_SETTINGS, COMPONENT_ID_DEBUG_SETTINGS, COMPONENT_ID_EDITOR_SETTINGS)