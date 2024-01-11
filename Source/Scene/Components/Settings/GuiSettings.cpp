#include "PCH.h"
#include "GuiSettings.h"
#include "DelayedJobs.h"
#include "InputSettings.h"
#include "SimulationSettings.h"
#include "RenderSettings.h"

namespace GuiSettings
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(GUI_SETTINGS);
	DEFINE_OBJECT(GUI_SETTINGS);
	REGISTER_OBJECT_UPDATE_CALLBACK(GUI_SETTINGS, BEFORE, DELAYED_JOBS);
	REGISTER_OBJECT_RENDER_CALLBACK(GUI_SETTINGS, "GUI", OpenGL, AFTER, "GUI [Begin]", 1, &GuiSettings::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiMenuBar(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateObjectsWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateStatsWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateLogWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateProfilerWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateMaterialEditorWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateShaderInspectorWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateBufferInspectorWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateTextureInspectorWindow(Scene::Scene& scene, Scene::Object* guiSettings);
	void generateMainRenderWindow(Scene::Scene& scene, Scene::Object* guiSettings);

	////////////////////////////////////////////////////////////////////////////////
	struct GuiStateVars
	{
		std::unordered_map<std::string, bool*> m_bools;
		std::unordered_map<std::string, int*> m_ints;
		std::unordered_map<std::string, float*> m_floats;
		std::unordered_map<std::string, std::string*> m_strings;
	};

	////////////////////////////////////////////////////////////////////////////////
	GuiStateVars getGuiStateVars(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		GuiStateVars result;

		// Global theme
		result.m_floats["GUI_FontScale"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_fontScale;
		result.m_floats["GUI_PlotLineWeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_plotLineWeight;
		result.m_strings["GUI_Font"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_font;
		result.m_strings["GUI_Theme"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_guiStyle;
		result.m_strings["GUI_ColorMap"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_plotColorMap;
		result.m_bools["GUI_LockLayout"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout;

		// Show gui elements
		for (auto& element : guiSettings->component<GuiSettings::GuiSettingsComponent>().m_guiElements)
			result.m_bools[element.m_name] = &element.m_open;

		// Log output
		result.m_strings["Log_IncludeFilter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.m_text;
		result.m_strings["Log_ExcludeFilter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.m_text;

		result.m_bools["Log_ShowTrace"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showTrace;
		result.m_bools["Log_ShowDebug"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDebug;
		result.m_bools["Log_ShowInfo"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showInfo;
		result.m_bools["Log_ShowWarning"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showWarning;
		result.m_bools["Log_ShowError"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showError;
		result.m_bools["Log_ShowDate"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate;
		result.m_bools["Log_ShowSeverity"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity;
		result.m_bools["Log_ShowRegion"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showRegion;
		result.m_bools["Log_ShowQuickFilter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showQuickFilter;
		result.m_bools["Log_AutoScroll"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_autoScroll;
		result.m_bools["Log_LongMessages"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_longMessages;
		result.m_floats["Log_MessageSpacing"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_messageSpacing;
		result.m_floats["Log_TraceColorX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_traceColor.x;
		result.m_floats["Log_TraceColorY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_traceColor.y;
		result.m_floats["Log_TraceColorZ"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_traceColor.z;
		result.m_floats["Log_DebugColorX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_debugColor.x;
		result.m_floats["Log_DebugColorY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_debugColor.y;
		result.m_floats["Log_DebugColorZ"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_debugColor.z;
		result.m_floats["Log_InfoColorX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_infoColor.x;
		result.m_floats["Log_InfoColorY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_infoColor.y;
		result.m_floats["Log_InfoColorZ"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_infoColor.z;
		result.m_floats["Log_WarningColorX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_warningColor.x;
		result.m_floats["Log_WarningColorY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_warningColor.y;
		result.m_floats["Log_WarningColorZ"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_warningColor.z;
		result.m_floats["Log_ErrorColorX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_errorColor.x;
		result.m_floats["Log_ErrorColorY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_errorColor.y;
		result.m_floats["Log_ErrorColorZ"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_errorColor.z;

		// Stats
		result.m_floats["Stats_IndentScale"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_statWindowSettings.m_indentScale;

		// Profiler charts
		result.m_floats["Profiler_GraphHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_graphHeight;
		result.m_ints["Profiler_UpdateRatio"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_updateRatio;
		result.m_ints["Profiler_AvgWindowSize"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgWindowSize;
		result.m_ints["Profiler_NumFrames"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow;
		result.m_ints["Profiler_NodeLabels"] = (int*)&guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodeLabelMode;
		result.m_bools["Profiler_Freeze"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_freeze;
		result.m_floats["Profiler_PlotColorR"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_plotColor.r;
		result.m_floats["Profiler_PlotColorG"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_plotColor.g;
		result.m_floats["Profiler_PlotColorB"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_plotColor.b;
		result.m_floats["Profiler_AvgPlotColorR"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgPlotColor.r;
		result.m_floats["Profiler_AvgPlotColorG"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgPlotColor.g;
		result.m_floats["Profiler_AvgPlotColorB"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgPlotColor.b;

		// Shader inspector
		result.m_strings["ShaderInspector_Filter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_selectorFilter.m_text;
		result.m_strings["ShaderInspector_CurrentProgram"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram;
		result.m_ints["ShaderInspector_VariableNames"] = (int*)&guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_variableNames;
		result.m_ints["ShaderInspector_NumBlockRowsX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows.x;
		result.m_ints["ShaderInspector_NumBlockRowsY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows.y;
		result.m_ints["ShaderInspector_NumVariableRowsX"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows.x;
		result.m_ints["ShaderInspector_NumVariableRowsY"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows.y;
		result.m_ints["ShaderInspector_SelectorHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_shaderSelectorHeight;

		// Buffer inspector
		result.m_strings["BufferInspector_Filter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_bufferInspectorSettings.m_selectorFilter.m_text;
		result.m_strings["BufferInspector_CurrentBuffer"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_bufferInspectorSettings.m_currentBuffer;
		result.m_ints["BufferInspector_SelectorHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_bufferInspectorSettings.m_bufferSelectorHeight;

		// Texture inspector
		result.m_strings["TextureInspector_Filter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_selectorFilter.m_text;
		result.m_ints["TextureInspector_SelectorHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_textureSelectorHeight;
		result.m_ints["TextureInspector_PreviewHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_previewHeight;
		result.m_ints["TextureInspector_TooltipHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_tooltipHeight;
		result.m_ints["TextureInspector_1DTextureBlockSize"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_1dTextureBlockSize;
		result.m_ints["TextureInspector_1DArrayTextureBlockSize"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_1dArrayTextureBlockSize;

		// Material inspector
		result.m_strings["MaterialEditor_Filter"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_selectorFilter.m_text;
		result.m_ints["MaterialEditor_SelectorHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_materialSelectorHeight;
		result.m_ints["MaterialEditor_TooltipHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_tooltipHeight;
		result.m_ints["MaterialEditor_PreviewHeight"] = &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_previewHeight;

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void saveGuiState(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "GUI State Save");

		// Result of the saving
		std::ordered_pairs_in_blocks result;

		// Store some of the gui settings
		GuiStateVars guiState = getGuiStateVars(scene, guiSettings);
		for (auto const& var : guiState.m_bools) result["GuiSettings"].push_back({ var.first, std::to_string(*var.second) });
		for (auto const& var : guiState.m_ints) result["GuiSettings"].push_back({ var.first, std::to_string(*var.second) });
		for (auto const& var : guiState.m_floats) result["GuiSettings"].push_back({ var.first, std::to_string(*var.second) });
		for (auto const& var : guiState.m_strings) result["GuiSettings"].push_back({ var.first, *var.second });

		// Store the current header open state for the objects
		for (auto const& object : scene.m_objects)
		{
			if (object.second.component<EditorSettings::EditorSettingsComponent>().m_headerOpen)
			{
				result["ObjectsWindow#OpenHeaders"].push_back({ object.first, "1" });
			}
		}

		// Store the keyed editor properties
		for (auto& object : scene.m_objects)
		{
			std::string header = "Object#EditorProperties#" + object.first;
			for (auto const& property : object.second.component<EditorSettings::EditorSettingsComponent>().m_properties)
			{
				static std::string s_syncedSuffix = "#Synced";
				if (property.second.m_persistent && std::equal(s_syncedSuffix.rbegin(), s_syncedSuffix.rend(), property.first.rbegin()) == false)
				{
					result[header].push_back({ property.first, EditorSettings::storeEditorProperty(scene, &(object.second), property.first) });
				}
			}
		}

		// Store the profiler open category names
		for (auto const& categoryName : guiSettings->component<GuiSettings::GuiSettingsComponent>().m_statWindowSettings.m_openNodes)
		{
			result["StatsWindow#OpenNodes"].push_back({ categoryName, "1" });
		}

		// Store the profiler chart category names
		for (auto const& categoryName : guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow)
		{
			result["ProfilerChart#ShownCharts"].push_back({ categoryName, "1" });
		}

		// Save the state
		Asset::savePairsInBlocks(scene, guiSettings->component<GuiSettings::GuiSettingsComponent>().m_guiStateFileName, result, false);
	}

	////////////////////////////////////////////////////////////////////////////////
	void restoreGuiState(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "GUI State Restore");

		// Load the previous state
		auto stateOpt = Asset::loadPairsInBlocks(scene, guiSettings->component<GuiSettings::GuiSettingsComponent>().m_guiStateFileName);
		if (!stateOpt.has_value()) return;

		// Extract the state contents
		std::ordered_pairs_in_blocks& state = stateOpt.value();

		// Restore the GUI state
		GuiStateVars guiState = getGuiStateVars(scene, guiSettings);
		for (auto const& guiStateVar : state["GuiSettings"])
		{
			if (guiState.m_bools.find(guiStateVar.first) != guiState.m_bools.end()) *guiState.m_bools[guiStateVar.first] = std::from_string<bool>(guiStateVar.second);
			if (guiState.m_ints.find(guiStateVar.first) != guiState.m_ints.end()) *guiState.m_ints[guiStateVar.first] = std::from_string<int>(guiStateVar.second);
			if (guiState.m_floats.find(guiStateVar.first) != guiState.m_floats.end()) *guiState.m_floats[guiStateVar.first] = std::from_string<float>(guiStateVar.second);
			if (guiState.m_strings.find(guiStateVar.first) != guiState.m_strings.end()) *guiState.m_strings[guiStateVar.first] = guiStateVar.second;
		}

		// Restore the current header open state for the objects
		for (auto const& node : state["ObjectsWindow#OpenHeaders"])
		{
			if (scene.m_objects.find(node.first) != scene.m_objects.end())
			{
				scene.m_objects[node.first].component<EditorSettings::EditorSettingsComponent>().m_headerOpen = true;
			}
		}

		// Restore the keyed editor properties
		for (auto& object : scene.m_objects)
		{
			std::string header = "Object#EditorProperties#" + object.first;
			for (auto const& property : state[header])
			{
				if (EditorSettings::parseEditorProperty(scene, &(object.second), property.first, property.second))
				{
					object.second.component<EditorSettings::EditorSettingsComponent>().m_properties[property.first + "#Synced"] =
						object.second.component<EditorSettings::EditorSettingsComponent>().m_properties[property.first];
				}
			}
		}

		// Restore the stat window state
		for (auto const& node : state["StatsWindow#OpenNodes"])
		{
			guiSettings->component<GuiSettings::GuiSettingsComponent>().m_statWindowSettings.m_openNodes.push_back(node.first);
		}

		// Restore the profilert chart state
		for (auto const& node : state["ProfilerChart#ShownCharts"])
		{
			guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow.insert(node.first);
		}

		// Update the regex filters
		guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.update();
		guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.update();
		guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_selectorFilter.update();
		guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_selectorFilter.update();
		guiSettings->component<GuiSettings::GuiSettingsComponent>().m_bufferInspectorSettings.m_selectorFilter.update();
		guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_selectorFilter.update();
	}

	////////////////////////////////////////////////////////////////////////////////
	void initFramebuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Max supported resolution
		const glm::ivec2 resolution = GPU::maxResolution();

		Scene::createTexture(scene, "ImGui_MainRenderTarget", GL_TEXTURE_2D, resolution[0], resolution[1], 1, GL_RGBA8, GL_RGBA, GL_LINEAR, GL_LINEAR);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{	
		Asset::loadShader(scene, "Imgui", "imgui");
	}

	////////////////////////////////////////////////////////////////////////////////
	void initTextures(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Asset::loadTexture(scene, "Textures/GUI/trash_black.png", "Textures/GUI/trash_black.png");
		Asset::loadTexture(scene, "Textures/GUI/trash_white.png", "Textures/GUI/trash_white.png");
		Asset::loadTexture(scene, "Textures/GUI/x_black.png", "Textures/GUI/x_black.png");
		Asset::loadTexture(scene, "Textures/GUI/x_white.png", "Textures/GUI/x_white.png");
		Asset::loadTexture(scene, "Textures/GUI/list_black.png", "Textures/GUI/list_black.png");
		Asset::loadTexture(scene, "Textures/GUI/list_white.png", "Textures/GUI/list_white.png");
		Asset::loadTexture(scene, "Textures/GUI/checkmark-512.png", "Textures/GUI/checkmark-512.png");
		Asset::loadTexture(scene, "Textures/GUI/x-mark-512.png", "Textures/GUI/x-mark-512.png");
	}

	////////////////////////////////////////////////////////////////////////////////
	void initFonts(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Font sizes
		const size_t textFontSize = 32;
		const size_t iconFontSize = 32;

		// External fonts
		Asset::loadExternalFont(scene, "Arial", "arialbd.ttf", textFontSize);
		Asset::loadExternalFont(scene, "Segoe UI", "segoeuib.ttf", textFontSize);
		Asset::loadExternalFont(scene, "Roboto", "Fonts/roboto-medium.ttf", textFontSize);
		Asset::loadExternalFont(scene, "Ruda", "Fonts/Ruda-Bold.ttf", textFontSize);
		Asset::loadExternalFont(scene, "Open Sans", "Fonts/OpenSans-Bold.ttf", textFontSize);
		Asset::loadExternalFont(scene, "Open Sans", "Fonts/OpenSans-Bold.ttf", textFontSize);
		Asset::loadExternalFont(scene, "FontAwesome", "Fonts/fontawesome-webfont.ttf", iconFontSize);
		Asset::loadExternalFont(scene, "MaterialIcons", "Fonts/materialdesignicons-webfont.ttf", iconFontSize);
	}

	////////////////////////////////////////////////////////////////////////////////
	void uploadFontAtlas(Scene::Scene& scene, Scene::Object* object)
	{
		Debug::log_debug() << "Uploading imgui fonts" << Debug::end;

		if (scene.m_textures.find("ImguiFont") != scene.m_textures.end())
		{
			glDeleteTextures(1, &scene.m_textures["ImguiFont"].m_texture);
		}

		// Extract the default imgui font
		int width, height;
		unsigned char* pixels;
		ImGuiIO& io{ ImGui::GetIO() };
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		// Upload the texture data
		Scene::createTexture(scene, "ImguiFont", GL_TEXTURE_2D, width, height, 1,
			GL_RGBA8, GL_RGBA, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
			GL_UNSIGNED_BYTE, pixels, GL_TEXTURE0);

		// Also store it in the global ImGui context
		io.Fonts->TexID = &(scene.m_textures["ImguiFont"].m_texture);

		Debug::log_debug() << "Successfully uploaded imgui fonts" << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Access the render settings option
		Scene::Object* renderSettings = filterObjects(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS)[0];

		// Gui persistent state file name
		object.component<GuiSettings::GuiSettingsComponent>().m_guiStateFileName = (EnginePaths::configFilesFolder() / "gui_state.ini").string();

		// Find out the larger monitor dimension
		glm::ivec2 monitorRes = Context::getMonitorResolution(scene.m_context);
		const float maxDim = float(glm::max(monitorRes.x, monitorRes.y));

		// DPI scale factor (using 4K as reference)
		object.component<GuiSettings::GuiSettingsComponent>().m_dpiScale = glm::sqrt(maxDim / 3840.0f);

		// Initialize imgui.
		ImGuiIO& io = ImGui::GetIO();

		// Set some global settings
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Set the config file path
		static const std::string s_iniFileName = (EnginePaths::configFilesFolder() / "imgui.ini").string();
		io.IniFilename = s_iniFileName.c_str();

		// Set up the keymap
		io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
		io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
		io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
		io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
		io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
		io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
		io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
		io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
		io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

		// Null the draw list function (we call draw ourself)
		//io.RenderDrawListsFn = nullptr;

		// Store the display size
		io.DisplaySize = ImVec2(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution.x, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution.y);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

		// Determine the font scaling
		if (object.component<GuiSettings::GuiSettingsComponent>().m_fontScale == 0.0f)
		{
			object.component<GuiSettings::GuiSettingsComponent>().m_fontScale = object.component<GuiSettings::GuiSettingsComponent>().m_dpiScale * 0.71f;
		}

		io.FontGlobalScale = object.component<GuiSettings::GuiSettingsComponent>().m_fontScale;

		// Determine plot line weighting
		if (object.component<GuiSettings::GuiSettingsComponent>().m_plotLineWeight == 0.0f)
		{
			object.component<GuiSettings::GuiSettingsComponent>().m_plotLineWeight = object.component<GuiSettings::GuiSettingsComponent>().m_dpiScale * 2.0f;
		}

		ImPlot::GetStyle().LineWeight = object.component<GuiSettings::GuiSettingsComponent>().m_plotLineWeight;

		// Generate the custom styles
		for (int i = 0; i < ImGui::GetNumStyles(); ++i)
		{
			ImGuiStyle style;
			const char* name = ImGui::StyleColorsById(i, &style, object.component<GuiSettings::GuiSettingsComponent>().m_dpiScale);
			scene.m_guiStyles[name] = style;
		}

		// Get the available plot color maps
		ImPlotContext* gp = ImPlot::GetCurrentContext();
		for (int i = 0; i < ImPlot::GetColormapCount(); ++i)
		{
			const char* name = ImPlot::GetColormapName(i);
			scene.m_plotColorMaps[name] = i;
		}

		// Store the filter names
		object.component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.m_validComponents = Debug::InMemoryLogEntry::s_componentNames;
		object.component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.m_validComponents = Debug::InMemoryLogEntry::s_componentNames;

		// Register the available windows as open
		object.component<GuiSettings::GuiSettingsComponent>().m_guiElements =
		{
			GuiElement{ "Object Editor", &generateObjectsWindow, true },
			GuiElement{ "Stats Window", &generateStatsWindow, true },
			GuiElement{ "Log Output", &generateLogWindow, true },
			GuiElement{ "Profiler", &generateProfilerWindow, true },
			GuiElement{ "Material Editor", &generateMaterialEditorWindow, true },
			GuiElement{ "Shader Inspector", &generateShaderInspectorWindow, true },
			GuiElement{ "Buffer Inspector", &generateBufferInspectorWindow, true },
			GuiElement{ "Texture Inspector", &generateTextureInspectorWindow, true },
			GuiElement{ "Render Output", &generateMainRenderWindow, true },
		};

		// Enable the currently active style
		ImGui::GetStyle() = scene.m_guiStyles[object.component<GuiSettings::GuiSettingsComponent>().m_guiStyle];

		// Append the necessary resource initializers
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Font, initFonts, "Fonts");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initTextures, "Textures");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initFramebuffers, "FrameBuffers");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "OpenGL Shaders");

		// Upload the GUI fonts
		DelayedJobs::postJob(scene, &object, "Upload GUI Fonts", [](Scene::Scene& scene, Scene::Object& object)
		{
			uploadFontAtlas(scene, &object);
		});

		// Restore the gui state
		if (object.component<GuiSettings::GuiSettingsComponent>().m_saveGuiState)
		{
			DelayedJobs::postJob(scene, &object, "Restore Gui State", [](Scene::Scene& scene, Scene::Object& object)
			{
				restoreGuiState(scene, &object);
			});
		}

		// Resize the cached state vector
		object.component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_cachedMessageState.resize(Debug::logger_impl::error_full_inmemory_buffer.m_bufferLength);
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object)
	{
		// Turn the gui
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_F11] == 1)
		{
			object->component<GuiSettings::GuiSettingsComponent>().m_showGui = !object->component<GuiSettings::GuiSettingsComponent>().m_showGui;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		if (ImGui::Combo("Style", object->component<GuiSettings::GuiSettingsComponent>().m_guiStyle, scene.m_guiStyles))
		{
			ImGui::GetStyle() = scene.m_guiStyles[object->component<GuiSettings::GuiSettingsComponent>().m_guiStyle];
		}
		if (ImGui::Combo("Plot Color Map", object->component<GuiSettings::GuiSettingsComponent>().m_plotColorMap, scene.m_plotColorMaps))
		{
			ImPlot::GetStyle().Colormap = scene.m_plotColorMaps[object->component<GuiSettings::GuiSettingsComponent>().m_plotColorMap];
			ImPlot::BustColorCache();
		}
		ImGui::Combo("Font", object->component<GuiSettings::GuiSettingsComponent>().m_font, scene.m_fonts);

		ImGui::SliderFloat("Font Scale", &object->component<GuiSettings::GuiSettingsComponent>().m_fontScale, 0.0f, 4.0f);

		ImGui::SliderFloat2("Main Render Blit Constraints", glm::value_ptr(object->component<GuiSettings::GuiSettingsComponent>().m_blitAspectConstraint), 1.0f, 2.0f);

		if (object->component<GuiSettings::GuiSettingsComponent>().m_saveGuiState)
		{
			ImGui::SliderFloat("Gui State Save Interval", &object->component<GuiSettings::GuiSettingsComponent>().m_saveGuiStateInterval, 0.0f, 60.0f);
		}

		if (ImGui::TreeNode("GUI Flags"))
		{
			ImGui::Checkbox("Show While No Input", &object->component<GuiSettings::GuiSettingsComponent>().m_showGuiNoInput);
			ImGui::Checkbox("Fixed Aspect Ratio", &object->component<GuiSettings::GuiSettingsComponent>().m_blitFixedAspectRatio);
			ImGui::Checkbox("Lock Layout", &object->component<GuiSettings::GuiSettingsComponent>().m_lockLayout);
			ImGui::Checkbox("Group Objects By Type", &object->component<GuiSettings::GuiSettingsComponent>().m_groupObjects);
			ImGui::Checkbox("Hide Disabled Groups", &object->component<GuiSettings::GuiSettingsComponent>().m_hideDisabledGroups);
			ImGui::Checkbox("Persistent Gui State", &object->component<GuiSettings::GuiSettingsComponent>().m_saveGuiState);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("GUI Elements"))
		{
			for (auto& element : object->component<GuiSettings::GuiSettingsComponent>().m_guiElements)
				ImGui::Checkbox(element.m_name.c_str(), &element.m_open);

			ImGui::TreePop();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Gui object header */
	void generateGuiObjectGroupEditor(Scene::Scene& scene, Scene::Object* object)
	{
		// Early out
		if (ImGui::BeginPopup("ObjectGroupEditor") == false) return;
		
		// Extract the group settings object
		auto groupSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

		for (auto& group : groupSettings->component<SimulationSettings::SimulationSettingsComponent>().m_groups)
		{
			bool wasInGroup = SimulationSettings::isObjectInGroup(scene, object, group), inGroup = wasInGroup;
			if (ImGui::Checkbox(group.m_name.c_str(), &inGroup))
			{
				if (wasInGroup)
				{
					SimulationSettings::removeObjectFromGroup(scene, object, group);
				}
				else
				{
					SimulationSettings::addObjectToGroup(scene, object, group);
				}
			}
		}

		ImGui::EndPopup();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool generateObjectHeader(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		// Whether we should keep the object or not
		bool deleteObject = false, showGroupsEditor = false;

		const char* label = object->m_name.c_str();
		bool* results[] = { &deleteObject, &showGroupsEditor, &object->m_enabled };
		const int numControls = object->component<EditorSettings::EditorSettingsComponent>().m_allowDisable ? ARRAYSIZE(results) : 0;
		ImTextureID buttonTextures[] = { &scene.m_textures["Textures/GUI/x_white.png"].m_texture, &scene.m_textures["Textures/GUI/list_white.png"].m_texture, nullptr };
		const char* buttonLabels[] = { nullptr, nullptr, nullptr };
		//ImTextureID buttonTextures[] = { nullptr, nullptr, nullptr };
		//const char* buttonLabels[] = { ICON_IGFD_CANCEL, ICON_IGFD_LIST, nullptr };
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_AllowItemOverlap; 
		if (object->component<EditorSettings::EditorSettingsComponent>().m_headerOpen)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiID id = window->GetID(label);
		bool is_open = ImGui::TreeNodeBehavior(id, flags, label);

		// Context object
		ImGuiContext& context = *GImGui;

		// Hover data backup
		ImGuiLastItemDataBackup last_item_backup;

		// Extract the previous cursor pos and padding
		ImVec2 prev_cursor_pos = ImGui::GetCursorScreenPos();
		float prev_padding = context.Style.FramePadding.y;

		// Rect of the collapsing header
		ImRect header_rect = window->DC.LastItemRect;

		// Size of a label
		ImVec2 empty_label_size = ImGui::CalcTextSize("", NULL, true);

		// Set the new padding size
		context.Style.FramePadding.y = -context.Style.FramePadding.y * 0.5f;

		// Push colors for the close button
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, context.Style.Colors[ImGuiCol_FrameBgActive]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, context.Style.Colors[ImGuiCol_FrameBgHovered]);
		ImGui::PushStyleColor(ImGuiCol_Button, context.Style.Colors[ImGuiCol_FrameBg]);

		// Current draw pos (x of the lower left corner and y of the center
		ImVec2 drawPos = ImVec2(header_rect.Max.x, header_rect.GetCenter().y);

		// Create header controls
		for (int i = 0; i < numControls; ++i)
		{
			// Create a regular button, with text
			if (buttonLabels[i] != nullptr)
			{
				const ImVec2 label_size = ImGui::CalcTextSize(buttonLabels[i], NULL, true);

				ImVec2 button_size = ImVec2(label_size.x + context.Style.FramePadding.x * 2, label_size.y + context.Style.FramePadding.y * 2);
				ImVec2 button_pos = ImVec2(ImMin(drawPos.x, window->ClipRect.Max.x) - context.Style.FramePadding.x - button_size.x, drawPos.y - button_size.y / 2);
				ImGui::SetCursorScreenPos(button_pos);
				*results[i] = ImGui::Button(buttonLabels[i]);
			}

			// Create a textured button
			else if (buttonTextures[i] != nullptr)
			{
				ImVec2 button_size = ImVec2(empty_label_size.y + context.Style.FramePadding.y * 2, empty_label_size.y + context.Style.FramePadding.y * 2);
				ImVec2 button_pos = ImVec2(ImMin(drawPos.x, window->ClipRect.Max.x) - context.Style.FramePadding.x - button_size.x, drawPos.y - button_size.y / 2);
				ImGui::SetCursorScreenPos(button_pos);
				*results[i] = ImGui::ImageButton(buttonTextures[i], button_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), 0, ImVec4(0, 0, 0, 0), context.Style.Colors[ImGuiCol_CheckMark]);
			}

			// Create a checkbox
			else
			{
				// Create the enable checkbox
				ImVec2 button_size = ImVec2(empty_label_size.y + context.Style.FramePadding.y * 2, empty_label_size.y + context.Style.FramePadding.y * 2);
				ImVec2 button_pos = ImVec2(ImMin(drawPos.x, window->ClipRect.Max.x) - context.Style.FramePadding.x - button_size.y, drawPos.y - button_size.y / 2);
				ImGui::SetCursorScreenPos(button_pos);
				ImGui::Checkbox("", results[i]);
			}

			// Update the draw position
			drawPos.x = window->DC.LastItemRect.Min.x;
		}

		// Pop the close button colors
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		// Reset the cursor position and padding
		ImGui::SetCursorScreenPos(prev_cursor_pos);
		context.Style.FramePadding.y = prev_padding;

		// Restore hover data
		last_item_backup.Restore();

		// Store the header open flag
		object->component<EditorSettings::EditorSettingsComponent>().m_headerOpen = is_open;

		// Remove the object if it is no longer needed
		if (deleteObject) removeObject(scene, *object);

		// Open the group editor
		if (showGroupsEditor) ImGui::OpenPopup("ObjectGroupEditor");
		
		return !deleteObject;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Gui object header */
	bool generateGuiObjectHeader(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		// Generate the object header
		if (!generateObjectHeader(scene, guiSettings, object)) return false;

		// Generate the gui object editor
		generateGuiObjectGroupEditor(scene, object);

		// Return the header open status
		return object->component<EditorSettings::EditorSettingsComponent>().m_headerOpen;
	};

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiMenuBar(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Menu Bar");

		bool openDuplicateModal = false;

		// Generate the menu bar
		if (ImGui::BeginMenuBar())
		{
			// Menu
			if (ImGui::BeginMenu("Menu"))
			{
				// ImGui
				ImGui::MenuItem("ImGui", NULL, false, false);
				if (ImGui::MenuItem("Show Demo"))
				{
					EditorSettings::editorProperty<bool>(scene, guiSettings, "GuiSettings_ShowImguiDemo") = true;
				}

				if (ImGui::MenuItem("Show Style Editor"))
				{
					EditorSettings::editorProperty<bool>(scene, guiSettings, "GuiSettings_ShowImguiStyleEditor") = true;
				}
				ImGui::Separator();

				// ImPlot
				ImGui::MenuItem("ImPlot", NULL, false, false);
				if (ImGui::MenuItem("Show Demo"))
				{
					EditorSettings::editorProperty<bool>(scene, guiSettings, "GuiSettings_ShowImplotDemo") = true;
				}
				if (ImGui::MenuItem("Show Style Editor"))
				{
					EditorSettings::editorProperty<bool>(scene, guiSettings, "GuiSettings_ShowImPlotStyleEditor") = true;
				}
				ImGui::Separator();


				if (ImGui::MenuItem("Quit"))
				{
					glfwSetWindowShouldClose(scene.m_context.m_window, GLFW_TRUE);
				}
				ImGui::EndMenu();
			}

			// Add object
			if (ImGui::BeginMenu("Add Object"))
			{
				for (auto objectType: Scene::objectTypes())
				{
					if (ImGui::MenuItem(Scene::objectNames()[objectType].c_str()))
					{
						Scene::Object* object = &createObject(scene, Scene::objectNames()[objectType], objectType);

						object->m_enabled = true;
					}
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool beginObjectGroup(Scene::Scene& scene, Scene::Object* guiSettings, Scene::ObjectType objectType)
	{
		// Whether the header is open or not
		bool headerOpen = false;

		// Color of the category header
		ImVec4 headerColor = ImGui::GetStyle().Colors[ImGuiCol_Header];
		ImVec4 headerColorHovered = ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered];
		ImVec4 headerColorActive = ImGui::GetStyle().Colors[ImGuiCol_HeaderActive];

		ImVec4 headerColorHSV = ImGui::ColorConvertRGBtoHSV(headerColor);
		ImVec4 headerColorHoveredHSV = ImGui::ColorConvertRGBtoHSV(headerColorHovered);
		ImVec4 headerColorActiveHSV = ImGui::ColorConvertRGBtoHSV(headerColorActive);

		headerColorHSV.z *= 0.55f;
		headerColorHoveredHSV.z *= 0.55f;
		headerColorActiveHSV.z *= 0.55f;

		ImVec4 headerColorRGB = ImGui::ColorConvertHSVtoRGB(headerColorHSV);
		ImVec4 headerColorHoveredRGB = ImGui::ColorConvertHSVtoRGB(headerColorHoveredHSV);
		ImVec4 headerColorActiveRGB = ImGui::ColorConvertHSVtoRGB(headerColorActiveHSV);

		// Push the header color
		ImGui::PushStyleColor(ImGuiCol_Header, headerColorRGB);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerColorHoveredRGB);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, headerColorActiveRGB);

		// Open the category header
		headerOpen = ImGui::CollapsingHeader(Scene::objectNames()[objectType].c_str());

		// Pop the header color
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		return headerOpen;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateObjectsWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Objects Editor");

		// Extract the simulation settings object
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

		// Count the number of objects per category
		std::unordered_map<std::string, int> guiObjectCounts;
		for (auto category : Scene::objectGuiCategories())
		{
			guiObjectCounts[category.second] += Scene::filterObjects(scene, category.first, true, true).size();
		}

		// Get the name of the focused window
		std::string focused;

		// Skip generating the stats window in the first couple of frames; this avoids a number of weird behaviours
		if (simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId >= 3)
		{
			if (auto focusedSync = EditorSettings::consumeEditorProperty<std::string>(scene, guiSettings, "FocusedWindow#Synced"); focusedSync.has_value())
			{
				focused = focusedSync.value();
			}
		}

		// Generate each category's window
		for (auto const& countIt: guiObjectCounts)
		{
			// Extract the category string and object count
			auto [category, count] = countIt;

			// Skip empty categories
			if (count == 0)
				continue;

			// Set the window settings
			ImGui::SetNextWindowPos(ImVec2(100, 600), ImGuiCond_FirstUseEver);
			std::string windowName = category;
			ImGuiWindowFlags flags = ImGuiWindowFlags_None | ImGuiWindowFlags_AlwaysAutoResize;
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

			// Focus on the next window
			if (focused == category)
				ImGui::SetNextWindowFocus();

			// Open the window
			ImGui::SetNextWindowSize(ImVec2(250, 350), ImGuiCond_FirstUseEver);
			if (ImGui::Begin(windowName.c_str(), nullptr, flags))
			{
				// Store the window focus status
				if (ImGui::IsWindowFocused())
				{
					EditorSettings::editorProperty<std::string>(scene, guiSettings, "FocusedWindow") = windowName;
				}

				// Generate GUI elements for the various object types
				for (auto it : Scene::objectGuiGenerators())
				{
					// Extaxt the data
					auto [objectType, objectGuiFunction] = it;

					// Only generate objects belonging to the current category
					if (Scene::objectGuiCategories()[objectType] != category)  continue;

					// List of all the objects in this category
					auto const& objects = Scene::filterObjects(scene, objectType, true, true);

					// Skip empty groups
					if (objects.empty()) continue;

					Profiler::ScopedCpuPerfCounter perfCounter(scene, Scene::objectNames()[objectType]);

					// Determine whether we need a collapsing header for multiple objects
					if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_groupObjects == false || beginObjectGroup(scene, guiSettings, objectType))
					{
						// Go through each object of the corresponding object type
						for (auto object : objects)
						{
							Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

							// Skip the object if its group is not enabled
							if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_hideDisabledGroups == true &&
								SimulationSettings::isObjectEnabledByGroups(scene, object) == false)
								continue;

							// Pust the name of the object as a unique scope ID
							ImGui::PushID(object->m_name.c_str());

							// Generate the header of the object
							if (generateGuiObjectHeader(scene, guiSettings, object))
							{
								Debug::DebugRegion debugRegion(object->m_name);

								// invoke its GUI generator callback
								objectGuiFunction(scene, guiSettings, object);
							}

							// Pop the object ID
							ImGui::PopID();
						}
					}
				}

				// Handle the scroll bar persistence
				std::string scrollBarKey = category + "_Window_ScrollY";
				if (auto synced = EditorSettings::consumeEditorProperty<float>(scene, guiSettings, scrollBarKey + "#Synced"); synced.has_value())
				{
					ImGui::SetScrollY(synced.value());
				}
				EditorSettings::editorProperty<float>(scene, guiSettings, scrollBarKey) = ImGui::GetScrollY();
			}
			ImGui::End();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	#define DRAGDROP_PAYLOAD_TYPE_PROFILER_CATEGORY     "PROFILER_CATEGORY"

	namespace StatsWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		void makeDragDropSource(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTreeIterator node)
		{
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload(DRAGDROP_PAYLOAD_TYPE_PROFILER_CATEGORY, node->m_category.c_str(), node->m_category.size() + 1);
				ImGui::Text(node->m_name.c_str());
				ImGui::EndDragDropSource();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Cursor offsets for the various columns
		static constexpr std::array<const char*, 6> s_columnNames =
		{
			"Name",
			"Current",
			"Min",
			"Avg",
			"Max",
			"Count",
		};

		////////////////////////////////////////////////////////////////////////////////
		void generateNode(Scene::Scene& scene, Scene::Object* guiSettings, size_t depth,
			Profiler::ProfilerThreadTree const& tree, Profiler::ProfilerThreadTreeIterator root)
		{
			// List of open nodes
			auto& openNodes = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_statWindowSettings.m_openNodes;

			// Iterate over the children
			for (auto it = tree.begin(root); it != tree.end(root); ++it)
			{
				// Compute the number of child entries
				int numChildren = std::distance(tree.begin(it), tree.end(it));

				// Extract the current node's data
				auto const& entry = Profiler::dataEntry(scene, *it);

				// Extract the current value
				std::array<std::string, s_columnNames.size()> values =
				{
					numChildren > 0 ? it->m_name + " (" + std::to_string(numChildren) + ")" : it->m_name,
					Profiler::convertEntryData<std::string>(entry.m_current),
					Profiler::convertEntryData<std::string>(entry.m_min),
					Profiler::convertEntryData<std::string>(entry.m_avg),
					Profiler::convertEntryData<std::string>(entry.m_max),
					std::to_string(entry.m_totalEntryCount)
				};

				// Do we need to do this recursively
				bool nodeOpen = numChildren > 0;

				// Start with the node name
				ImGui::TableNextColumn();
				if (numChildren > 0)
				{
					// Was the node previously open
					auto nodeit = std::find(openNodes.begin(), openNodes.end(), it->m_category);
					const bool wasNodeOpen = nodeit != openNodes.end();

					// Generate the treenode header
					nodeOpen = ImGui::TreeNodeEx(values[0].c_str(), ImGuiTreeNodeFlags_SpanFullWidth | (wasNodeOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None));

					// Update the open-flag of the node
					if (nodeOpen == true && wasNodeOpen == false)
						openNodes.push_back(it->m_category);
					else if (nodeOpen == false && wasNodeOpen == true)
						openNodes.erase(nodeit);
				}
				else
				{
					// Create the selectable for the item
					ImGui::TreeNodeEx(values[0].c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
				}

				// Enable drag-drop for the profiler window
				makeDragDropSource(scene, guiSettings, it);

				// Tooltip
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(values[0].c_str());

				// Display the properties of the node
				for (int i = 1; i < values.size(); ++i)
				{
					ImGui::TableNextColumn();
					ImGui::Selectable(values[i].c_str());
					if (ImGui::IsItemHovered()) ImGui::SetTooltip(values[i].c_str());
				}

				// Recursion if the node has children
				if (nodeOpen)
				{
					ImGui::PushID(it->m_name.c_str());
					generateNode(scene, guiSettings, depth + 1, tree, it);
					ImGui::PopID();
					ImGui::TreePop();
				}
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateTree(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// The tree to display
			auto const& profilerTree = scene.m_profilerTree[scene.m_profilerBufferReadId][0];

			// Table flags
			const ImGuiTableFlags tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit |
				ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
				ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;

			// Generate the table
			if (ImGui::BeginTable("StatsTable", s_columnNames.size(), tableFlags))
			{
				// Setup the columns
				ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
				for (int i = 0; i < s_columnNames.size(); ++i)
					ImGui::TableSetupColumn(s_columnNames[i]);
				ImGui::TableHeadersRow();

				// Traverse the tree by starting at the root node
				generateNode(scene, guiSettings, 0, profilerTree, profilerTree.begin());

				ImGui::EndTable();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateStatsWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		// Extract the simulation settings object
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

		// Skip generating the stats window in the first couple of frames; this avoids a number of weird behaviours
		if (simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId < 3)
			return;

		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Stats Window");

		// Create the window
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		if (ImGui::Begin("Statistics", nullptr, flags))
		{
			// Generate the settings editor
			if (ImGui::TreeNode("Settings"))
			{				
				ImGui::SliderFloat("Indentation Scaling", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_statWindowSettings.m_indentScale, 0.0f, 2.0f);

				ImGui::TreePop();
			}

			// Generate the tree
			StatsWindow::generateTree(scene, guiSettings);

			// Handle the scroll bar persistence
			if (auto synced = EditorSettings::consumeEditorProperty<float>(scene, guiSettings, "StatsWindow_ScrollY#Synced"); synced.has_value())
			{
				ImGui::SetScrollY(synced.value());
			}
			EditorSettings::editorProperty<float>(scene, guiSettings, "StatsWindow_ScrollY") = ImGui::GetScrollY();
		}
		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace ProfilerWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		struct Payload
		{
			// Necessary scene information
			Scene::Object* m_guiSettings;

			// Iterator to the node
			Profiler::ProfilerThreadTreeIterator m_iterator;

			// Data of the node
			std::reference_wrapper<Profiler::ProfilerDataEntry> m_entry;

			Payload(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTreeIterator iterator) :
				m_iterator(iterator),
				m_entry(Profiler::dataEntry(scene, *iterator)),
				m_guiSettings(guiSettings)
			{}
		};

		////////////////////////////////////////////////////////////////////////////////
		std::string formatValue(Profiler::ProfilerDataEntry const& entry, float val)
		{
			if (Profiler::isEntryType<Profiler::ProfilerDataEntry::EntryDataFieldTime>(entry.m_min) == false)
			{
				return std::to_string(val);
			}
			return Units::secondsToString(Units::millisecondsToSeconds(val));
		}

		////////////////////////////////////////////////////////////////////////////////
		int getFrameId(Scene::Object* guiSettings, int index)
		{
			return guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_startFrameId + index;
		}

		////////////////////////////////////////////////////////////////////////////////
		int getFrameId(Payload const& payload, int index)
		{
			return getFrameId(payload.m_guiSettings, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorCurrent(void* pPayload, int index)
		{
			// Extract the payload
			Payload const& payload = *(const Payload*)pPayload;

			// Id of the current frame
			const int frameId = getFrameId(payload, index);

			// Look the the specified frame's data
			if (const auto it = payload.m_entry.get().m_previousValues.find(frameId); it != payload.m_entry.get().m_previousValues.end())
			{
				// Return the current value
				return ImPlotPoint(frameId, Profiler::convertEntryData<float>(it->second));
			}

			// Fall back to zero if it's not stored
			return ImPlotPoint(frameId, 0.0f);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorAvgSliding(void* pPayload, int index)
		{
			// Extract the payload
			Payload const& payload = *(const Payload*)pPayload;

			// Id of the current frame
			const int frameId = getFrameId(payload, index);

			// Size of the average window
			const int windowSize = payload.m_guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgWindowSize;

			// Return the min value
			return ImPlotPoint(frameId, Profiler::slidingAverageWindowed<float>(payload.m_entry.get(), frameId, windowSize, false));
		}

		////////////////////////////////////////////////////////////////////////////////
		void tooltipGenerator(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTreeIterator node, int index)
		{
			// Extract the simulation settings object
			Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

			// Extract the data entry for the input node
			Profiler::ProfilerDataEntry const& entry = Profiler::dataEntry(scene, *node);

			// Id of the current frame
			const int frameId = getFrameId(guiSettings, index);

			// Size of the average window
			const int windowSize = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgWindowSize;

			// Look the the specified frame's data
			auto it = entry.m_previousValues.find(frameId);

			// Float values
			const float cur = it == entry.m_previousValues.end() ? 0.0f : Profiler::convertEntryData<float>(it->second);
			const float min = Profiler::slidingMinWindowed<float>(entry, frameId, windowSize);
			const float avg = Profiler::slidingAverageWindowed<float>(entry, frameId, windowSize, false);
			const float max = Profiler::slidingMaxWindowed<float>(entry, frameId, windowSize);

			// String values
			const std::string frameStartTime = Units::minutesToString(Units::secondsToMinutes(
				simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameStartTimepoint[frameId]));
			const std::string curStr = formatValue(entry, cur);
			const std::string minStr = formatValue(entry, min);
			const std::string avgStr = formatValue(entry, avg);
			const std::string maxStr = formatValue(entry, max);
			const std::string countStr = std::to_string(entry.m_currentCount);
			const std::string totalCountStr = std::to_string(entry.m_totalEntryCount);

			// Generate the tooltip
			ImGui::BeginTooltip();
			ImGui::Text("Time: %s (Frame #%d)", frameStartTime.c_str(), frameId);
			if (ImGui::BeginTable("ProfilerEntryTooltip", 2))
			{
				ImGui::TableNextColumn(); ImGui::BulletText("Current:"); ImGui::TableNextColumn(); ImGui::Text(curStr.c_str());
				ImGui::TableNextColumn(); ImGui::BulletText("Min:"); ImGui::TableNextColumn(); ImGui::Text(minStr.c_str());
				ImGui::TableNextColumn(); ImGui::BulletText("Average:"); ImGui::TableNextColumn(); ImGui::Text(avgStr.c_str());
				ImGui::TableNextColumn(); ImGui::BulletText("Max:"); ImGui::TableNextColumn(); ImGui::Text(maxStr.c_str());
				ImGui::TableNextColumn(); ImGui::BulletText("Entries:"); ImGui::TableNextColumn(); ImGui::Text("%s (%s)", countStr.c_str(), totalCountStr.c_str());
				ImGui::EndTable();
			}
			ImGui::EndTooltip();
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getLabel(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTreeIterator node)
		{
			switch (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodeLabelMode)
			{
			case ProfilerChartsSettings::CurrentCategory:
				return node->m_name;

			case ProfilerChartsSettings::ObjectCategory:
			{
				std::string parentName;
				int parentOffset = 0;
				for (auto const& object : scene.m_objects)
				{
					int offset = node->m_category.find(object.first);
					if (offset != std::string::npos && offset > parentOffset)
					{
						parentName = object.first;
						parentOffset = offset;
					}
				}
				return parentName.empty() ? node->m_name : node->m_category.substr(parentOffset);
			}

			case ProfilerChartsSettings::FullCategory:
				return node->m_category;
			}

			return node->m_name;
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateLabel(Scene::Scene& scene, Scene::Object* guiSettings, std::string label)
		{
			// Replace the separators with a single one
			std::string_replace_all(label, "::", ":");

			// Go through each category
			std::istringstream iss(label);
			bool first = true;
			for (std::string category; std::getline(iss, category, ':'); )
			{
				if (first == false)
				{
					ImGui::SameLine();
					ImGui::Arrow(1.0f, ImGuiDir_Right);
					ImGui::SameLine();
				}
				ImGui::Text(category.c_str());
				first = false;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string generateOverview(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTreeIterator node)
		{
			// Extract the simulation settings object
			Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

			// Extract the data entry for the input node
			Profiler::ProfilerDataEntry const& entry = Profiler::dataEntry(scene, *node);

			// Start frame id and number of frames
			const int startFrameId = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_startFrameId;
			const int endFrameId = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_endFrameId;
			const int numFrames = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow;

			// Extract the current frame time
			const int currFrameId = glm::min(simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId - 1, startFrameId + numFrames - 1);
			auto currIt = entry.m_previousValues.find(currFrameId);

			// Min, avg and max values (sliding)
			const float cur = (currIt == entry.m_previousValues.end()) ? 0.0f : Profiler::convertEntryData<float>(currIt->second);
			const float min = Profiler::slidingMin<float>(entry, startFrameId, endFrameId);
			const float avg = Profiler::slidingAverage<float>(entry, startFrameId, endFrameId, false);
			const float max = Profiler::slidingMax<float>(entry, startFrameId, endFrameId);

			// Construct the final overlay string
			std::stringstream overlaystream;
			overlaystream
				<< std::setw(9) << formatValue(Profiler::dataEntry(scene, *node), cur) << " ("
				<< "Min: " << std::setw(9) << formatValue(Profiler::dataEntry(scene, *node), min) << "," << std::string(4, ' ')
				<< "Avg: " << std::setw(9) << formatValue(Profiler::dataEntry(scene, *node), avg) << "," << std::string(4, ' ')
				<< "Max: " << std::setw(9) << formatValue(Profiler::dataEntry(scene, *node), max) << ")" << std::endl;
			return overlaystream.str();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateChart(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTreeIterator node, size_t depth, size_t threadId)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, node->m_category);

			// Height of the graph
			const float graphHeight = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_graphHeight;

			// Push the node name as an ID
			ImGui::PushID(node->m_name.c_str());

			// Delete the plot
			const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding.y * 0.5f, framePadding.y * 0.5f));
			const float buttonSize = ImGui::GetFontSize() - 2.0f * ImGui::GetStyle().FramePadding.y;
			if (ImGui::ImageButton(&scene.m_textures["Textures/GUI/x_white.png"].m_texture, ImVec2(buttonSize, buttonSize), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), -1))
			{
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow.erase(node->m_category);
			}
			ImGui::PopStyleVar();

			ImGui::SameLine();

			// Node open flag
			bool& nodeOpen = EditorSettings::editorProperty<bool>(scene, guiSettings, "ProfilerChartOpen_" + node->m_category);

			// Treenode flags for the chart
			const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen |
				(nodeOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);

			// Generate the tree node header
			nodeOpen = ImGui::TreeNodeEx("##label", flags);
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				generateLabel(scene, guiSettings, node->m_category);
				ImGui::EndTooltip();
			}

			// Generate the tree node label
			ImGui::SameLine();
			generateLabel(scene, guiSettings, getLabel(scene, guiSettings, node));

			// Size of the content area
			const float contentAreaHeight = ImGui::GetFontSize() + 2.0f * ImGui::GetStyle().WindowPadding.y + 
				(nodeOpen ? graphHeight + ImGui::GetStyle().ItemSpacing.y : 0.0f);

			// Generate the content area
			if (ImGui::BeginChild("ContentArea", ImVec2(0.0f, contentAreaHeight), true))
			{
				// Generate the overview text
				const std::string overview = generateOverview(scene, guiSettings, node);
				ImGui::TextAlign(overview.c_str(), nodeOpen ? ImGuiTextAlignment_Center : ImGuiTextAlignment_Left);

				// Generate the chart itself
				if (nodeOpen)
				{
					// Axis flags
					const ImPlotFlags plotFlags = ImPlotFlags_AntiAliased | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_NoTitle;
					const ImPlotAxisFlags xFlags = ImPlotAxisFlags_AutoFit;
					const ImPlotAxisFlags yFlags = ImPlotAxisFlags_AutoFit;

					if (ImPlot::BeginPlot(node->m_category.c_str(), nullptr, nullptr, ImVec2(-1, graphHeight), plotFlags, xFlags, yFlags))
					{
						// Node generator payload
						Payload payload(scene, guiSettings, node);

						// How many frames to show
						const int numFrames = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow;

						// Avg line
						ImPlot::SetNextLineStyle(ImGui::ColorFromGlmVector(
							guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgPlotColor));
						ImPlot::PlotLineG("Average", &dataGeneratorAvgSliding, &payload, numFrames);

						// Current line
						ImPlot::SetNextLineStyle(ImGui::ColorFromGlmVector(
							guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_plotColor));
						ImPlot::PlotLineG("Current", &dataGeneratorCurrent, &payload, numFrames);

						// Handle tooltips
						if (ImPlot::IsPlotHovered())
						{
							// Mouse coordinates in plot coordinates
							const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
							ImPlot::MouseHitLineResult mouseHit = ImPlot::FindMouseHitLineG(&dataGeneratorCurrent, &payload, numFrames, mouse);

							// Generate the tooltip if the index is valid
							if (mouseHit.m_inside)
							{
								// Add the highlighter
								ImPlot::HighlightMouseHitLineG(mouseHit, ImGui::Color32FromGlmVector(
									guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_plotColor));

								// Generate tooltip
								tooltipGenerator(scene, guiSettings, node, mouseHit.m_index);
							}
						}

						ImPlot::EndPlot();
					}
				}
			}
			ImGui::EndChild();


			// Get rid of the ID
			ImGui::PopID();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateNode(Scene::Scene& scene, Scene::Object* guiSettings, Profiler::ProfilerThreadTree const& tree, Profiler::ProfilerThreadTreeIterator root, size_t depth, size_t threadId)
		{
			//Profiler::ScopedCpuPerfCounter perfCounter(scene, "Profiler Node", true);

			// Iterate over the children
			for (auto it = tree.begin(root); it != tree.end(root); ++it)
			{
				// Whether this chart should be drawn or not
				bool drawChart = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow.count(it->m_category);

				// Generate the corresponding chart
				if (drawChart)
				{
					ImGui::PushID(it->m_category.c_str());
					generateChart(scene, guiSettings, it, depth, threadId);
					ImGui::PopID();
				}

				// Keep traversing
				generateNode(scene, guiSettings, tree, it, depth + 1, threadId);
			};
		}
	}	

	////////////////////////////////////////////////////////////////////////////////
	void generateProfilerWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Profiler Window");

		// Find the simulation settings object
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

		// Skip generating the stats window in the first couple of frames; this avoids a number of weird behaviours
		if (simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId < 3)
			return;
		
		// Flags for the window
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		// Create the window
		if (ImGui::Begin("Profiler", nullptr, flags))
		{
			// Generate the settings editor
			if (ImGui::TreeNode("Settings"))
			{
				ImGui::SliderFloat("Graph Height", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_graphHeight, 100.0f, 320.0f);
				ImGui::Combo("Node Label", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodeLabelMode, ProfilerChartsSettings::NodeLabelMode_meta);
				ImGui::SliderInt("Number of Frames", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow, 1, 256);
				ImGui::SliderInt("Update Ratio", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_updateRatio, 1, 256);
				ImGui::SliderInt("Average Window Size", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgWindowSize, 1, 128);

				ImGui::ColorEdit4("Plot Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_plotColor));
				ImGui::ColorEdit4("Avg Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgPlotColor));

				ImGui::Checkbox("Freeze Output", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_freeze);

				ImGui::TreePop();
			}

			// Compute the ID of the new start frame
			int newFrameStartId = glm::max(0, simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId - guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow);
			if (newFrameStartId >= guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_lastFrameDrawn + guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_updateRatio && 
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_freeze == false)
			{
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_lastFrameDrawn = newFrameStartId;
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_startFrameId = newFrameStartId;
			}
			guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_endFrameId = glm::min(
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_startFrameId + guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow - 1, 
				simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId - 2);

			// Begin the content area
			if (ImGui::BeginChild("Charts", ImVec2(0.0f, 0.0f), true))
			{
				if (guiSettings->component<GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow.size())
				{
					// Traverse the tree by starting at the root node
					ProfilerWindow::generateNode(scene, guiSettings, scene.m_profilerTree[scene.m_profilerBufferReadId][0], 
						scene.m_profilerTree[scene.m_profilerBufferReadId][0].begin(), 0, 0);
				}

				// Handle the scroll bar persistence
				std::string scrollBarKey = "ProfilerWindow_ScrollY";
				if (auto synced = EditorSettings::consumeEditorProperty<float>(scene, guiSettings, scrollBarKey + "#Synced"); synced.has_value())
				{
					ImGui::SetScrollY(synced.value());
				}
				EditorSettings::editorProperty<float>(scene, guiSettings, scrollBarKey) = ImGui::GetScrollY();
			}
			ImGui::EndChild();

			// Drag and drop support
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(DRAGDROP_PAYLOAD_TYPE_PROFILER_CATEGORY))
				{
					std::vector<char> category(payload->DataSize);
					memcpy(category.data(), payload->Data, payload->DataSize);
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow.insert(std::string(category.data()));
				}

				ImGui::EndDragDropTarget();
			}
		}
		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace LogWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		Debug::InMemoryLogBuffer const& getBuffer(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDebug)   return Debug::logger_impl::debug_full_inmemory_buffer;
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showTrace)   return Debug::logger_impl::trace_full_inmemory_buffer;
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showInfo)    return Debug::logger_impl::info_full_inmemory_buffer;
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showWarning) return Debug::logger_impl::warning_full_inmemory_buffer;
			return Debug::logger_impl::error_full_inmemory_buffer;
		}

		////////////////////////////////////////////////////////////////////////////////
		ImVec4 getEntryColor(Scene::Scene& scene, Scene::Object* guiSettings, Debug::InMemoryLogEntry const& message)
		{
			glm::vec3 color = glm::vec3(1.0f);

			if (message.m_sourceLog == Debug::log_debug().m_ref.get().m_name)   color = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_debugColor;
			if (message.m_sourceLog == Debug::log_trace().m_ref.get().m_name)   color = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_traceColor;
			if (message.m_sourceLog == Debug::log_info().m_ref.get().m_name)    color = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_infoColor;
			if (message.m_sourceLog == Debug::log_warning().m_ref.get().m_name) color = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_warningColor;
			if (message.m_sourceLog == Debug::log_error().m_ref.get().m_name)   color = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_errorColor;

			return ImGui::ColorFromGlmVector(color);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool filterEntry(Scene::Scene& scene, Scene::Object* guiSettings, Debug::InMemoryLogBuffer const& buffer, int messageId, Debug::InMemoryLogEntry const& message)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Filtering", true);

			// Load the cached state
			auto& msgState = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_cachedMessageState[messageId];

			// Use the cached data, if able
			if (msgState.m_source == &buffer && msgState.m_date == message.m_dateEpoch && msgState.m_filterTime == guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_lastFilterUpdate)
				return msgState.m_visible;

			bool result = true;
			if (message.m_sourceLog == Debug::log_debug().m_ref.get().m_name)   result &= guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDebug;
			if (message.m_sourceLog == Debug::log_trace().m_ref.get().m_name)   result &= guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showTrace;
			if (message.m_sourceLog == Debug::log_info().m_ref.get().m_name)    result &= guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showInfo;
			if (message.m_sourceLog == Debug::log_warning().m_ref.get().m_name) result &= guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showWarning;
			if (message.m_sourceLog == Debug::log_error().m_ref.get().m_name)   result &= guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showError;

			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.m_empty == false)
			{
				result = result && (
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("Source", message.m_sourceLog) ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("Region", message.m_region) ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("Date", message.m_date) ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("Function", message.m_sourceFunction) ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("File", message.m_sourceFile) ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("Line", message.m_sourceLine) ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter.test("Message", message.m_message)
				);
			}

			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.m_empty == false)
			{
				result = result && (
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("Source", message.m_sourceLog) &&
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("Region", message.m_region) &&
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("Date", message.m_date) &&
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("Function", message.m_sourceFunction) &&
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("File", message.m_sourceFile) &&
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("Line", message.m_sourceLine) &&
					!guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter.test("Message", message.m_message)
				);
			}

			// Update the cached data
			msgState.m_source = &buffer;
			msgState.m_date = message.m_dateEpoch;
			msgState.m_visible = result;
			msgState.m_filterTime = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_lastFilterUpdate;

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateEntry(Scene::Scene& scene, Scene::Object* guiSettings, Debug::InMemoryLogEntry const& message)
		{
			// Text size 
			ImVec2 severityTextSize = ImGui::CalcTextSize("#WARNING#");

			// Whether the tooltip should be shown or not
			bool showTooltip = false;

			// Push the message color
			ImGui::PushStyleColor(ImGuiCol_Text, getEntryColor(scene, guiSettings, message));

			// Spacing to apply
			const float spacing = severityTextSize.y * guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_messageSpacing;

			// Add some vertical space between the entries
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);

			// Generate the severity
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showRegion)
			{
				float colWidth = 0.0f;
				ImGui::TableNextColumn();
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate)
				{
					ImGui::Text(message.m_date.c_str());
					showTooltip = showTooltip || ImGui::IsItemHovered();
				}
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity)
				{
					ImGui::Text(message.m_sourceLog.c_str());
					showTooltip = showTooltip || ImGui::IsItemHovered();
				}
			}
			else
			{
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate)
				{
					ImGui::TableNextColumn();
					ImGui::Text(message.m_date.c_str());
					showTooltip = showTooltip || ImGui::IsItemHovered();
				}
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity)
				{
					ImGui::TableNextColumn();
					ImGui::Text(message.m_sourceLog.c_str());
					showTooltip = showTooltip || ImGui::IsItemHovered();
				}
			}

			// Generate the actual log message
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
			ImGui::TableNextColumn();
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showRegion)
			{
				ImGui::Text(message.m_region.c_str());
				showTooltip = showTooltip || ImGui::IsItemHovered();
			}

			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_longMessages)
			{
				ImGui::TextWrapped(message.m_message.c_str());
			}
			else
			{
				std::string clippedMessage = message.m_message;
				auto it = clippedMessage.find_first_of('\n');
				if (it != std::string::npos) clippedMessage = clippedMessage.substr(0, it);

				ImGui::Text(clippedMessage.c_str());
			}
			showTooltip = showTooltip || ImGui::IsItemHovered();
			ImGui::PopStyleColor();

			// Generate the tooltip
			if (showTooltip)
			{
				std::filesystem::path sourceFile = message.m_sourceFile;
				std::string sourceFileRelative = std::filesystem::relative(sourceFile).string();
				std::string regionString = message.m_region;
				std::string_replace_all(regionString, "][", "::");
				std::string_replace_all(regionString, "[", "");
				std::string_replace_all(regionString, "]", "");

				ImGui::SetNextWindowSize(ImVec2(800.0f, -1.0f));
				ImGui::BeginTooltip();
				if (ImGui::BeginTable("LogEntryTooltip", 2))
				{
					ImGui::TableNextColumn(); ImGui::Text("Date:"); ImGui::TableNextColumn(); ImGui::Text("%s", message.m_date.c_str());
					ImGui::TableNextColumn(); ImGui::Text("Severity:"); ImGui::TableNextColumn(); ImGui::Text("%s", message.m_sourceLog.c_str());
					ImGui::TableNextColumn(); ImGui::Text("Region:"); ImGui::TableNextColumn(); ImGui::Text("%s", regionString.c_str());
					ImGui::TableNextColumn(); ImGui::Text("Thread:"); ImGui::TableNextColumn(); ImGui::Text("%s", message.m_threadId.c_str());
					ImGui::TableNextColumn(); ImGui::Text("Source:"); ImGui::TableNextColumn(); ImGui::Text("%s", message.m_sourceFunction.c_str());
					ImGui::TableNextColumn(); ImGui::Text(""); ImGui::TableNextColumn(); ImGui::TextWrapped("[%s (%s)]", sourceFileRelative.c_str(), message.m_sourceLine.c_str());
					ImGui::EndTable();
				}
				ImGui::Separator();
				ImGui::TextWrapped(message.m_message.c_str());
				ImGui::EndTooltip();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateClipped(Scene::Scene& scene, Scene::Object* guiSettings, Debug::InMemoryLogBuffer const& buffer)
		{
			// Extract the visible message ids
			std::vector<size_t> visibleMessages;
			visibleMessages.reserve(buffer.m_numMessages);
			for (size_t i = 0; i < buffer.m_numMessages; ++i)
			{
				const int messageId = (buffer.m_startMessageId + i) % buffer.m_bufferLength;
				Debug::InMemoryLogEntry const& message = buffer.m_messages[messageId];
				if (filterEntry(scene, guiSettings, buffer, messageId, message))
					visibleMessages.push_back(i);
			}

			// Show the clipped list
			ImGuiListClipper clipper;
			clipper.Begin(visibleMessages.size());
			while (clipper.Step())
				for (size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
					generateEntry(scene, guiSettings, buffer.m_messages[visibleMessages[i]]);

			// Finish clipping
			clipper.End();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateSlow(Scene::Scene& scene, Scene::Object* guiSettings, Debug::InMemoryLogBuffer const& buffer)
		{
			// Go through each message
			for (size_t i = 0; i < buffer.m_numMessages; ++i)
			{
				// Generate an entry for the message
				int messageId = (buffer.m_startMessageId + i) % buffer.m_bufferLength;
				Debug::InMemoryLogEntry const& message = buffer.m_messages[messageId];
				if (filterEntry(scene, guiSettings, buffer, messageId, message))
					generateEntry(scene, guiSettings, message);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateMessages(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the corresponding buffer
			Debug::InMemoryLogBuffer const& buffer = getBuffer(scene, guiSettings);

			// Determine the number of columns nedded
			int numCols = 1;
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showRegion)
			{
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate ||
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity)
					++numCols;
			}
			else
			{
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate) ++numCols;
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity) ++numCols;
			}

			// Generate the log message table
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			if (ImGui::BeginTable("LogMessages", numCols, ImGuiTableFlags_SizingFixedFit))
			{
				// Configure the column sizes
				ImVec2 severityTextSize = ImGui::CalcTextSize("#WARNING#");
				ImVec2 dateTextSize = ImGui::CalcTextSize("2019. SEPTEMBER 12., 12:24:36");
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showRegion)
				{
					float colWidth = 0.0f;
					if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate)
						colWidth = glm::max(colWidth, dateTextSize.x);
					if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity)
						colWidth = glm::max(colWidth, dateTextSize.x);

					if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate ||
						guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity)
						ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, colWidth);
				}
				else
				{
					if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate)
						ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, dateTextSize.x);
					if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity)
						ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, severityTextSize.x);
				}
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, severityTextSize.x);

				// Generate the log messages
				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_longMessages)
					generateSlow(scene, guiSettings, buffer);
				else
					generateClipped(scene, guiSettings, buffer);

				// Close the table
				ImGui::EndTable();
			}
			ImGui::PopStyleVar(2);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateLogWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Log Window");

		// Window attributes
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = 0;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		// Create the window
		if (ImGui::Begin("Log", nullptr, flags))
		{
			// Generate the settings editor
			bool settingsShown = settingsShown = ImGui::TreeNode("Settings");
			if (settingsShown)
			{
				// Log Settings
				ImGui::Checkbox("Debug", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDebug);
				ImGui::SameLine();
				ImGui::PushID(glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_debugColor));
				ImGui::ColorEdit3("Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_debugColor));
				ImGui::PopID();

				ImGui::Checkbox("Trace", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showTrace);
				ImGui::SameLine();
				ImGui::PushID(glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_traceColor));
				ImGui::ColorEdit3("Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_traceColor));
				ImGui::PopID();

				ImGui::Checkbox("Info", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showInfo);
				ImGui::SameLine();
				ImGui::PushID(glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_infoColor));
				ImGui::ColorEdit3("Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_infoColor));
				ImGui::PopID();

				ImGui::Checkbox("Warning", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showWarning);
				ImGui::SameLine();
				ImGui::PushID(glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_warningColor));
				ImGui::ColorEdit3("Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_warningColor));
				ImGui::PopID();

				ImGui::Checkbox("Error", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showError);
				ImGui::SameLine();
				ImGui::PushID(glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_errorColor));
				ImGui::ColorEdit3("Color", glm::value_ptr(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_errorColor));
				ImGui::PopID();

				ImGui::SliderFloat("Message Spacing", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_messageSpacing, 0.0f, 2.0f);

				if (ImGui::RegexFilter("Include Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter))
				{
					Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_lastFilterUpdate = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
				}

				if (ImGui::RegexFilter("Exclude Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_discardFilter))
				{
					Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_lastFilterUpdate = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
				}
				
				ImGui::Checkbox("Show Date", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showDate);
				ImGui::SameLine();
				ImGui::Checkbox("Show Severity", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showSeverity);
				ImGui::SameLine();
				ImGui::Checkbox("Show Region", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showRegion);
				ImGui::SameLine();
				ImGui::Checkbox("Show Quick Filter", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showQuickFilter);
				ImGui::SameLine();
				ImGui::Checkbox("Long Messages", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_longMessages);
				ImGui::SameLine();
				ImGui::Checkbox("Auto Scroll", &guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_autoScroll);
				ImGui::SameLine();
				if (ImGui::Button("Clear"))
				{
					Debug::logger_impl::debug_only_inmemory_buffer.clear();
					Debug::logger_impl::debug_full_inmemory_buffer.clear();
					Debug::logger_impl::trace_only_inmemory_buffer.clear();
					Debug::logger_impl::trace_full_inmemory_buffer.clear();
					Debug::logger_impl::info_only_inmemory_buffer.clear();
					Debug::logger_impl::info_full_inmemory_buffer.clear();
					Debug::logger_impl::warning_only_inmemory_buffer.clear();
					Debug::logger_impl::warning_full_inmemory_buffer.clear();
					Debug::logger_impl::error_only_inmemory_buffer.clear();
					Debug::logger_impl::error_full_inmemory_buffer.clear();
				}

				ImGui::Separator();
				ImGui::TreePop();
			}

			// Leave some space for bottom filter and clear line, if needed
			const bool showQuickFilter = !settingsShown && guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_showQuickFilter;
			float bottomLineSpace = 0.0f;
			if (showQuickFilter)
			{
				// Height of the bar
				float barHeight = ImGui::CalcTextSize("XX").y + ImGui::GetStyle().FramePadding.y * 2.0f + ImGui::GetStyle().ItemSpacing.y * 2.0f;

				// This is the space necessary
				bottomLineSpace += barHeight;
			}

			// Begin the content area
			if (ImGui::BeginChild("Messages", ImVec2(0.0f, 0.0f - bottomLineSpace), true))
			{
				// Generate the log messages
				LogWindow::generateMessages(scene, guiSettings);

				// Implement auto scrolling
				const float prevMaxScroll = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_maxScrollY;
				const float currMaxScroll = ImGui::GetScrollMaxY();

				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_maxScrollY = currMaxScroll;

				if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_autoScroll && 
					prevMaxScroll + 8.0f < currMaxScroll && ImGui::GetScrollY() <= currMaxScroll)
					ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();

			// Show the quick action bar at the bottom
			if (showQuickFilter)
			{
				if (ImGui::RegexFilter("Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_includeFilter))
				{
					Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
					guiSettings->component<GuiSettings::GuiSettingsComponent>().m_logOutputSettings.m_lastFilterUpdate = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear"))
				{
					Debug::logger_impl::debug_only_inmemory_buffer.clear();
					Debug::logger_impl::debug_full_inmemory_buffer.clear();
					Debug::logger_impl::trace_only_inmemory_buffer.clear();
					Debug::logger_impl::trace_full_inmemory_buffer.clear();
					Debug::logger_impl::info_only_inmemory_buffer.clear();
					Debug::logger_impl::info_full_inmemory_buffer.clear();
					Debug::logger_impl::warning_only_inmemory_buffer.clear();
					Debug::logger_impl::warning_full_inmemory_buffer.clear();
					Debug::logger_impl::error_only_inmemory_buffer.clear();
					Debug::logger_impl::error_full_inmemory_buffer.clear();
				}
			}
		}

		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace ShaderInspectorWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		std::string getVariableName(Scene::Scene& scene, Scene::Object* guiSettings, std::string name)
		{
			switch (guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_variableNames)
			{
			case ShaderInspectorSettings::Full:
				return name;
			case ShaderInspectorSettings::NameOnly:
			{
				size_t start = name.find_last_of('.');
				start = start == std::string::npos ? 0 : start + 1;
				size_t end = name.back() == ']' ? name.find_last_of('[') : std::string::npos;
				size_t count = end == std::string::npos ? std::string::npos : end - start;
				return name.substr(start, count);
			}
			}

			return name;
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateUniformsTable(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the program id
			GLuint program = scene.m_shaders[guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram].m_program;

			// Get the list of uniforms
			auto uniforms = GPU::inspectProgramUniforms(program);

			// Table contents
			std::vector<std::vector<std::string>> contents;

			// Go through each row to and compute the text lengths
			for (auto const& uniform : uniforms)
			{
				// Stringify the values
				std::vector<std::string> values =
				{
					std::to_string(uniform.m_location),
					getVariableName(scene, guiSettings, uniform.m_name),
					GPU::dataTypeName(uniform.m_type, uniform.m_arraySize),
					uniform.m_isArray ? std::to_string(uniform.m_arrayStride) : ""
				};
				contents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig conf;
			conf.cols.count = 4;
			conf.cols.headers = { "Loc", "Name", "Type", "Stride" };
			conf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			conf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT };
			conf.rows.values = contents;

			ImGui::Table("Uniforms", conf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateUniformBlocksTable(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the program id
			GLuint program = scene.m_shaders[guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram].m_program;

			// Get the list of uniforms
			auto blocks = GPU::inspectProgramUniformBlocks(program);

			// Table contents
			std::vector<std::vector<std::string>> blockListContents;

			// Go through each row to and compute the text lengths
			for (auto const& block : blocks)
			{
				// Stringify the values
				std::vector<std::string> values =
				{
					std::to_string(block.m_binding),
					block.m_name,
					std::to_string(block.m_variables.size()),
					std::to_string(block.m_dataSize)
				};
				blockListContents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig blockConf;
			blockConf.cols.count = 4;
			blockConf.cols.headers = { "Bind", "Name", "Variables", "Size" };
			blockConf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			blockConf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::INT };
			blockConf.rows.values = blockListContents;
			blockConf.rows.display_min = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows.x;
			blockConf.rows.display_max = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows.y;
			blockConf.selection.enabled = true;
			blockConf.selection.row_id = &EditorSettings::editorProperty<int>(scene, guiSettings, "ShaderInspector_UniformBlockId");

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0.0f));
			ImGui::Table("Blocks", blockConf);
			ImGui::PopStyleVar();

			// Add some spece between the two tables
			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			// Which block to inspect
			std::vector<std::string> blockNames;
			for (auto const& block : blocks)
			{
				blockNames.push_back(block.m_name);
			}

			// Table contents
			std::vector<std::vector<std::string>> variableContents;

			// Go through each row to and compute the text lengths
			if (!blocks.empty())
			{
				for (auto const& variable : blocks[EditorSettings::editorProperty<int>(scene, guiSettings, "ShaderInspector_UniformBlockId")].m_variables)
				{
					// Stringify the values
					std::vector<std::string> values =
					{
						std::to_string(variable.m_byteOffset),
						getVariableName(scene, guiSettings, variable.m_name),
						GPU::dataTypeName(variable.m_type, variable.m_arraySize),
						std::to_string(variable.m_padding),
						variable.m_isArray ? std::to_string(variable.m_arrayStride) : ""
					};
					variableContents.emplace_back(values);
				}
			}

			// Generate the actual table
			ImGui::TableConfig variableConf;
			variableConf.cols.count = 5;
			variableConf.cols.headers = { "Offset", "Name", "Type", "Padding", "Stride" };
			variableConf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			variableConf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::INT };
			variableConf.rows.values = variableContents;
			variableConf.rows.display_min = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows.x;
			variableConf.rows.display_max = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows.y;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0.0f));
			ImGui::Table("Variables", variableConf);
			ImGui::PopStyleVar();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateShaderStorageBlocksTable(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the program id
			GLuint program = scene.m_shaders[guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram].m_program;

			// Get the list of uniforms
			auto blocks = GPU::inspectProgramShaderStorageBlocks(program);

			// Table contents
			std::vector<std::vector<std::string>> blockListContents;

			// Go through each row to and compute the text lengths
			for (auto const& block : blocks)
			{
				// Stringify the values
				std::vector<std::string> values =
				{
					std::to_string(block.m_binding),
					block.m_name,
					std::to_string(block.m_variables.size()),
					std::to_string(block.m_dataSize)
				};
				blockListContents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig blockConf;
			blockConf.cols.count = 4;
			blockConf.cols.headers = { "Bind", "Name", "Variables", "Size" };
			blockConf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			blockConf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::INT };
			blockConf.rows.values = blockListContents;
			blockConf.rows.display_min = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows.x;
			blockConf.rows.display_max = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows.y;
			blockConf.selection.enabled = true;
			blockConf.selection.row_id = &EditorSettings::editorProperty<int>(scene, guiSettings, "ShaderInspector_ShaderStorageBlockId");

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0.0f));
			ImGui::Table("Blocks", blockConf);
			ImGui::PopStyleVar();

			// Add some spece between the two tables
			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			// Which block to inspect
			std::vector<std::string> blockNames;
			for (auto const& block : blocks)
			{
				blockNames.push_back(block.m_name);
			}

			// Table contents
			std::vector<std::vector<std::string>> variableContents;

			// Go through each row to and compute the text lengths
			if (!blocks.empty())
			{
				for (auto const& variable : blocks[EditorSettings::editorProperty<int>(scene, guiSettings, "ShaderInspector_ShaderStorageBlockId")].m_variables)
				{
					// Stringify the values
					std::vector<std::string> values =
					{
						std::to_string(variable.m_byteOffset),
						getVariableName(scene, guiSettings, variable.m_name),
						GPU::dataTypeName(variable.m_type, variable.m_arraySize),
						std::to_string(variable.m_padding),
						variable.m_isArray ? std::to_string(variable.m_arrayStride) : "",
						std::to_string(variable.m_topLevelArrayStride)
					};
					variableContents.emplace_back(values);
				}
			}

			// Generate the actual table
			ImGui::TableConfig variableConf;
			variableConf.cols.count = 6;
			variableConf.cols.headers = { "Offset", "Name", "Type", "Padding", "Stride", "Owner Stride" };
			variableConf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			variableConf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::INT };
			variableConf.rows.values = variableContents;
			variableConf.rows.display_min = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows.x;
			variableConf.rows.display_max = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows.y;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().WindowPadding.x, 0.0f));
			ImGui::Table("Variables", variableConf);
			ImGui::PopStyleVar();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateInputsTable(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the program id
			GLuint program = scene.m_shaders[guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram].m_program;

			// Get the list of inputs
			auto inputs = GPU::inspectProgramInputs(program);

			// Table contents
			std::vector<std::vector<std::string>> contents;

			// Go through each row to and compute the text lengths
			for (auto const& input : inputs)
			{
				// Stringify the values
				std::vector<std::string> values =
				{
					input.m_location < 0 ? "" : std::to_string(input.m_location),
					getVariableName(scene, guiSettings, input.m_name),
					GPU::dataTypeName(input.m_type, input.m_arraySize),
				};
				contents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig conf;
			conf.cols.headers = { "Loc", "Name", "Type" };
			conf.cols.count = 3;
			conf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			conf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT };
			conf.rows.values = contents;

			ImGui::Table("Inputs", conf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateOutputsTable(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the program id
			GLuint program = scene.m_shaders[guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram].m_program;

			// Get the list of outputs
			auto outputs = GPU::inspectProgramOutputs(program);

			// Table contents
			std::vector<std::vector<std::string>> contents;

			// Go through each row to and compute the text lengths
			for (auto const& output : outputs)
			{
				// Stringify the values
				std::vector<std::string> values =
				{
					output.m_location < 0 ? "" : std::to_string(output.m_location),
					getVariableName(scene, guiSettings, output.m_name),
					GPU::dataTypeName(output.m_type, output.m_arraySize),
				};
				contents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig conf;
			conf.cols.headers = { "Loc", "Name", "Type" };
			conf.cols.count = 3;
			conf.cols.alignments = { ImGui::TableConfig::Column::Right, ImGui::TableConfig::Column::Left, ImGui::TableConfig::Column::Left };
			conf.cols.data_types = { ImGui::TableConfig::Column::INT, ImGui::TableConfig::Column::STRING, ImGui::TableConfig::Column::INT };
			conf.rows.values = contents;

			ImGui::Table("Outputs", conf);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool generateProgramSelector(Scene::Scene& scene, Scene::Object* guiSettings, std::string const& label, std::string& currentProgramId, float selectorHeight, ImGui::FilterRegex const& regex)
		{
			bool valueChanged = false;

			ImGui::TextDisabled(label.c_str());

			ImGuiWindowFlags selectorChildFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
			if (ImGui::BeginChild("ProgramSelector", ImVec2(0.0f, selectorHeight), true, selectorChildFlags))
			{
				for (auto const& program : scene.m_shaders)
				{
					const bool item_selected = (program.first == currentProgramId);

					if (regex.m_empty == false && !regex.test(program.first))
						continue;

					ImGui::PushID(program.first.c_str());

					if (ImGui::Selectable(program.first.c_str(), item_selected))
					{
						valueChanged |= true;
						currentProgramId = program.first;
					}

					if (item_selected)
						ImGui::SetItemDefaultFocus();

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text(program.first.c_str());
						ImGui::EndTooltip();
					}

					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			return valueChanged;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateShaderInspectorWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Shader Inspector Window");

		// Window attributes
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = 0;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		// Create the window
		if (ImGui::Begin("Shader Inspector", nullptr, flags))
		{
			// Generate the settings editor
			if (ImGui::TreeNode("Settings"))
			{
				ImGui::SliderInt("Shader Selector Height", &guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_shaderSelectorHeight, 1, 600);
				ImGui::Combo("Variable Names", &guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_variableNames, ShaderInspectorSettings::VariableNameMethod_meta);
				ImGui::SliderInt2("Num Block Rows", glm::value_ptr(guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numBlockRows), 0, 16);
				ImGui::SliderInt2("Num Variable Rows", glm::value_ptr(guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_numVariablesRows), 0, 16);
				ImGui::TreePop();
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Current program id
			std::string& currentProgramId = guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_currentProgram;

			// Fall back to the GUI program if the old program no longer exists
			bool resetBlockIds = false;
			if (auto programIt = scene.m_shaders.find(currentProgramId); programIt == scene.m_shaders.end() || programIt->second.m_program == 0)
			{
				currentProgramId = "Imgui/imgui";
				resetBlockIds = true;
			}

			// Which program to debug
			resetBlockIds |= ShaderInspectorWindow::generateProgramSelector(scene, guiSettings, "Program", currentProgramId, 
				guiSettings->component<GuiSettingsComponent>().m_shaderInspectorSettings.m_shaderSelectorHeight, 
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_selectorFilter);
			ImGui::RegexFilter("Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_shaderInspectorSettings.m_selectorFilter);

			// Reset the selected block ids when a significat change occurs
			if (resetBlockIds)
			{
				EditorSettings::editorProperty<int>(scene, guiSettings, "ShaderInspector_UniformBlockId") = 0;
				EditorSettings::editorProperty<int>(scene, guiSettings, "ShaderInspector_ShaderStorageBlockId") = 0;
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Shader name label
			ImGui::TextDisabled(currentProgramId.c_str());

			// Begin the content area
			if (ImGui::BeginChild("Properties", ImVec2(0.0f, 0.0f), true))
			{
				if (ImGui::BeginTabBar("Shader Properties Tab"))
				{
					// Restore the selected tab id
					std::string activeTab;
					if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, guiSettings, "ShaderInspectorPropertiesBar_SelectedTab#Synced"); activeTabSynced.has_value())
						activeTab = activeTabSynced.value();

					// Uniforms
					if (ImGui::BeginTabItem("Uniforms", activeTab.c_str()))
					{
						ShaderInspectorWindow::generateUniformsTable(scene, guiSettings);

						EditorSettings::editorProperty<std::string>(scene, guiSettings, "ShaderInspectorPropertiesBar_SelectedTab") = ImGui::CurrentTabItemName();
						ImGui::EndTabItem();
					}

					// Uniform blocks
					if (ImGui::BeginTabItem("Uniform Blocks", activeTab.c_str()))
					{
						ShaderInspectorWindow::generateUniformBlocksTable(scene, guiSettings);

						EditorSettings::editorProperty<std::string>(scene, guiSettings, "ShaderInspectorPropertiesBar_SelectedTab") = ImGui::CurrentTabItemName();
						ImGui::EndTabItem();
					}

					// Shader storage blocks
					if (ImGui::BeginTabItem("Shader Storage Blocks", activeTab.c_str()))
					{
						ShaderInspectorWindow::generateShaderStorageBlocksTable(scene, guiSettings);

						EditorSettings::editorProperty<std::string>(scene, guiSettings, "ShaderInspectorPropertiesBar_SelectedTab") = ImGui::CurrentTabItemName();
						ImGui::EndTabItem();
					}

					// Program inputs
					if (ImGui::BeginTabItem("Inputs", activeTab.c_str()))
					{
						ShaderInspectorWindow::generateInputsTable(scene, guiSettings);

						EditorSettings::editorProperty<std::string>(scene, guiSettings, "ShaderInspectorPropertiesBar_SelectedTab") = ImGui::CurrentTabItemName();
						ImGui::EndTabItem();
					}

					// Program outputs
					if (ImGui::BeginTabItem("Outputs", activeTab.c_str()))
					{
						ShaderInspectorWindow::generateOutputsTable(scene, guiSettings);

						EditorSettings::editorProperty<std::string>(scene, guiSettings, "ShaderInspectorPropertiesBar_SelectedTab") = ImGui::CurrentTabItemName();
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
			}
			ImGui::EndChild();
		}

		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace TextureInspectorWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		bool generateTextureCombo(Scene::Scene& scene, Scene::Object* guiSettings, std::string const& label, std::string& currentTextureId)
		{
			bool valueChanged = false;
			if (ImGui::BeginCombo(label.c_str(), currentTextureId.c_str()))
			{
				for (auto const& texture : scene.m_textures)
				{
					ImGui::PushID(texture.first.c_str());

					const bool item_selected = (texture.first == currentTextureId);

					unsigned char alpha = texture.second.m_type == GL_TEXTURE_2D ? 255 : 0;
					if (ImGui::SelectableWithIcon(texture.first.c_str(), item_selected, &scene.m_textures[texture.first].m_texture, ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, alpha), ImColor(0, 0, 0, 0)))
					{
						valueChanged |= true;
						currentTextureId = texture.first;
					}

					if (item_selected)
						ImGui::SetItemDefaultFocus();

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text(texture.first.c_str());
						ImGui::EndTooltip();
					}

					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			return valueChanged;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool generateTextureSelector(Scene::Scene& scene, Scene::Object* guiSettings, std::string const& label, 
			std::string& currentTextureId, float selectorHeight, ImGui::FilterRegex const& regex)
		{
			bool valueChanged = false;

			// Selector label
			ImGui::Text(label.c_str());

			// Texture selector
			ImGuiWindowFlags selectorChildFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
			if (ImGui::BeginChild("TextureSelector", ImVec2(0.0f, selectorHeight), true, selectorChildFlags))
			{
				for (auto const& texture : scene.m_textures)
				{
					const bool item_selected = (texture.first == currentTextureId);

					if (regex.m_empty == false && !regex.test(texture.first))
						continue;

					ImGui::PushID(texture.first.c_str());

					unsigned char alpha = texture.second.m_type == GL_TEXTURE_2D ? 255 : 0;
					if (ImGui::SelectableWithIcon(texture.first.c_str(), item_selected, &scene.m_textures[texture.first].m_texture, ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, alpha), ImColor(0, 0, 0, 0)))
					{
						valueChanged |= true;
						currentTextureId = texture.first;
					}

					if (item_selected)
						ImGui::SetItemDefaultFocus();

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text(texture.first.c_str());
						ImGui::EndTooltip();
					}

					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			return valueChanged;
		}

		////////////////////////////////////////////////////////////////////////////////
		void updateTextures(Scene::Scene& scene, Scene::Object* guiSettings, std::string const& currentTextureName, std::string const& previewTextureName)
		{
			// Extract the texture objects
			auto& previewTexture = scene.m_textures[previewTextureName];
			auto& currentTexture = scene.m_textures[currentTextureName];

			// Visualize it
			const size_t arraySlice = guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_arraySliceId;
			const size_t lodLevel = guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_lodLevelId;
			const glm::ivec3 textureSize = GPU::mipDimensions(currentTexture, lodLevel);

			// Generate a properly sized texture, if needed
			if (previewTexture.m_width != textureSize[0] || previewTexture.m_height != textureSize[1] ||
				previewTexture.m_layout != currentTexture.m_layout || previewTexture.m_format != currentTexture.m_format)
			{
				Scene::createTexture(scene, previewTextureName, GL_TEXTURE_2D,
					textureSize[0], textureSize[1], 1,
					currentTexture.m_format, currentTexture.m_layout,
					GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
			}

			// Upload the preview texture if the display changed
			if (currentTexture.m_type == GL_TEXTURE_1D)
			{
				// TODO: implement
			}
			else if (currentTexture.m_type == GL_TEXTURE_1D_ARRAY)
			{
				// TODO: implement
			}
			else if (currentTexture.m_type == GL_TEXTURE_2D)
			{
				glCopyImageSubData(
					currentTexture.m_texture, currentTexture.m_type, lodLevel, 0, 0, 0,
					previewTexture.m_texture, previewTexture.m_type, 0, 0, 0, 0, previewTexture.m_width, previewTexture.m_height, 1);
			}
			else if (currentTexture.m_type == GL_TEXTURE_2D_ARRAY || currentTexture.m_type == GL_TEXTURE_3D)
			{
				glCopyImageSubData(
					currentTexture.m_texture, currentTexture.m_type, lodLevel, 0, 0, arraySlice,
					previewTexture.m_texture, previewTexture.m_type, 0, 0, 0, 0, previewTexture.m_width, previewTexture.m_height, 1);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getFileDialogFilters(Scene::Scene& scene)
		{
			// List of formats and the corresponding file filters
			using FormatDescription = std::string;
			using FormatFilter = std::vector<std::string>;
			using Format = std::pair<FormatDescription, FormatFilter>;
			static std::vector<Format> s_imageFormats =
			{
				{ "BMP - Windows Bitmap", { ".bmp" } },
				{ "JPG - JPG/JPEG Format", { ".jpg", ".jpeg" } },
				{ "PCX - Zsoft Painbrush", { ".pcx" } },
				{ "PNG - Portable Network Graphics", { ".png" } },
				{ "RAW - Raw Image Data", { ".raw" } },
				{ "TGA - Truevision Target", { ".tga" } },
				{ "TIF - Tagged Image File Format", { ".tif" } },
			};

			// Resulting str
			static std::string s_result;

			// Construct the result
			if (s_result.empty())
			{
				// Construct the "all image format" filter
				FormatFilter filter;
				for (auto const& format : s_imageFormats)
					filter.insert(filter.end(), format.second.begin(), format.second.end());
				s_imageFormats.insert(s_imageFormats.begin(), { "Any Image Format", filter });

				// Join the filters together for the result
				s_result = std::string_join(",", s_imageFormats.begin(), s_imageFormats.end(), [](Format const& format)
					{
						return format.first + " (" + std::string_join(" ", format.second.begin(), format.second.end()) + "){" + std::string_join(",", format.second.begin(), format.second.end()) + "}";
					});
			}

			return s_result;
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateOpenFileDialog(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Extract the file dialog object
			auto fileDialog = ImGuiFileDialog::Instance();

			// Texture name and path
			std::string& textureName = EditorSettings::editorProperty<std::string>(scene, guiSettings, "LoadTexture_TextureName", false);
			std::string& texturePath = EditorSettings::editorProperty<std::string>(scene, guiSettings, "LoadTexture_TexturePath", false);

			// Open the file dialog
			if (ImGui::Button("Load Texture"))
			{
				ImGui::OpenPopup("LoadTextureDialog");
				textureName = texturePath = "";
			}

			if (ImGui::BeginPopup("LoadTextureDialog"))
			{
				//ImGui::Combo("Texture Type", );
				ImGui::InputText("Texture Name", textureName);
				ImGui::InputText("Texture Path", texturePath);
				ImGui::SameLine();
				if (ImGui::Button("Browse"))
				{
					std::string const& filters = getFileDialogFilters(scene);
					std::string const& texturesFolder = (EnginePaths::assetsFolder() / "Textures").string();

					Debug::log_debug() << "Texture type filters: " << filters << Debug::end;
					Debug::log_debug() << "Textures folder: " << texturesFolder << Debug::end;

					fileDialog->OpenModal("LoadTextureFileSelectDialog", "Select texture...", filters.c_str(), texturesFolder, "");
				}

				// Generate the dialog itself
				if (fileDialog->Display("LoadTextureFileSelectDialog"))
				{
					if (fileDialog->IsOk())
					{
						texturePath = fileDialog->GetFilePathName();
						textureName = std::filesystem::path(texturePath).filename().string();
					}
					fileDialog->Close();
				}

				// Try to load the texture
				if (ImGui::ButtonEx("Ok", "|########|"))
				{
					Asset::loadTexture(scene, textureName, texturePath);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::ButtonEx("Cancel", "|########|"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateContentArea(Scene::Scene& scene, Scene::Object* guiSettings, bool displayChanged)
		{
			// Name of the current texture
			std::string& currentTextureId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "TextureInspector_CurrentTexture");

			// Texture name label
			ImGui::TextDisabled(currentTextureId.c_str());

			if (!ImGui::BeginChild("Properties", ImVec2(0.0f, 0.0f), true))
			{
				ImGui::EndChild();
				return;
			}

			// Visualize the texture
			std::string const& previewTextureName = guiSettings->m_name + "_texture_inspector_preview";
			auto& previewTexture = scene.m_textures[previewTextureName];
			auto& currentTexture = scene.m_textures[currentTextureId];

			GLint numMipLevels = currentTexture.m_mipmapped ? GPU::numMipLevels(currentTexture) : 1;
			glBindTexture(currentTexture.m_type, 0);
			if (currentTexture.m_type == GL_TEXTURE_3D || currentTexture.m_type == GL_TEXTURE_2D_ARRAY)
			{
				displayChanged |= ImGui::SliderInt("Array Slice", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_arraySliceId, 0, currentTexture.m_depth - 1);
			}
			if (currentTexture.m_mipmapped)
			{
				displayChanged |= ImGui::SliderInt("LOD", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_lodLevelId, 0, numMipLevels - 1);
			}
			displayChanged |= ImGui::SliderFloat("Zoom Factor", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_zoomFactor, 1.0f, 32.0f);
			if (ImGui::Button("Export"))
			{
				Asset::saveTexture(scene, previewTextureName, EnginePaths::generateUniqueFilepath("TextureInspector/" + currentTextureId + "_", ".png"));
			}

			// Update the texture, if needed
			if (displayChanged) updateTextures(scene, guiSettings, currentTextureId, previewTextureName);

			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			if (ImGui::BeginTable("TexturePreviewSplit", 2))
			{
				// Texture parameters
				ImGui::TableNextColumn();
				if (ImGui::BeginChild("Texture Parameters", ImVec2(0, -1), false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
				{
					const int lodLevel = guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_lodLevelId;
					const glm::ivec3 dimensions = GPU::mipDimensions(currentTexture, lodLevel);
					const size_t channels = GPU::textureFormatChannels(currentTexture.m_format);
					const size_t texelSize = (GPU::textureFormatTexelSize(currentTexture.m_format) + 7) / 8;
					const size_t numTexelsLevel = GPU::numTexels(currentTexture, lodLevel, 1);
					const size_t numTexelsTotal = GPU::numTexels(currentTexture, 0, numMipLevels);
					const size_t levelSize = GPU::textureSizeBytes(currentTexture, 0, -1); numTexelsLevel* texelSize;
					const size_t textureSize = GPU::textureSizeBytes(currentTexture, lodLevel, 1);

					#define ADD_ROW(NAME, FORMAT, ...) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, __VA_ARGS__); }
					#define ADD_ROW_STR(NAME, FORMAT, STR) { std::string const& tmp = STR; ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, tmp.c_str()); }

					// Common
					ImGui::TextDisabled("Common");
					if (ImGui::BeginTable("TexturePropertiesCommon", 2))
					{
						ADD_ROW("Binding ID", "%d", (currentTexture.m_bindingId > 0 ? currentTexture.m_bindingId - GL_TEXTURE0 : 0));
						ADD_ROW_STR("Type", "%s", GPU::enumToString(currentTexture.m_type));
						ImGui::EndTable();
					}

					// Dimensions (Base)
					ImGui::Dummy(ImVec2(0.0f, 15.0f));
					ImGui::TextDisabled("Dimensions (Base)");
					if (ImGui::BeginTable("TexturePropertiesDimensionsBase", 2))
					{
						ADD_ROW("Width", "%d", currentTexture.m_dimensions.x);
						ADD_ROW("Height", "%d", currentTexture.m_dimensions.y);
						ADD_ROW("Depth", "%d", currentTexture.m_dimensions.z);
						ADD_ROW("Texels", "%llu", numTexelsTotal);
						ADD_ROW_STR("Texture Size", "%s", Units::bytesToString(textureSize));
						ImGui::EndTable();
					}

					// Dimensions (Level)
					if (currentTexture.m_mipmapped)
					{
						ImGui::Dummy(ImVec2(0.0f, 15.0f));
						ImGui::TextDisabled("Dimensions (Level)");
						if (ImGui::BeginTable("TexturePropertiesDimensionsLevel", 2))
						{
							ADD_ROW("Width", "%d", dimensions.x);
							ADD_ROW("Height", "%d", dimensions.y);
							ADD_ROW("Depth", "%d", dimensions.z);
							ADD_ROW("Texels", "%llu", numTexelsLevel);
							ADD_ROW_STR("Texture Size", "%s", Units::bytesToString(levelSize));
							ImGui::EndTable();
						}
					}

					// Format
					ImGui::Dummy(ImVec2(0.0f, 15.0f));
					ImGui::TextDisabled("Format");
					if (ImGui::BeginTable("TexturePropertiesFormat", 2))
					{
						ADD_ROW("Channels", "%d", channels);
						ADD_ROW_STR("Format", "%s", GPU::enumToString(currentTexture.m_format));
						ADD_ROW_STR("Layout", "%s", GPU::enumToString(currentTexture.m_layout));
						ImGui::EndTable();
					}

					// Filtering
					ImGui::Dummy(ImVec2(0.0f, 15.0f));
					ImGui::TextDisabled("Filtering");
					if (ImGui::BeginTable("TexturePropertiesFiltering", 2))
					{
						ADD_ROW_STR("Min Filter", "%s", GPU::enumToString(currentTexture.m_minFilter));
						ADD_ROW_STR("Mag Filter", "%s", GPU::enumToString(currentTexture.m_magFilter));
						ADD_ROW_STR("Wrap Mode", "%s", GPU::enumToString(currentTexture.m_wrapMode));
						if (currentTexture.m_mipmapped)
							ADD_ROW("Mip Levels", "%d", numMipLevels);
						ImGui::EndTable();
					}
				}
				ImGui::EndChild();

				// Texture preview
				ImGui::TableNextColumn();
				if (ImGui::BeginChild("Texture Preview", ImVec2(0, ImGui::GetContentRegionMax().y - ImGui::GetCursorPos().y), false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar))
				{
					// Display the actual image
					const float aspect = float(previewTexture.m_width) / float(previewTexture.m_height);
					const int imageSize = guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_previewHeight;
					const float zoomUvScale = 1.0f / guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_zoomFactor;

					ImGui::TextDisabled("Preview");
					ImGui::Image(&previewTexture.m_texture, ImVec2(imageSize * aspect, imageSize), 
						ImVec2(0, 1) * zoomUvScale, ImVec2(1, 0) * zoomUvScale, 
						ImColor(255, 255, 255, 255), ImColor(0, 0, 0, 0));

					// Large size tooltip
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						int imageSize = guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_tooltipHeight;
						ImGui::Image(&previewTexture.m_texture, ImVec2(imageSize * aspect, imageSize), 
							ImVec2(0, 1) * zoomUvScale, ImVec2(1, 0) * zoomUvScale, 
							ImColor(255, 255, 255, 255), ImColor(0, 0, 0, 0));
						ImGui::EndTooltip();
					}
				}
				ImGui::EndChild();

				ImGui::EndTable();
			}

			ImGui::EndChild();
		}

	}

	////////////////////////////////////////////////////////////////////////////////
	void generateTextureInspectorWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Texture Inspector Window");

		// Window attributes
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = 0;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		// Name of the current texture
		std::string& currentTextureId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "TextureInspector_CurrentTexture");

		// Whether the display changed or not
		bool displayChanged = guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_liveUpdate;

		// Create the window
		if (ImGui::Begin("Texture Inspector", nullptr, flags))
		{
			// Generate the settings editor
			if (ImGui::TreeNode("Settings"))
			{
				ImGui::SliderInt("Texture Selector Height", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_textureSelectorHeight, 1, 600);
				ImGui::SliderInt("Preview Image Height", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_previewHeight, 1, 2048);
				ImGui::SliderInt("Tooltip Image Height", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_tooltipHeight, 1, 2048);
				displayChanged |= ImGui::SliderInt("1D Block Height", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_1dTextureBlockSize, 1, 1024);
				displayChanged |= ImGui::SliderInt("1D Array Block Height", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_1dArrayTextureBlockSize, 1, 256);
				ImGui::Checkbox("Live Update", &guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_liveUpdate);

				ImGui::TreePop();
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Fall back to the font atlas if the old texture no longer exists
			if (auto textureIt = scene.m_textures.find(currentTextureId); textureIt == scene.m_textures.end() || textureIt->second.m_texture == 0)
			{
				currentTextureId = "ImguiFont";
			}

			// Generate the texture selector
			displayChanged |= TextureInspectorWindow::generateTextureSelector(scene, guiSettings, "Texture", currentTextureId,
				guiSettings->component<GuiSettingsComponent>().m_textureInspectorSettings.m_textureSelectorHeight,
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_selectorFilter);
			ImGui::RegexFilter("Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_textureInspectorSettings.m_selectorFilter);

			// Generate the open file dialog
			TextureInspectorWindow::generateOpenFileDialog(scene, guiSettings);

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Generate the content area
			TextureInspectorWindow::generateContentArea(scene, guiSettings, displayChanged);
		}
		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace MaterialEditorWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		void generateMaterialSelector(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			std::string& currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName");

			// Material selector label
			ImGui::TextDisabled("Material");

			ImGui::FilterRegex const& filter = guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_selectorFilter;

			ImGuiWindowFlags selectorChildFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
			if (ImGui::BeginChild("TableContents", ImVec2(0.0f, guiSettings->component<GuiSettingsComponent>().m_materialEditorSettings.m_materialSelectorHeight), true, selectorChildFlags))
			{
				for (auto const& material : scene.m_materials)
				{
					const bool item_selected = (material.first == currentMaterialId);

					if (filter.m_empty == false && !filter.test(material.first))
						continue;

					ImGui::PushID(material.first.c_str());

					if (ImGui::Selectable(material.first.c_str(), item_selected))
					{
						currentMaterialId = material.first;
					}

					if (item_selected)
						ImGui::SetItemDefaultFocus();

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text(material.first.c_str());
						ImGui::EndTooltip();
					}

					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			ImGui::RegexFilter("Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_materialEditorSettings.m_selectorFilter);
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateTextureEditor(Scene::Scene& scene, Scene::Object* guiSettings, std::string const& name, std::string& texture, std::string const& defaultTexture)
		{
			ImGui::PushID(name.c_str());

			int previewHeight = guiSettings->component<GuiSettingsComponent>().m_materialEditorSettings.m_previewHeight;
			int tooltipHeight = guiSettings->component<GuiSettingsComponent>().m_materialEditorSettings.m_tooltipHeight;

			TextureInspectorWindow::generateTextureCombo(scene, guiSettings, name.c_str(), texture);
			ImGui::SameLine();
			ImGui::ImageSquare(&scene.m_textures[texture].m_texture, ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(0, 0, 0, 0));
			bool showTooltip = ImGui::IsItemHovered();
			ImGui::SameLine();
			float cursorPosY = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(cursorPosY + ImGui::GetStyle().FramePadding.y);
			ImGui::SameLine();
			ImGui::SetCursorPosY(cursorPosY);
			if (ImGui::Button("Clear"))
				texture = defaultTexture;

			if (showTooltip)
			{
				ImGui::BeginTooltip();
				ImGui::Text(texture.c_str());
				ImGui::Image(&scene.m_textures[texture].m_texture, ImVec2(tooltipHeight, tooltipHeight), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(0, 0, 0, 0));
				ImGui::EndTooltip();
			}

			ImGui::PopID();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateMaterialEditor(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			std::string& currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName");

			ImGui::TextDisabled(currentMaterialId.c_str());
			if (ImGui::BeginChild("Properties", ImVec2(0.0f, 0.0f), true))
			{
				GPU::Material& material = scene.m_materials[currentMaterialId];

				ImGui::Combo("Blend Mode", &material.m_blendMode, GPU::Material::BlendMode_meta);

				ImGui::ColorEdit3("Diffuse", glm::value_ptr(material.m_diffuse));
				ImGui::ColorEdit3("Emissive", glm::value_ptr(material.m_emissive));

				ImGui::SliderFloat("Opacity", &material.m_opacity, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &material.m_metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("Roughness", &material.m_roughness, 0.015f, 1.0f);
				ImGui::SliderFloat("Specular", &material.m_specular, 0.0f, 1.0f);
				ImGui::SliderFloat("Normal Map Strength", &material.m_normalMapStrength, 0.0f, 1.0f);
				ImGui::SliderFloat("Displacement Map Strength", &material.m_displacementScale, 0.0f, 1.0f);
				ImGui::Checkbox("Two-sided", &material.m_twoSided);

				generateTextureEditor(scene, guiSettings, "Diffuse Map", material.m_diffuseMap, "default_diffuse_map");
				generateTextureEditor(scene, guiSettings, "Normal Map", material.m_normalMap, "default_normal_map");
				generateTextureEditor(scene, guiSettings, "Specular Map", material.m_specularMap, "default_specular_map");
				generateTextureEditor(scene, guiSettings, "Alpha Map", material.m_alphaMap, "default_alpha_map");
				generateTextureEditor(scene, guiSettings, "Displacement Map", material.m_displacementMap, "default_displacement_map");
			}
			ImGui::EndChild();
		}

		////////////////////////////////////////////////////////////////////////////////
		void openNewMaterialPopup(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			std::string& currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName");

			for (int i = 1;; ++i)
			{
				std::string matName = "Material " + std::to_string(i);
				if (scene.m_materials.find(matName) == scene.m_materials.end())
				{
					EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_NewMaterialName", false) = matName;
					EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_ReferenceMaterialName", false) = "";
					break;
				}
			}
			ImGui::OpenPopup("MaterialEditor_NewMaterial");
		}

		////////////////////////////////////////////////////////////////////////////////
		void openDuplicateMaterialPopup(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			std::string& currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName");

			for (int i = 1;; ++i)
			{
				std::string matName = currentMaterialId + " " + std::to_string(i);
				if (scene.m_materials.find(matName) == scene.m_materials.end())
				{
					EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_NewMaterialName", false) = matName;
					EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_ReferenceMaterialName", false) = currentMaterialId;
					break;
				}
			}
			ImGui::OpenPopup("MaterialEditor_NewMaterial");
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateNewMaterialPopup(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			std::string& currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName");

			ImGui::InputText("Material Name", EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_NewMaterialName"));

			if (ImGui::ButtonEx("Ok", "|########|"))
			{
				std::string refMat = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_ReferenceMaterialName");
				scene.m_materials[EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_NewMaterialName")] = refMat.empty() ? GPU::Material() : scene.m_materials[refMat];
				currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_NewMaterialName");

				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::ButtonEx("Cancel", "|########|"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateMaterialEditorWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Material Editor Window");

		// Window attributes
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = 0;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		// Forced editor focus
		if (EditorSettings::consumeEditorProperty<bool>(scene, guiSettings, "MaterialEditor_ForceFocus").has_value())
		{
			ImGui::SetNextWindowFocus();
		}

		// Create the window
		if (ImGui::Begin("Material Editor", nullptr, flags))
		{
			// Generate the settings editor
			if (ImGui::TreeNode("Settings"))
			{
				ImGui::SliderInt("Material Selector Height", &guiSettings->component<GuiSettingsComponent>().m_materialEditorSettings.m_materialSelectorHeight, 1, 600);
				ImGui::SliderInt("Preview Image Height", &guiSettings->component<GuiSettingsComponent>().m_materialEditorSettings.m_previewHeight, 1, 2048);
				ImGui::SliderInt("Tooltip Image Height", &guiSettings->component<GuiSettingsComponent>().m_materialEditorSettings.m_tooltipHeight, 1, 2048);
				ImGui::TreePop();
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Fall back to the first shader
			std::string& currentMaterialId = EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName");
			auto materialIt = scene.m_materials.find(currentMaterialId);
			if (materialIt == scene.m_materials.end())
			{
				currentMaterialId = scene.m_materials.begin()->first;
			}

			// Which material to edit
			MaterialEditorWindow::generateMaterialSelector(scene, guiSettings);

			// New material button
			if (ImGui::ButtonEx("New", "|#########|"))
			{
				MaterialEditorWindow::openNewMaterialPopup(scene, guiSettings);
			}

			// Duplicate material
			ImGui::SameLine();
			if (ImGui::ButtonEx("Duplicate", "|#########|"))
			{
				MaterialEditorWindow::openDuplicateMaterialPopup(scene, guiSettings);
			}

			// Make a new or duplicate material
			if (ImGui::BeginPopup("MaterialEditor_NewMaterial"))
			{
				MaterialEditorWindow::generateNewMaterialPopup(scene, guiSettings);
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Begin the content area
			MaterialEditorWindow::generateMaterialEditor(scene, guiSettings);
		}

		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	void startEditingMaterial(Scene::Scene& scene, std::string const& materialName)
	{
		Scene::Object* guiSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_GUI_SETTINGS);

		EditorSettings::editorProperty<std::string>(scene, guiSettings, "MaterialEditor_MaterialName") = materialName;
		EditorSettings::editorProperty<bool>(scene, guiSettings, "MaterialEditor_ForceFocus") = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace BufferInspectorWindow
	{
		////////////////////////////////////////////////////////////////////////////////
		bool generateBufferSelector(Scene::Scene& scene, Scene::Object* guiSettings, std::string const& label, 
			std::string& currentBufferId, float selectorHeight, ImGui::FilterRegex const& regex)
		{
			bool valueChanged = false;

			// Buffer selector label
			ImGui::Text(label.c_str());

			// Buffer selector
			ImGuiWindowFlags selectorChildFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
			if (ImGui::BeginChild("BufferSelector", ImVec2(0.0f, selectorHeight), true, selectorChildFlags))
			{
				for (auto const& buffer : scene.m_genericBuffers)
				{
					const bool item_selected = (buffer.first == currentBufferId);

					if (regex.m_empty == false && !regex.test(buffer.first))
						continue;

					ImGui::PushID(buffer.first.c_str());

					if (ImGui::Selectable(buffer.first.c_str(), item_selected))
					{
						valueChanged |= true;
						currentBufferId = buffer.first;
					}

					if (item_selected)
						ImGui::SetItemDefaultFocus();

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text(buffer.first.c_str());
						ImGui::EndTooltip();
					}

					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			return valueChanged;
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateContentArea(Scene::Scene& scene, Scene::Object* guiSettings)
		{
			// Current program id
			std::string& currentBufferId = guiSettings->component<GuiSettingsComponent>().m_bufferInspectorSettings.m_currentBuffer;

			// Label
			ImGui::TextDisabled(currentBufferId.c_str());

			#define ADD_ROW(NAME, FORMAT, ...) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, __VA_ARGS__); }
			#define ADD_ROW_STR(NAME, FORMAT, STR) { std::string const& tmp = STR; ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, tmp.c_str()); }
			#define ADD_ROW_BOOL(NAME, FORMAT, B) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, B ? "true" : "false"); }
			#define ADD_ROW_FLAGS(NAME, FORMAT, FLAGS, FLAG) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, ((FLAGS & FLAG) != 0) ? "true" : "false"); }

			// Typeset the buffer properties
			if (ImGui::BeginChild("Properties", ImVec2(0.0f, 0.0f), true))
			{
				GPU::GenericBuffer& buffer = scene.m_genericBuffers[currentBufferId];

				// Common
				ImGui::TextDisabled("Common");
				if (ImGui::BeginTable("BufferPropertiesCommon", 2))
				{
					ADD_ROW_STR("Type", "%s", GPU::enumToString(buffer.m_bufferType));
					ADD_ROW("Binding", "%d", buffer.m_bindingId);
					ADD_ROW_STR("Size", "%s", Units::bytesToString(buffer.m_size));
					ADD_ROW_STR("Element Size", "%s", Units::bytesToString(buffer.m_elementSize));
					ADD_ROW_BOOL("Indexed", "%s", buffer.m_indexed);
					ADD_ROW_BOOL("Immutable", "%s", buffer.m_immutable);
					ImGui::EndTable();
				}


				// Flags
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				ImGui::TextDisabled("Flags");
				if (ImGui::BeginTable("BufferPropertiesFlags", 2))
				{
					ADD_ROW_FLAGS("Dynamic Storage", "%s", buffer.m_flags, GL_DYNAMIC_STORAGE_BIT);
					ADD_ROW_FLAGS("Client Storage", "%s", buffer.m_flags, GL_CLIENT_STORAGE_BIT);
					ADD_ROW_FLAGS("Map Read", "%s", buffer.m_flags, GL_MAP_READ_BIT);
					ADD_ROW_FLAGS("Map Write", "%s", buffer.m_flags, GL_MAP_WRITE_BIT);
					ADD_ROW_FLAGS("Map Persistent", "%s", buffer.m_flags, GL_MAP_PERSISTENT_BIT);
					ADD_ROW_FLAGS("Map Coherent", "%s", buffer.m_flags, GL_MAP_COHERENT_BIT);
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Typeset the buffer contents
			if (ImGui::BeginChild("BufferContents", ImVec2(0.0f, 0.0f), true))
			{
			}
			ImGui::EndChild();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateBufferInspectorWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Buffer Inspector Window");

		// Window attributes
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = 0;
		if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		// Create the window
		if (ImGui::Begin("Buffer Inspector", nullptr, flags))
		{
			// Generate the settings editor
			if (ImGui::TreeNode("Settings"))
			{
				ImGui::SliderInt("Buffer Selector Height", &guiSettings->component<GuiSettingsComponent>().m_bufferInspectorSettings.m_bufferSelectorHeight, 1, 600);
				ImGui::TreePop();
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Current program id
			std::string& currentBufferId = guiSettings->component<GuiSettingsComponent>().m_bufferInspectorSettings.m_currentBuffer;

			// Fall back to the GUI program if the old program no longer exists
			if (auto bufferIt = scene.m_genericBuffers.find(currentBufferId); bufferIt == scene.m_genericBuffers.end() || bufferIt->second.m_buffer == 0)
			{
				currentBufferId = scene.m_genericBuffers.begin()->first;
			}

			// Which program to debug
			bool bufferChanged = BufferInspectorWindow::generateBufferSelector(scene, guiSettings, "Buffer", currentBufferId, 
				guiSettings->component<GuiSettingsComponent>().m_bufferInspectorSettings.m_bufferSelectorHeight, 
				guiSettings->component<GuiSettings::GuiSettingsComponent>().m_bufferInspectorSettings.m_selectorFilter);
			ImGui::RegexFilter("Filter", guiSettings->component<GuiSettings::GuiSettingsComponent>().m_bufferInspectorSettings.m_selectorFilter);

			// Reset the buffer flag
			if (bufferChanged)
			{
				guiSettings->component<GuiSettingsComponent>().m_bufferInspectorSettings.m_displayContents = false;
			}

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			#define ADD_ROW(NAME, FORMAT, ...) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, __VA_ARGS__); }
			#define ADD_ROW_STR(NAME, FORMAT, STR) { std::string const& tmp = STR; ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, tmp.c_str()); }
			#define ADD_ROW_BOOL(NAME, FORMAT, B) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, B ? "true" : "false"); }
			#define ADD_ROW_FLAGS(NAME, FORMAT, FLAGS, FLAG) { ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, ((FLAGS & FLAG) != 0) ? "true" : "false"); }

			// Typeset the buffer properties
			ImGui::TextDisabled(currentBufferId.c_str());
			if (ImGui::BeginChild("Properties", ImVec2(0.0f, 0.0f), true))
			{
				GPU::GenericBuffer& buffer = scene.m_genericBuffers[currentBufferId];

				// Common
				ImGui::TextDisabled("Common");
				if (ImGui::BeginTable("BufferPropertiesCommon", 2))
				{
					ADD_ROW_STR("Type", "%s", GPU::enumToString(buffer.m_bufferType));
					ADD_ROW("Binding", "%d", buffer.m_bindingId);
					ADD_ROW_STR("Size", "%s", Units::bytesToString(buffer.m_size));
					ADD_ROW_STR("Element Size", "%s", Units::bytesToString(buffer.m_elementSize));
					ADD_ROW_BOOL("Indexed", "%s", buffer.m_indexed);
					ADD_ROW_BOOL("Immutable", "%s", buffer.m_immutable);
					ImGui::EndTable();
				}


				// Flags
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				ImGui::TextDisabled("Flags");
				if (ImGui::BeginTable("BufferPropertiesFlags", 2))
				{
					ADD_ROW_FLAGS("Dynamic Storage", "%s", buffer.m_flags, GL_DYNAMIC_STORAGE_BIT);
					ADD_ROW_FLAGS("Client Storage", "%s", buffer.m_flags, GL_CLIENT_STORAGE_BIT);
					ADD_ROW_FLAGS("Map Read", "%s", buffer.m_flags, GL_MAP_READ_BIT);
					ADD_ROW_FLAGS("Map Write", "%s", buffer.m_flags, GL_MAP_WRITE_BIT);
					ADD_ROW_FLAGS("Map Persistent", "%s", buffer.m_flags, GL_MAP_PERSISTENT_BIT);
					ADD_ROW_FLAGS("Map Coherent", "%s", buffer.m_flags, GL_MAP_COHERENT_BIT);
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();

			// Add some vertical space
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// Typeset the buffer contents
			if (ImGui::BeginChild("BufferContents", ImVec2(0.0f, 0.0f), true))
			{
			}
			ImGui::EndChild();
		}

		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateMainRenderWindow(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		// Get the render settings object
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Push some necessary style settings
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		// Create the main window
		ImGui::SetNextWindowSize(ImVec2(512, 512), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Main Render", nullptr, ImGuiWindowFlags_NoCollapse))
		{
			// Extract the necessary sizes
			glm::vec2 resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
			glm::vec2 displaySize = { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
			glm::vec2 blitImageSize = displaySize;
			float renderAspectRatio = resolution.x / resolution.y;
			float displayAspectRatio = displaySize.x / displaySize.y;

			// Limit the display aspect ratio
			if (guiSettings->component<GuiSettings::GuiSettingsComponent>().m_blitFixedAspectRatio)
			{
				glm::vec2 resolutionToDisplayRatio = resolution / displaySize;
				if (resolutionToDisplayRatio.x > resolutionToDisplayRatio.y)
				{
					//blitImageSize = glm::vec2(displaySize.y * renderAspectRatio, displaySize.y);
					blitImageSize = glm::vec2(displaySize.x, displaySize.x / renderAspectRatio);
				}
				else
				{
					//blitImageSize = glm::vec2(displaySize.x, displaySize.x / renderAspectRatio);
					blitImageSize = glm::vec2(displaySize.y * renderAspectRatio, displaySize.y);
				}
			}
			else
			{
				if (displayAspectRatio < guiSettings->component<GuiSettings::GuiSettingsComponent>().m_blitAspectConstraint.x || displayAspectRatio > guiSettings->component<GuiSettings::GuiSettingsComponent>().m_blitAspectConstraint.y)
				{
					float correctedAspectRatio = glm::min(glm::max(displayAspectRatio, guiSettings->component<GuiSettings::GuiSettingsComponent>().m_blitAspectConstraint.x), guiSettings->component<GuiSettings::GuiSettingsComponent>().m_blitAspectConstraint.y);
					float correctedWidth = glm::min(displaySize.y * correctedAspectRatio, displaySize.x);
					blitImageSize = glm::vec2(correctedWidth, correctedWidth / correctedAspectRatio);
				}
			}

			// End UV coordinates
			ImVec2 uv = ImVec2{ resolution.x / scene.m_textures["ImGui_MainRenderTarget"].m_width, resolution.y / scene.m_textures["ImGui_MainRenderTarget"].m_height };

			// Copy over the main scene texture contents to the intermediate buffer
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glDisable(GL_SCISSOR_TEST);
			glDisable(GL_DEPTH_TEST);
			RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings, 
				RenderSettings::GB_WriteBuffer, RenderSettings::GB_ReadBuffer, 0, GL_READ_FRAMEBUFFER);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene.m_textures["ImGui_MainRenderTarget"].m_framebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, resolution.x, resolution.y, 0, 0, resolution.x, resolution.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			// Fill the alpha with 1s
			glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures["ImGui_MainRenderTarget"].m_framebuffer);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Display the image
			ImGui::SetCursorPosX((displaySize.x - blitImageSize.x) * 0.5f);
			ImGui::SetCursorPosY((displaySize.y - blitImageSize.y) * 0.5f);
			ImGui::Image(&scene.m_textures["ImGui_MainRenderTarget"].m_texture, ImVec2(blitImageSize.x, blitImageSize.y), ImVec2(0.0f, uv.y), ImVec2(uv.x, 0.0f));
		}

		// Reset the settings
		ImGui::PopStyleVar();

		// End the window immediately
		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool guiVisible(Scene::Scene& scene, Scene::Object* guiSettings)
	{
		// Extract the necessary components
		Scene::Object* input = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_INPUT);
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

		// Evaluate the state
		return
		(
			guiSettings->component<GuiSettings::GuiSettingsComponent>().m_showGui == true &&
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_focused == true &&
			(guiSettings->component<GuiSettings::GuiSettingsComponent>().m_showGuiNoInput == true || input->component<InputSettings::InputComponent>().m_input == true)
		);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool objectPicker(Scene::Scene& scene, const char* label, std::string& objectName, unsigned long long componentMask, 
		bool exactMatch, bool includeDisabled, bool thisGroupOnly)
	{
		// Find the set of object matching the filter
		std::vector<Scene::Object*> validObjects;
		Scene::filterObjects(scene, validObjects, componentMask, exactMatch, includeDisabled, thisGroupOnly);
		// Extract their names
		std::vector<std::string> validObjectNames(validObjects.size());
		std::transform(validObjects.begin(), validObjects.end(), validObjectNames.begin(), 
			[](Scene::Object* object) { return object->m_name; });
		// Generate the combo control and return it as the result
		return ImGui::Combo(label, objectName, validObjectNames);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Don't do anything if the gui needs to be hidden
		if (guiVisible(scene, object) == false)
			return;

		// Extract the imgui singleton
		ImGuiIO& io{ ImGui::GetIO() };

		// Update the global font scale
		io.FontGlobalScale = object->component<GuiSettings::GuiSettingsComponent>().m_fontScale;

		// Begin an imgui frame
		ImGui::NewFrame();

		// Set the font globally here
		ImGui::PushFont(scene.m_fonts[object->component<GuiSettings::GuiSettingsComponent>().m_font]);

		// Generate the main dockspace
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
		if (/*autoHideTabs*/ false) dockFlags |= ImGuiDockNodeFlags_AutoHideTabBar;
		if (object->component<GuiSettings::GuiSettingsComponent>().m_lockLayout) dockFlags |= ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoDockingInCentralNode;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Main Dockspace", nullptr, window_flags);
		generateGuiMenuBar(scene, object);
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockFlags);
		ImGui::PopStyleVar(3);
		ImGui::End();

		// Generate the available gui elements
		for (auto const& element : object->component<GuiSettings::GuiSettingsComponent>().m_guiElements)
			if (element.m_open) element.m_generator(scene, object);

		// imgui Demo window
		if (EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImguiDemo"))
		{
			ImGui::ShowDemoWindow(&EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImguiDemo"));
		}

		// imgui style editor
		if (EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImguiStyleEditor"))
		{
			if (ImGui::Begin("ImGui Style Editor", &EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImguiStyleEditor")))
				ImGui::ShowStyleEditor();
			ImGui::End();
		}

		// implot Demo window
		if (EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImplotDemo"))
		{
			ImPlot::ShowDemoWindow(&EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImplotDemo"));
		}

		// imgui style editor
		if (EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImPlotStyleEditor"))
		{
			if (ImGui::Begin("ImPlot Style Editor", &EditorSettings::editorProperty<bool>(scene, object, "GuiSettings_ShowImPlotStyleEditor")))
				ImPlot::ShowStyleEditor();
			ImGui::End();
		}

		// Pop the global font
		ImGui::PopFont();

		// Save the gui state after each frame
		if (object->component<GuiSettings::GuiSettingsComponent>().m_saveGuiState)
		{
			float time = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime;
			float elapsed = time - object->component<GuiSettings::GuiSettingsComponent>().m_guiStateLastSaved;

			if (elapsed >= object->component<GuiSettings::GuiSettingsComponent>().m_saveGuiStateInterval)
			{
				saveGuiState(scene, object);
				object->component<GuiSettings::GuiSettingsComponent>().m_guiStateLastSaved = time;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Don't do anything if the gui needs to be hidden
		if (guiVisible(scene, object) == false)
			return;

		// Extract the imgui singleton
		ImGuiIO& io{ ImGui::GetIO() };

		// Render the GUI
		ImGui::Render();
		ImDrawData* drawData = ImGui::GetDrawData();

		Profiler::ScopedGpuPerfCounter perfCounter(scene, "GUI");

		int fb_width{ (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x) };
		int fb_height{ (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y) };

		drawData->ScaleClipRects(io.DisplayFramebufferScale);

		// Backup GL state
		GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
		GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
		GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
		GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
		GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
		GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
		GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
		GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
		GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
		GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
		GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
		GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
		GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
		GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
		GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glActiveTexture(GL_TEXTURE0);

		// Bind the imgui shader
		Scene::bindShader(scene, "Imgui", "imgui");

		// Setup viewport, orthographic projection matrix
		glViewport(0, 0, fb_width, fb_height);
		auto proj = glm::ortho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(proj));

		// Generate the vao and buffers
		GLuint vao, vbo, ibo;
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ibo);

		// Configure the vao
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_POSITION);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_UV);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_COLOR);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

		// Render command lists
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = drawData->CmdLists[n];
			const ImDrawIdx* idx_buffer_offset = 0;

			glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback)
				{
					pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, *(GLuint*)pcmd->TextureId);
					glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
					glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
				}
				idx_buffer_offset += pcmd->ElemCount;
			}
		}

		// Unbind the vao and buffers
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// Delete the buffers and vao
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ibo);

		// Restore modified GL state
		glUseProgram(last_program);
		glActiveTexture(last_active_texture);
		glBindTexture(GL_TEXTURE_2D, last_texture);
		glBindVertexArray(last_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
		glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
		glBlendFunc(last_blend_src, last_blend_dst);
		if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
		if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
		glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
		glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
	}
}