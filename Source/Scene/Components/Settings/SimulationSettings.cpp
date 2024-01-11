#include "PCH.h"
#include "SimulationSettings.h"
#include "InputSettings.h"
#include "DelayedJobs.h"

namespace SimulationSettings
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(SIMULATION_SETTINGS);
	DEFINE_OBJECT(SIMULATION_SETTINGS);
	REGISTER_OBJECT_UPDATE_CALLBACK(SIMULATION_SETTINGS, AFTER, INPUT);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Store the startup date
		time(&object.component<SimulationSettings::SimulationSettingsComponent>().m_startupDate);
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object)
	{
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_SPACE] == 1)
		{
			object->component<SimulationSettingsComponent>().m_timeDilation = object->component<SimulationSettingsComponent>().m_timeDilation == 0.0f ? 1.0f : 0.0f;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		updateEnabledGroupList(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiGroup(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, ObjectGroup& group)
	{
		if (ImGui::RadioButton("###enabled", group.m_enabled))
		{
			DelayedJobs::postJob(scene, object, "Enable Group " + group.m_name, [groupName = group.m_name](Scene::Scene& scene, Scene::Object& object)
			{
				enableGroup(scene, &object, groupName, true);
			});
		}
		ImGui::SameLine();
		ImGui::InputText("###name", group.m_name);
		ImGui::SameLine();
		if (ImGui::Button("Delete"))
		{
			DelayedJobs::postJob(scene, object, "Delete Group " + group.m_name, [groupName = group.m_name](Scene::Scene& scene, Scene::Object& object)
			{
				deleteGroup(scene, &object, groupName);
			});
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::SliderFloat("Time dilation", &object->component<SimulationSettings::SimulationSettingsComponent>().m_timeDilation, 0.0f, 4.0f);
		ImGui::Checkbox("Simulate in Background", &object->component<SimulationSettings::SimulationSettingsComponent>().m_simulateInBackground);

		// Resource reloading
		if (ImGui::Button("Reload Resources"))
		{
			ImGui::OpenPopup("SimulationSettings_ReloadResources");
		}

		if (ImGui::BeginPopup("SimulationSettings_ReloadResources"))
		{
			Scene::ResourceType* resourceType = (Scene::ResourceType*)&EditorSettings::editorProperty<int>(scene, object, "SimulationSettings_ReloadResourceType");
			ImGui::Combo("Resource Type", resourceType, Scene::ResourceType_meta);

			if (ImGui::ButtonEx("Ok", "|########|"))
			{
				Scene::reloadResources(scene, *resourceType);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::ButtonEx("Cancel", "|########|"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("SimulationSettings_CreateObjectGroup"))
		{
			ImGui::InputText("Category", EditorSettings::editorProperty<std::string>(scene, object, "ObjectGroup_CreateGroup_Category", false));
			ImGui::InputText("Name", EditorSettings::editorProperty<std::string>(scene, object, "ObjectGroup_CreateGroup_Name", false));

			if (ImGui::ButtonEx("Ok", "|########|"))
			{
				// Make sure no group with this name exists
				if (!groupExists(scene, object, EditorSettings::editorProperty<std::string>(scene, object, "ObjectGroup_CreateGroup_Name")))
				{
					createGroup(scene, 
						EditorSettings::editorProperty<std::string>(scene, object, "ObjectGroup_CreateGroup_Category"), 
						EditorSettings::editorProperty<std::string>(scene, object, "ObjectGroup_CreateGroup_Name"));
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::ButtonEx("Cancel", "|########|"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Collect the unique categories
		std::unordered_set<std::string> categories;
		categories.reserve(object->component<SimulationSettingsComponent>().m_groups.size());

		for (auto const& group : object->component<SimulationSettingsComponent>().m_groups)
			if (categories.find(group.m_category) == categories.end())
				categories.insert(group.m_category);

		// Generate the gui for each category
		for (auto const& category : categories)
		{
			ImGui::Separator();
			ImGui::Text(category.c_str());
			ImGui::Separator();

			for (auto& group : object->component<SimulationSettingsComponent>().m_groups)
			{
				if (group.m_category != category) continue;

				ImGui::PushID(&group);
				generateGuiGroup(scene, guiSettings, object, group);
				ImGui::PopID();
			}
		}

		ImGui::Separator();

		// Button for creating new groups
		if (ImGui::Button("New Group"))
		{
			ImGui::OpenPopup("SimulationSettings_CreateObjectGroup");
		}

		// Update the groups
		updateEnabledGroupList(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateEnabledGroupList(Scene::Scene& scene, Scene::Object* object)
	{
		unsigned long long enabled = 0;
		for (size_t groupId = 0; groupId < object->component<SimulationSettingsComponent>().m_groups.size(); ++groupId)
		{
			if (object->component<SimulationSettingsComponent>().m_groups[groupId].m_enabled)
			{
				enabled = enabled | std::bit_mask(groupId);
			}
		}
		object->component<SimulationSettingsComponent>().m_enabledGroups = enabled;
	}

	////////////////////////////////////////////////////////////////////////////////
	void createGroup(Scene::Scene& scene, Scene::Object* object, std::string const& category, std::string const& groupName, bool enabled)
	{
		// Extract the list of groups
		auto& groups = object->component<SimulationSettingsComponent>().m_groups;

		// Enable the group if it's enabled by console args,
		enabled |= Config::AttribValue("object_group").contains(groupName);

		// Create and store the new group
		ObjectGroup newGroup;
		newGroup.m_name = groupName;
		newGroup.m_category = category;
		newGroup.m_enabled = enabled;
		newGroup.m_groupId = groups.size();
		groups.emplace_back(newGroup);

		// Update the list of enabled groups
		updateEnabledGroupList(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void createGroup(Scene::Scene& scene, std::string const& category, std::string const& groupName, bool enabled)
	{
		createGroup(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), category, groupName, enabled);
	}

	////////////////////////////////////////////////////////////////////////////////
	void deleteGroup(Scene::Scene& scene, Scene::Object* object, std::string const& groupName)
	{
		auto it = std::find_if(object->component<SimulationSettingsComponent>().m_groups.begin(), object->component<SimulationSettingsComponent>().m_groups.end(), [&](auto const& group) { return group.m_name == groupName; });
		if (it == object->component<SimulationSettingsComponent>().m_groups.end()) return;

		// Id of the group
		size_t groupId = it->m_groupId;

		// Mask values
		static const unsigned long long ONES = 0xFFFFFFFFFFFFFFFF;

		unsigned long long preMaskExc = (ONES >> groupId) << groupId; // mask to zero out the bits right to this group
		unsigned long long preMaskInc = ~preMaskExc;                  // mask to zero out the bits left to this group (including this one)

		// Update the object groups flag
		for (auto& object : scene.m_objects)
		{
			unsigned long long groups = object.second.m_groups;
			object.second.m_groups = (groups & preMaskInc) | ((groups & preMaskExc) >> 1);
		}

		// Erase the group
		object->component<SimulationSettingsComponent>().m_groups.erase(it);

		// Update the internal group ids
		for (size_t i = 0; i < object->component<SimulationSettingsComponent>().m_groups.size(); ++i)
		{
			object->component<SimulationSettingsComponent>().m_groups[i].m_groupId = i;
		}

		// Update the list of enabled groups
		updateEnabledGroupList(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void deleteGroup(Scene::Scene& scene, std::string const& groupName)
	{
		deleteGroup(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), groupName);
	}

	////////////////////////////////////////////////////////////////////////////////
	ObjectGroup* getGroupByName(Scene::Scene& scene, Scene::Object* object, std::string const& groupName)
	{
		for (size_t i = 0; i < object->component<SimulationSettingsComponent>().m_groups.size(); ++i)
			if (object->component<SimulationSettingsComponent>().m_groups[i].m_name == groupName)
				return &object->component<SimulationSettingsComponent>().m_groups[i];

		return nullptr;
	}

	////////////////////////////////////////////////////////////////////////////////
	ObjectGroup* getGroupByName(Scene::Scene& scene, std::string const& groupName)
	{
		return getGroupByName(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), groupName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool groupExists(Scene::Scene& scene, Scene::Object* object, std::string const& groupName)
	{
		return getGroupByName(scene, object, groupName) != nullptr;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool groupExists(Scene::Scene& scene, std::string const& groupName)
	{
		return groupExists(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), groupName);
	}

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, Scene::Object* object, std::string const& groupName)
	{
		if (auto group = getGroupByName(scene, object, groupName); group != nullptr)
			return std::bit_mask(group->m_groupId);
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, std::string const& group)
	{
		return makeGroupFlags(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), group);
	}

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, Scene::Object* object, std::vector<std::string> const& groups)
	{
		unsigned long long result = 0;
		for (auto const& groupName : groups)
			if (auto group = getGroupByName(scene, object, groupName); group != nullptr) 
				result |= std::bit_mask(group->m_groupId);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	unsigned long long makeGroupFlags(Scene::Scene& scene, std::vector<std::string> const& groups)
	{
		return makeGroupFlags(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), groups);
	}

	////////////////////////////////////////////////////////////////////////////////
	void enableGroup(Scene::Scene& scene, Scene::Object* object, std::string const& groupName, bool enabled)
	{
		if (auto group = getGroupByName(scene, object, groupName); group != nullptr)
		{
			// Update the flags in this category
			for (auto& groupIt : object->component<SimulationSettingsComponent>().m_groups)
				if (groupIt.m_category == group->m_category) groupIt.m_enabled = groupIt.m_name == groupName ? enabled : false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void enableGroup(Scene::Scene& scene, std::string const& groupName, bool enabled)
	{
		return enableGroup(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), groupName, enabled);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectInGroup(Scene::Scene& scene, Scene::Object* object, ObjectGroup const& group)
	{
		return (object->m_groups & std::bit_mask(group.m_groupId)) != 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectInGroup(Scene::Scene& scene, Scene::Object* object, std::string const& group)
	{
		if (auto groupObj = getGroupByName(scene, group); groupObj != nullptr)
			return isObjectInGroup(scene, object, *groupObj);
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	void addObjectToGroup(Scene::Scene& scene, Scene::Object* object, ObjectGroup const& group)
	{
		object->m_groups|= std::bit_mask(group.m_groupId);
	}

	////////////////////////////////////////////////////////////////////////////////
	void addObjectToGroup(Scene::Scene& scene, Scene::Object* object, std::string const& group)
	{
		if (auto groupObj = getGroupByName(scene, group); groupObj != nullptr)
			addObjectToGroup(scene, object, *groupObj);
	}

	////////////////////////////////////////////////////////////////////////////////
	void removeObjectFromGroup(Scene::Scene& scene, Scene::Object* object, ObjectGroup const& group)
	{
		object->m_groups = object->m_groups & ~(std::bit_mask(group.m_groupId));
	}

	////////////////////////////////////////////////////////////////////////////////
	void removeObjectFromGroup(Scene::Scene& scene, Scene::Object* object, std::string const& group)
	{
		if (auto groupObj = getGroupByName(scene, group); groupObj != nullptr)
			removeObjectFromGroup(scene, object, *groupObj);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabledByGroups(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		return object->m_groups == 0 || simulationSettings == nullptr ||
			simulationSettings->component<SimulationSettingsComponent>().m_enabledGroups == 0 ||
			(object->m_groups & simulationSettings->component<SimulationSettingsComponent>().m_enabledGroups) != 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabledByGroups(Scene::Scene& scene, Scene::Object* object)
	{
		if (object->m_groups == 0) return true;
		return isObjectEnabledByGroups(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabled(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		return object->m_enabled && isObjectEnabledByGroups(scene, simulationSettings, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isObjectEnabled(Scene::Scene& scene, Scene::Object* object)
	{
		if (object->m_enabled && object->m_groups == 0) return true;
		return isObjectEnabled(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), object);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getActiveGroup(Scene::Scene& scene, Scene::Object* simulationSettings, std::string const& category)
	{
		// Disable everything first
		for (auto& group : simulationSettings->component<SimulationSettingsComponent>().m_groups)
		{
			if (group.m_category == category && group.m_enabled)
			{
				return group.m_name;
			}
		}
		return "";
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getActiveGroup(Scene::Scene& scene, std::string const& category)
	{
		return getActiveGroup(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS), category);
	}
}