#include "PCH.h"
#include "DebugSettings.h"
#include "SimulationSettings.h"

namespace DebugSettings
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(DEBUG_SETTINGS);
	DEFINE_OBJECT(DEBUG_SETTINGS);
	REGISTER_OBJECT_UPDATE_CALLBACK(DEBUG_SETTINGS, AFTER, INPUT);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Default log levels
		object.component<DebugSettings::DebugSettingsComponent>().m_logToMemory = Debug::defaultLogChannelsMemory();
		object.component<DebugSettings::DebugSettingsComponent>().m_logConsole = Debug::defaultLogChannelsConsole();
		object.component<DebugSettings::DebugSettingsComponent>().m_logToFile = Debug::defaultLogChannelsFile();
		object.component<DebugSettings::DebugSettingsComponent>().m_profileCpu = Profiler::profilingDefault();
		object.component<DebugSettings::DebugSettingsComponent>().m_profileGpu = Profiler::profilingDefault();

		// Set the default log levels
		updateLoggerStates(scene, &object);

		// Set the error report mode
		if (object.component<DebugSettingsComponent>().m_crtSettings.m_debugMemory)
		{
			// Set debug flags if the debugger is present
			_CrtSetDbgFlag(
				_CRTDBG_ALLOC_MEM_DF | 
				_CRTDBG_LEAK_CHECK_DF | 
				_CRTDBG_CHECK_EVERY_1024_DF | 
				_CRTDBG_LEAK_CHECK_DF);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	bool isTaskCostly(Scene::Object* debugSettings, Profiler::ProfilerDataEntry const& entry)
	{
		return (Profiler::isEntryType<Profiler::ProfilerDataEntry::EntryDataFieldTime>(entry.m_current) &&
			Profiler::convertEntryData<float>(entry.m_current) >= debugSettings->component<DebugSettings::DebugSettingsComponent>().m_costlyTaskLogThreshold);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool collectCostlyTasks(Scene::Scene& scene, Scene::Object* debugSettings, Profiler::ProfilerThreadTree const& tree, Profiler::ProfilerThreadTreeIterator root, std::unordered_set<std::string>& costlyNodeNames)
	{
		bool costly = false;
		for (auto it = tree.begin(root); it != tree.end(root); ++it)
		{
			auto const& entry = Profiler::dataEntry(scene, *it);
			costly |= isTaskCostly(debugSettings, entry) || collectCostlyTasks(scene, debugSettings, tree, it, costlyNodeNames);
			if (costly) costlyNodeNames.insert(entry.m_category);
		};
		return costly;
	}

	////////////////////////////////////////////////////////////////////////////////
	void logCostlyTasks(Scene::Scene& scene, Scene::Object* debugSettings, Profiler::ProfilerThreadTree const& tree, Profiler::ProfilerThreadTreeIterator root, size_t depth, std::unordered_set<std::string>& costlyNodeNames)
	{
		// Iterate over the children
		for (auto it = tree.begin(root); it != tree.end(root); ++it)
		{
			auto const& entry = Profiler::dataEntry(scene, *it);
			if (costlyNodeNames.find(entry.m_category) != costlyNodeNames.end())
			{
				std::stringstream prefix;
				for (size_t i = 0; i <= depth; ++i) prefix << "|" << std::string(debugSettings->component<DebugSettingsComponent>().m_costlyTaskLogNodeLength, i == depth ? '-' :' ');
				Debug::log_debug() << prefix.str() << entry.m_category << ": " << Profiler::convertEntryData<std::string>(entry.m_current) << Debug::end;
			}
			logCostlyTasks(scene, debugSettings, tree, it, depth + 1, costlyNodeNames); // Do this recursively
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shouldPurgeProfilerHistory(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		const int frameId = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
		const int lastPurge = object->component<DebugSettingsComponent>().m_profilerHistoryLastPurgeFrame;
		const int purgeFrequency = object->component<DebugSettingsComponent>().m_profilerHistoryPurgeFrequency;
		return frameId > lastPurge + purgeFrequency;
	}

	////////////////////////////////////////////////////////////////////////////////
	void purgeProfilerHistory(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Profiler History Purge");

		const int frameId = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
		const int keptFrames = object->component<DebugSettingsComponent>().m_profilerHistoryNumPrevFrames;

		for (auto& node : scene.m_profilerData)
		{
			if (node.second.m_previousValues.size() > 0)
			{
				// Create a local copy of the old values
				auto oldValues = node.second.m_previousValues;
				node.second.m_previousValues.clear();
				node.second.m_previousValues.reserve(keptFrames);

				// Copy over the old values that are recent enough
				for (auto const& value : oldValues)
					if (value.first >= frameId - keptFrames)
						node.second.m_previousValues[value.first] = value.second;
			}
		}

		object->component<DebugSettingsComponent>().m_profilerHistoryLastPurgeFrame = frameId;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Purge the profiler history buffer
		if (shouldPurgeProfilerHistory(scene, simulationSettings, object))
			purgeProfilerHistory(scene, simulationSettings, object);

		// Log the costly tasks
		auto& currentProfilerTree = scene.m_profilerTree[scene.m_profilerBufferReadId][0];
		std::unordered_set<std::string> costlyNodeNames;
		collectCostlyTasks(scene, object, currentProfilerTree, currentProfilerTree.begin(), costlyNodeNames);

		if (costlyNodeNames.size() > 0)
		{
			Debug::log_debug() << std::string(80, '=') << Debug::end;
			Debug::log_debug() << "List of costly tasks: " << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			logCostlyTasks(scene, object, currentProfilerTree, currentProfilerTree.begin(), 0, costlyNodeNames);
			Debug::log_debug() << std::string(80, '=') << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateLoggerStates(Scene::Scene& scene, Scene::Object* object)
	{
		Debug::setMemoryLogging(object->component<DebugSettings::DebugSettingsComponent>().m_logToMemory);
		Debug::setConsoleLogging(object->component<DebugSettings::DebugSettingsComponent>().m_logConsole);
		Debug::setFileLogging(object->component<DebugSettings::DebugSettingsComponent>().m_logToFile);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool generateGuiLogChannel(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, std::string const& label, Debug::LogChannels& channel)
	{
		bool logChanged = false;

		if (ImGui::TreeNode(label.c_str()))
		{
			logChanged = ImGui::Checkbox("Debug", &channel.m_debug) || logChanged;
			logChanged = ImGui::Checkbox("Trace", &channel.m_trace) || logChanged;
			logChanged = ImGui::Checkbox("Info", &channel.m_info) || logChanged;
			logChanged = ImGui::Checkbox("Warning", &channel.m_warning) || logChanged;
			logChanged = ImGui::Checkbox("Error", &channel.m_error) || logChanged;
			if (ImGui::Button("All"))
			{
				channel.m_debug = channel.m_trace = channel.m_info = channel.m_warning = channel.m_error = true;
				logChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("None"))
			{
				channel.m_debug = channel.m_trace = channel.m_info = channel.m_warning = channel.m_error = false;
				logChanged = true;
			}

			ImGui::TreePop();
		}
		return logChanged;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		bool logChanged = false;

		if (ImGui::BeginTabBar(object->m_name.c_str()) == false)
			return;

		// Restore the selected tab id
		std::string activeTab;
		if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, object, "MainTabBar_SelectedTab#Synced"); activeTabSynced.has_value())
			activeTab = activeTabSynced.value();

		if (ImGui::BeginTabItem("Log", activeTab.c_str()))
		{
			logChanged = generateGuiLogChannel(scene, guiSettings, object, "Log to Memory", object->component<DebugSettings::DebugSettingsComponent>().m_logToMemory) || logChanged;
			logChanged = generateGuiLogChannel(scene, guiSettings, object, "Log to Console", object->component<DebugSettings::DebugSettingsComponent>().m_logConsole) || logChanged;
			logChanged = generateGuiLogChannel(scene, guiSettings, object, "Log to File", object->component<DebugSettings::DebugSettingsComponent>().m_logToFile) || logChanged;


			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Profiler", activeTab.c_str()))
		{
			ImGui::DragInt("History Purge Frequency", &object->component<DebugSettings::DebugSettingsComponent>().m_profilerHistoryPurgeFrequency);
			ImGui::DragInt("History Kept Frames", &object->component<DebugSettings::DebugSettingsComponent>().m_profilerHistoryNumPrevFrames);

			ImGui::SliderFloat("Long Task Threshold", &object->component<DebugSettings::DebugSettingsComponent>().m_costlyTaskLogThreshold, 0.0f, 1000.0f);
			ImGui::SliderInt("Long Task Prefix Length", &object->component<DebugSettingsComponent>().m_costlyTaskLogNodeLength, 1, 4);

			ImGui::Checkbox("Profile CPU", &object->component<DebugSettings::DebugSettingsComponent>().m_profileCpu);
			ImGui::SameLine();
			ImGui::Checkbox("Profile GPU", &object->component<DebugSettings::DebugSettingsComponent>().m_profileGpu);
			ImGui::SameLine();
			ImGui::Checkbox("Profiler History", &object->component<DebugSettings::DebugSettingsComponent>().m_profilerStoreValues);

			if (ImGui::Button("Save Profiler Stats"))
			{
				EditorSettings::editorProperty<std::string>(scene, object, "Debug_SaveProfilerStats_FileName") = EnginePaths::generateUniqueFilename("Profile-Data-", ".csv");
				EditorSettings::editorProperty<std::string>(scene, object, "Debug_SaveProfilerStats_RootCategory") = ".*";
				ImGui::OpenPopup("SaveProfilerStats");
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear Profiler Stats"))
			{
				Profiler::clearTree(scene);
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// End the tab bar
		ImGui::EndTabBar();

		if (ImGui::BeginPopup("SaveProfilerStats"))
		{
			ImGui::InputText("File Name", EditorSettings::editorProperty<std::string>(scene, object, "Debug_SaveProfilerStats_FileName"));
			ImGui::InputText("Root Node", EditorSettings::editorProperty<std::string>(scene, object, "Debug_SaveProfilerStats_RootCategory"));

			if (ImGui::ButtonEx("Ok", "|########|"))
			{
				std::string csvSource = Profiler::exportTreeCsv(scene, EditorSettings::editorProperty<std::string>(scene, object, "Debug_SaveProfilerStats_RootCategory"), true);
				std::string fileName = std::string("Generated/Profiler/") + EditorSettings::editorProperty<std::string>(scene, object, "Debug_SaveProfilerStats_FileName");
				Asset::saveTextFile(scene, fileName, csvSource);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::ButtonEx("Cancel", "|########|"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (logChanged)
		{
			updateLoggerStates(scene, object);
		}
	}
}