#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "EditorSettings.h"

namespace GuiSettings
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "GuiSettings";
	static constexpr const char* DISPLAY_NAME = "Gui Settings";
	static constexpr const char* CATEGORY = "Settings";

	////////////////////////////////////////////////////////////////////////////////
	struct GuiElement
	{
		using Generator = void(*)(Scene::Scene& scene, Scene::Object* guiSettings);

		std::string m_name;
		Generator m_generator;
		bool m_open = true;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct StatWindowSettings
	{
		// Scale factor applied to the indentations
		float m_indentScale = 1.0f;

		// Which nodes are currently open
		std::vector<std::string> m_openNodes;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct ProfilerChartsSettings
	{
		// Node labeling mode
		meta_enum(NodeLabelMode, int, CurrentCategory, ObjectCategory, FullCategory);

		// Node labeling mode
		NodeLabelMode m_nodeLabelMode = ObjectCategory;

		// Size of the graphs
		float m_graphHeight = 120.0f;

		// Number of frames to show
		int m_numFramesToShow = 256;

		// How many frames to wait between updates
		int m_updateRatio = 1;

		// Averaging window size
		int m_avgWindowSize = 33;

		// Whether to freeze the current state or not
		bool m_freeze = false;

		// Which nodes to include
		std::unordered_set<std::string> m_nodesToShow;

		// Plot colors
		glm::vec4 m_plotColor = glm::vec4{ 1.0f, 1.0f, 0.0f, 0.21f };
		glm::vec4 m_avgPlotColor = glm::vec4{ 0.81f, 0.21f, 0.21f, 1.0f };

		// -------------- Internal state ------------

		// Start frame id
		int m_startFrameId = 0;

		// End frame id
		int m_endFrameId = 0;

		// Last updated frame
		int m_lastFrameDrawn = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct LogOutputSettings
	{
		// Which channels to show
		bool m_showTrace = false;
		bool m_showDebug = false;
		bool m_showInfo = true;
		bool m_showWarning = true;
		bool m_showError = true;

		// Whether to show the message date or not
		bool m_showDate = true;

		// Whether to show the message severity or not
		bool m_showSeverity = true;

		// Whether to show the source region or not
		bool m_showRegion = false;

		// Whether to show the quick filter bar or not
		bool m_showQuickFilter = true;

		// Whether to autoscroll or not
		bool m_autoScroll = true;

		// Whether to show long (multiline) messages or not
		bool m_longMessages = false;

		// Spacing between the individual messages
		float m_messageSpacing = 0.0f;

		// Filter text
		ImGui::FilterRegex m_includeFilter;
		ImGui::FilterRegex m_discardFilter = { "$Region$:OpenGL" };

		// Various message colors
		glm::vec3 m_traceColor = glm::vec3{ 0.5f, 0.5f, 0.5f };
		glm::vec3 m_debugColor = glm::vec3{ 0.0f, 0.0f, 1.0f };
		glm::vec3 m_infoColor = glm::vec3{ 1.0f, 1.0f, 1.0f };
		glm::vec3 m_warningColor = glm::vec3{ 1.0f, 1.0f, 0.0f };
		glm::vec3 m_errorColor = glm::vec3{ 1.0f, 0.0f, 0.0f };

		// -------------- Internal state ------------

		// Scrollbar max value
		float m_maxScrollY = 0.0f;

		// The frame in which the filters last changed
		int m_lastFilterUpdate = -1;

		// State of a cached message
		struct CachedMessageState
		{
			const Debug::InMemoryLogBuffer* m_source = nullptr;
			time_t m_date = 0;
			int m_filterTime = 0;
			bool m_visible = false;
		};

		// Cached message state for each message
		std::vector<CachedMessageState> m_cachedMessageState;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderInspectorSettings
	{
		// Whether we should sort the entries or not
		meta_enum(VariableNameMethod, int, Full, NameOnly);

		// How to display variable names
		VariableNameMethod m_variableNames = NameOnly;

		// Minimum and maximum block rows to display
		glm::ivec2 m_numBlockRows = { 4, 6 };

		// Minimum and maximum block rows to display
		glm::ivec2 m_numVariablesRows = { 4, 10 };

		// Height of the shader selector block
		int m_shaderSelectorHeight = 200;

		// Filter for the materials
		ImGui::FilterRegex m_selectorFilter;

		// -------------- Internal state ------------

		// Name of the current program
		std::string m_currentProgram;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct BufferInspectorSettings
	{
		// Height of the shader selector block
		int m_bufferSelectorHeight = 200;

		// Filter for the materials
		ImGui::FilterRegex m_selectorFilter;

		// Whether we want to display buffer contents or not
		bool m_displayContents = false;

		// -------------- Internal state ------------

		// Name of the current buffer
		std::string m_currentBuffer;

		// Program and block to extract the contents from
		std::string m_contentsProgram;
		std::string m_contentsBlock;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct TextureInspectorSettings
	{
		// Height of the texture selector block
		int m_textureSelectorHeight = 200;

		// Height of the image in the inspector preview
		int m_previewHeight = 300;

		// Height of the image in a tooltip
		int m_tooltipHeight = 1024 + 512;

		// Size of 1 block in for 1D textures
		int m_1dTextureBlockSize = 64;

		// Size of 1 block in for 1D texture array slices
		int m_1dArrayTextureBlockSize = 8;

		// Whether we should update the preview texture every frame for non-2D textures
		bool m_liveUpdate = true;

		// How much to zoom in
		float m_zoomFactor = 1.0f;

		// Filter for the materials
		ImGui::FilterRegex m_selectorFilter;

		// -------------- Internal state ------------

		// Which slice of an array texture to display
		int m_arraySliceId = 0;

		// Which LOD level to show
		int m_lodLevelId = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct MaterialEditorSettings
	{
		// Height of the material selector block
		int m_materialSelectorHeight = 200;

		// Height of the image in the inspector preview
		int m_previewHeight = 200;

		// Height of the image in a tooltip
		int m_tooltipHeight = 1024;

		// Filter for the materials
		ImGui::FilterRegex m_selectorFilter;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Gui settings component. */
	struct GuiSettingsComponent
	{
		// Whether the interface layout should be locked in place or not
		bool m_lockLayout = false;

		// Whether the GUI should be drawn or not
		bool m_showGui = true;

		// Whether the GUI should be drawn or not while input is routed to the simulation
		bool m_showGuiNoInput = false;

		// The various GUI elements
		std::vector<GuiElement> m_guiElements;

		// Whether to group objects by type or not
		bool m_groupObjects = false;
		
		// Should we hide disabled groups or not
		bool m_hideDisabledGroups = true;

		// Whether we should be using fixed aspect ratio or not
		bool m_blitFixedAspectRatio = true;

		// Whether to persis the GUI open state or not
		bool m_saveGuiState = true;

		// How often to save the gui state
		float m_saveGuiStateInterval = 2.0f;

		// Minimum and maximum blit aspect ratio
		glm::vec2 m_blitAspectConstraint = glm::vec2(1.0f, 1.75f);

		// Gui text scaling
		float m_fontScale = 0.0f;

		// Plot line weight
		float m_plotLineWeight = 0.0f;

		// Gui font
		std::string m_font = "";

		// Style of GUI
		std::string m_guiStyle = "Classic";

		// Plot color map
		std::string m_plotColorMap = "Deep";

		// Stat window settings
		StatWindowSettings m_statWindowSettings;

		// The log output settings
		LogOutputSettings m_logOutputSettings;

		// Profiler charts settings
		ProfilerChartsSettings m_profilerChartsSettings;

		// Shader inspector settings
		ShaderInspectorSettings m_shaderInspectorSettings;

		// Buffer inspector settings
		BufferInspectorSettings m_bufferInspectorSettings;

		// Texture inspector settings
		TextureInspectorSettings m_textureInspectorSettings;

		// Material editor size
		MaterialEditorSettings m_materialEditorSettings;

		// ========= Internal state ===============
		
		// Name of the persistent gui state file
		std::string m_guiStateFileName;

		// When was the gui state last save
		float m_guiStateLastSaved = 0.0f;

		// DPI scale factor
		float m_dpiScale = 1.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);
	
	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool guiVisible(Scene::Scene& scene, Scene::Object* guiSettings);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void startEditingMaterial(Scene::Scene& scene, std::string const& materialName);

	////////////////////////////////////////////////////////////////////////////////
	bool objectPicker(Scene::Scene& scene, const char* label, std::string& objectName, unsigned long long componentMask, 
		bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(GUI_SETTINGS, GuiSettingsComponent, GuiSettings::GuiSettingsComponent)
DECLARE_OBJECT(GUI_SETTINGS, COMPONENT_ID_GUI_SETTINGS, COMPONENT_ID_EDITOR_SETTINGS)