#include "PCH.h"
#include "Demo.h"

namespace Demo
{
	////////////////////////////////////////////////////////////////////////////////
	/** The main scene. */
	Scene::Scene g_scene;

	////////////////////////////////////////////////////////////////////////////////
	DemoScenes& demoScenes()
	{ 
		static DemoScenes s_demoScenes;
		return s_demoScenes;
	};

	////////////////////////////////////////////////////////////////////////////////
	void mainLoop()
	{
		// Show the window in windowed mode
		glfwShowWindow(g_scene.m_context.m_window);
		glfwSetWindowMonitor(g_scene.m_context.m_window, nullptr, 100, 100, 800, 600, 0);
		glfwMaximizeWindow(g_scene.m_context.m_window);

		// Run the main loop
		Scene::mainLoop(g_scene);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Common settings for cameras
	void commonCameraProperties(Scene::Scene& scene, Scene::Object& object, bool locked)
	{
		//object.component<Camera::CameraComponent>().m_near = 0.01f;
		//object.component<Camera::CameraComponent>().m_far = 100.0f;
		object.component<Camera::CameraComponent>().m_near = 0.01f;
		object.component<Camera::CameraComponent>().m_far = 50.0f;
		object.component<Camera::CameraComponent>().m_fovy = glm::radians(Config::AttribValue("fov").get<float>());
		object.component<Camera::CameraComponent>().m_fovyMin = Config::AttribValue("fov").get<float>();
		object.component<Camera::CameraComponent>().m_fovyMax = Config::AttribValue("fov").get<float>();
		object.component<Camera::CameraComponent>().m_apertureMethod = Camera::CameraComponent::Fixed;
		object.component<Camera::CameraComponent>().m_fixedAperture = Config::AttribValue("aperture").get<float>();
		object.component<Camera::CameraComponent>().m_apertureMin = 2.0f;
		object.component<Camera::CameraComponent>().m_apertureMax = 7.0f;
		object.component<Camera::CameraComponent>().m_focusDistance = Config::AttribValue("focus").get<float>();
		object.component<Camera::CameraComponent>().m_focusDistanceMin = 0.25f;
		object.component<Camera::CameraComponent>().m_focusDistanceMax = 8.0f;
		object.component<Camera::CameraComponent>().m_movementSpeed = 2.0f;
		object.component<Camera::CameraComponent>().m_mouseTurnSpeed = glm::radians(2.0f);
		object.component<Camera::CameraComponent>().m_keyTurnSpeed = glm::radians(60.0f);
		object.component<Camera::CameraComponent>().m_strollMultiplier = 0.1f;
		object.component<Camera::CameraComponent>().m_locked = locked;
	};

	////////////////////////////////////////////////////////////////////////////////
	void addCoreObjects(Scene::Scene& scene)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Core Objects");

		// Add the delayed job queue
		auto& delayedJobs = createObject(scene, Scene::OBJECT_TYPE_DELAYED_JOBS,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;

			object.component<DelayedJobs::DelayedJobsComponent>().m_processDisabledObjects = false;
			object.component<DelayedJobs::DelayedJobsComponent>().m_consumeDisabledObjects = true;
		}));

		// Add the debug object.
		auto& debugSettings = createObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;

			object.component<DebugSettings::DebugSettingsComponent>().m_crtSettings.m_debugMemory = true;
		}));

		// Add the simulation settings.
		auto& simulationSettings = createObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;
		}));

		// Add the input
		auto& input = createObject(scene, Scene::OBJECT_TYPE_INPUT,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;
			object.m_enabled = true;

			object.component<InputSettings::InputComponent>().m_input = false;
		}));

		// Add the rendering settings.
		auto& renderSettings = createObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;

			object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_renderer = RenderSettings::Renderer_meta_from_name(GPU::renderer())->value;
			object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitMode = RenderSettings::Fullscreen;
			object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitAnchor = RenderSettings::BotRight;
			object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitScale = 1.0f;
			object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitPercentage = glm::vec2(0.8f);
			object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitFiltering = RenderSettings::Linear;

			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_msaa = GPU::numMsaaSamples();
			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_forceMsaaBuffers = false;
			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels = GPU::voxelGridSize();
			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_anisotropicVoxels = true;
			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_gbufferDataFormat = RenderSettings::F16;
			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelGbufferDataFormat = RenderSettings::UI8;
			object.component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelRadianceDataFormat = RenderSettings::UI8;

			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_backfaceCull = RenderSettings::AllLayers;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_normalMapping = RenderSettings::NormalMapping;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_displacementMapping = RenderSettings::BasicParallaxMapping;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_shadowMethod = RenderSettings::ShadowMapping;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_depthPrepass = false;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_wireframeMesh = false;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_backgroundRendering = false;
			object.component<RenderSettings::RenderSettingsComponent>().m_features.m_showApertureSize = false;

			object.component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter = 180.0f;
			object.component<RenderSettings::RenderSettingsComponent>().m_units.m_nitsPerUnit = 5.0f;

			object.component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers = GPU::numLayers();
			object.component<RenderSettings::RenderSettingsComponent>().m_layers.m_depthPeelAlgorithm = RenderSettings::UmbraThresholding;
			object.component<RenderSettings::RenderSettingsComponent>().m_layers.m_minimumSeparationDepthGap = 0.5f;
			object.component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraScaleFactor = 1.0f;
			object.component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraMin = 0.0f;
			object.component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraMax = 1000.0f;

			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingMode = RenderSettings::DirectAndIndirect;
			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_brdfModel = RenderSettings::CookTorrance;
			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingFiltering = RenderSettings::Point;
			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_voxelShadingMode = RenderSettings::NormalWeighted;
			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_voxelDilationMode = RenderSettings::NormalDilation;
			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_gamma = 2.2f;
			object.component<RenderSettings::RenderSettingsComponent>().m_lighting.m_aoAffectsDirectLighting = false;
		}));

		// Add the GUI settings.
		auto& guiSettings = createObject(scene, Scene::OBJECT_TYPE_GUI_SETTINGS,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;

			object.component<GuiSettings::GuiSettingsComponent>().m_showGuiNoInput = true;
			object.component<GuiSettings::GuiSettingsComponent>().m_font = "Roboto";
			object.component<GuiSettings::GuiSettingsComponent>().m_guiStyle = "Custom Dark";

			object.component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodeLabelMode = GuiSettings::ProfilerChartsSettings::ObjectCategory;
			object.component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_graphHeight = 150.0f;
			object.component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_numFramesToShow = 100;
			object.component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_updateRatio = 1;
			object.component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_avgWindowSize = 8;
		}));

		// Add the video record settings.
		auto& recordSettings = createObject(scene, Scene::OBJECT_TYPE_RECORD_SETTINGS,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.component<EditorSettings::EditorSettingsComponent>().m_allowDisable = false;

			object.component<RecordSettings::RecordSettingsComponent>().m_videoFrameRate = 60;
			object.component<RecordSettings::RecordSettingsComponent>().m_includeGui = true;
			//object.component<RecordSettings::RecordSettingsComponent>().m_videoCompressor = "xvid";
			object.component<RecordSettings::RecordSettingsComponent>().m_outputFileName = "Generated/Recording/recording-$date$";
		}));
	}

	////////////////////////////////////////////////////////////////////////////////
	void addDemoSceneCommonObjects(Scene::Scene& scene, DemoScene const& demoScene)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, demoScene.m_name);

		// Extract some of the used objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// LIGHTING
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the voxel GI object
		auto& voxelGI = createObject(scene, demoScene.m_name + " Voxel GI", Scene::OBJECT_TYPE_VOXEL_GLOBAL_ILLUMINATION, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces = 1;
			//object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces = 8;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_radianceDecayRate = 0.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionDecayRate = 1.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionExponent = 1.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_minOcclusion = 0.05f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceStartOffset = 2.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularTraceStartOffset = 1.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceNormalOffset = 1.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularTraceNormalOffset = 0.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_maxTraceDistance = 100.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseIntensity = 1.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularIntensity = 1.0f;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseAperture = glm::radians(60.0f);
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularApertureMin = glm::radians(3.0f);
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularApertureMax = glm::radians(16.0f);
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicDiffuse = true;
			object.component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicSpecular = true;
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// BACKGROUNDS
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the volumetric clouds background.
		auto& volumetricClouds = createObject(scene, demoScene.m_name + " Volumetric Clouds", Scene::OBJECT_TYPE_VOLUMETRTIC_CLOUDS,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<VolumetricClouds::VolumetricCloudsComponent>().m_renderToAllLayers = true;
			object.component<VolumetricClouds::VolumetricCloudsComponent>().m_density = 0.2f;
			object.component<VolumetricClouds::VolumetricCloudsComponent>().m_cloudCoverage = 0.2f;
		}));

		// Add the black background.
		auto& backgroundBlack = createObject(scene, demoScene.m_name + " Black Background", Scene::OBJECT_TYPE_SKYBOX, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<SkyBox::SkyBoxComponent>().m_tint = glm::vec3(0.0f);
			object.component<SkyBox::SkyBoxComponent>().m_renderToAllLayers = true;
		}));

		// Add the blue background.
		auto& backgroundGray = createObject(scene, demoScene.m_name + " Gray Background", Scene::OBJECT_TYPE_SKYBOX, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<SkyBox::SkyBoxComponent>().m_tint = glm::vec3(23.0f / 255.0f, 23.0f / 255.0f, 23.0f / 255.0f);
			object.component<SkyBox::SkyBoxComponent>().m_renderToAllLayers = true;
		}));

		// Add the blue background.
		auto& backgroundDarkGray = createObject(scene, demoScene.m_name + " Dark Gray Background", Scene::OBJECT_TYPE_SKYBOX, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<SkyBox::SkyBoxComponent>().m_tint = glm::vec3(7.0f / 255.0f, 7.0f / 255.0f, 7.0f / 255.0f);
			object.component<SkyBox::SkyBoxComponent>().m_renderToAllLayers = true;
		}));

		// Add the blue background.
		auto& backgroundBlue = createObject(scene, demoScene.m_name + " Blue Background", Scene::OBJECT_TYPE_SKYBOX, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<SkyBox::SkyBoxComponent>().m_tint = glm::vec3(72.0f / 255.0f, 215.0f / 255.0f, 215.0f / 255.0f);
			object.component<SkyBox::SkyBoxComponent>().m_renderToAllLayers = true;
		}));

		// Add the skybox.
		auto& skybox = createObject(scene, demoScene.m_name + " Skybox", Scene::OBJECT_TYPE_SKYBOX, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<SkyBox::SkyBoxComponent>().m_textureName = "Textures/Skybox/Thunder";
			object.component<SkyBox::SkyBoxComponent>().m_renderToAllLayers = true;
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// FILTERS
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Common settings for tone mappers
		auto toneMapCameraProperties = [](Scene::Object& object)
		{
			// Exposure determination method
			object.component<ToneMap::TonemapComponent>().m_exposureMethod = ToneMap::TonemapComponent::AutoExposure;
			object.component<ToneMap::TonemapComponent>().m_fixedExposure = 1.0f;
			object.component<ToneMap::TonemapComponent>().m_exposureBias = 1.0f;
			object.component<ToneMap::TonemapComponent>().m_keyMethod = ToneMap::TonemapComponent::AutoKey;
			object.component<ToneMap::TonemapComponent>().m_fixedKey = 0.115f;

			// Eye adaptation
			object.component<ToneMap::TonemapComponent>().m_adaptationMethod = ToneMap::TonemapComponent::Exponential;
			object.component<ToneMap::TonemapComponent>().m_adaptationRate = 2.0f;
			object.component<ToneMap::TonemapComponent>().m_minAvgLuminance = 0.001f;
			object.component<ToneMap::TonemapComponent>().m_localLuminanceMipOffset = 2.0f;
			object.component<ToneMap::TonemapComponent>().m_maxLocalLuminanceContribution = 0.3f;

			// Operator
			object.component<ToneMap::TonemapComponent>().m_operator = ToneMap::TonemapComponent::Aces;
			object.component<ToneMap::TonemapComponent>().m_linearWhite = 11.2f;

			// Filmic tone map parameters
			object.component<ToneMap::TonemapComponent>().m_filmicSettings.m_shoulderStrength = 0.15f;
			object.component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearStrength = 0.06f;
			object.component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearAngle = 0.35f;
			object.component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeStrength = 0.2f;
			object.component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeNumerator = 0.02f;
			object.component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeDenominator = 0.3f;
		};

		// Add the tone mapper.
		auto& toneMapFixedExposure = createObject(scene, demoScene.m_name + " Tone Map (Fixed Exposure)", Scene::OBJECT_TYPE_TONEMAP, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			toneMapCameraProperties(object);

			// Exposure determination method
			object.component<ToneMap::TonemapComponent>().m_exposureMethod = ToneMap::TonemapComponent::FixedExposure;
		}));

		// Add the auto-exposure tone mapper.
		auto& toneMapAutoExposure = createObject(scene, demoScene.m_name + " Tone Map (Auto Exposure)", Scene::OBJECT_TYPE_TONEMAP,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			toneMapCameraProperties(object);

			// Exposure determination method
			object.component<ToneMap::TonemapComponent>().m_exposureMethod = ToneMap::TonemapComponent::AutoExposure;
			object.component<ToneMap::TonemapComponent>().m_keyMethod = ToneMap::TonemapComponent::AutoKey;
		}));

		// Add the post-lightning FXAA for the no MSAA case.
		auto& fxaaPostLightingNoMSAA = createObject(scene, demoScene.m_name + " FXAA Post Lighting [no MSAA]", Scene::OBJECT_TYPE_FXAA, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			//object.m_enabled = GPU::numMsaaSamples() < 2;
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<Fxaa::FxaaComponent>().m_colorSpace = Fxaa::FxaaComponent::HDR;
			object.component<Fxaa::FxaaComponent>().m_domain = Fxaa::FxaaComponent::AfterLighting;
			object.component<Fxaa::FxaaComponent>().m_dirReduceMin = 0.01f;
			object.component<Fxaa::FxaaComponent>().m_dirReduceMultiplier = 0.33f;
			object.component<Fxaa::FxaaComponent>().m_maxBlur = 8.0f;
		}));

		// Add the post-lightning FXAA for the MSAA Case.
		auto& fxaaPostLightingMSAA = createObject(scene, demoScene.m_name + " FXAA Post Lighting [MSAA]", Scene::OBJECT_TYPE_FXAA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			//object.m_enabled = GPU::numMsaaSamples() >= 2;
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<Fxaa::FxaaComponent>().m_colorSpace = Fxaa::FxaaComponent::HDR;
			object.component<Fxaa::FxaaComponent>().m_domain = Fxaa::FxaaComponent::AfterLighting;
			object.component<Fxaa::FxaaComponent>().m_dirReduceMin = 0.01f;
			object.component<Fxaa::FxaaComponent>().m_dirReduceMultiplier = 0.5f;
			object.component<Fxaa::FxaaComponent>().m_maxBlur = 8.0f;
		}));

		// Add the post-postprocessing FXAA.
		auto& fxaaPostPostprocessing = createObject(scene, demoScene.m_name + " FXAA Post Filters", Scene::OBJECT_TYPE_FXAA, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<Fxaa::FxaaComponent>().m_domain = Fxaa::FxaaComponent::AfterPostprocessing;
			object.component<Fxaa::FxaaComponent>().m_dirReduceMin = 0.01f;
			object.component<Fxaa::FxaaComponent>().m_dirReduceMultiplier = 0.33f;
			object.component<Fxaa::FxaaComponent>().m_maxBlur = 8.0f;
		}));

		// Add the debug visualization object.
		auto& motionBlur = createObject(scene, demoScene.m_name + " Motion Blur", Scene::OBJECT_TYPE_MOTION_BLUR, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<MotionBlur::MotionBlurComponent>().m_velocityScaleFactor = 0.5f;
			object.component<MotionBlur::MotionBlurComponent>().m_maxVelocity = 32.0f;
			object.component<MotionBlur::MotionBlurComponent>().m_numTaps = 16;
			object.component<MotionBlur::MotionBlurComponent>().m_tileSize = 20;
			object.component<MotionBlur::MotionBlurComponent>().m_centerWeight = 0.5f;
			object.component<MotionBlur::MotionBlurComponent>().m_tileFalloff = 0.5f;
			object.component<MotionBlur::MotionBlurComponent>().m_interpolationThreshold = 1.0f;
			object.component<MotionBlur::MotionBlurComponent>().m_jitterScale = 0.5f;
		}));

		// Add the gbuffer visualization object.
		auto& gbufferVisualization = createObject(scene, demoScene.m_name + " GBuffer Visualization", Scene::OBJECT_TYPE_DEBUG_VISUALIZATION, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget = DebugVisualization::GBuffer;
			object.component<DebugVisualization::DebugVisualizationComponent>().m_gbufferComponent = DebugVisualization::Albedo;
			object.component<DebugVisualization::DebugVisualizationComponent>().m_layer = 0;
		}));

		// Add the debug visualization object.
		auto& voxelGridVisualization = createObject(scene, demoScene.m_name + " Voxel Grid Visualization", Scene::OBJECT_TYPE_DEBUG_VISUALIZATION, 
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, demoScene.m_groupName);

			object.component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget = DebugVisualization::VoxelGrid;
			object.component<DebugVisualization::DebugVisualizationComponent>().m_voxelComponent = DebugVisualization::Radiance;
			object.component<DebugVisualization::DebugVisualizationComponent>().m_voxelFace = DebugVisualization::Isotropic;
		}));
	}

	////////////////////////////////////////////////////////////////////////////////
	void init()
	{
		// Start a new profiler frame
		Profiler::beginFrame(g_scene);

		Profiler::ScopedCpuPerfCounter perfCounter(g_scene, "Initialization");

		Debug::log_debug() << std::string(80, '=') << Debug::end;
		Debug::log_debug() << "List of available components:" << Debug::end;
		Debug::log_debug() << std::string(80, '-') << Debug::end;
		for (auto componentType : Scene::componentTypes())
			Debug::log_debug() << "  - " <<
				Scene::componentNames()[componentType] << " - " <<
				"ID: " << componentType << ", " <<
				"mask: " << std::bit_mask(componentType) <<
				Debug::end;
		Debug::log_debug() << std::string(80, '-') << Debug::end;
		for (auto objectType : Scene::objectTypes())
			Debug::log_debug() << "  - " <<
				Scene::objectNames()[objectType] << " - " <<
				"mask: " << objectType <<
				Debug::end;
		Debug::log_debug() << std::string(80, '=') << Debug::end;

		// Set the basic properties of the scene
		g_scene.m_name = "Test-Scene";
		g_scene.m_isDemoScene = true;

		// Store the context object
		g_scene.m_context = Context::g_context;
		glfwSetWindowUserPointer(Context::g_context.m_window, &g_scene);

		// Generate the list of available skybox textures
		g_scene.m_skyboxNames =
		{
			"Textures/Skybox/Thunder",
		};

		// Generate the list of available LUT textures
		g_scene.m_lutNames =
		{
			"Textures/FX/LUT/identity.png",
			"Textures/FX/LUT/sepia.png",
			"Textures/FX/LUT/bright.png",
			"Textures/FX/LUT/greenish.png",
			"Textures/FX/LUT/reddish.png",
			"Textures/FX/LUT/LUT_BleachBypass.png",
			"Textures/FX/LUT/LUT_CandleLight.png",
			"Textures/FX/LUT/LUT_CoolContrast.png",
			"Textures/FX/LUT/LUT_DesaturatedFog.png",
			"Textures/FX/LUT/LUT_Evening.png",
			"Textures/FX/LUT/LUT_Fall.png",
			"Textures/FX/LUT/LUT_Filmic.png",
			"Textures/FX/LUT/LUT_Filmic2.png",
			"Textures/FX/LUT/LUT_Filmic3.png",
			"Textures/FX/LUT/LUT_Filmic4.png",
			"Textures/FX/LUT/LUT_Filmic5.png",
			"Textures/FX/LUT/LUT_Filmic6.png",
			"Textures/FX/LUT/LUT_Filmic7.png",
			"Textures/FX/LUT/LUT_Filmic8.png",
			"Textures/FX/LUT/LUT_Filmic9.png",
			"Textures/FX/LUT/LUT_MatrixBlue.png",
			"Textures/FX/LUT/LUT_MatrixGreen.png",
			"Textures/FX/LUT/LUT_Night_Dark.png",
			"Textures/FX/LUT/LUT_Night1.png",
			"Textures/FX/LUT/LUT_Night2.png",
			"Textures/FX/LUT/LUT_StrongAmber.png",
			"Textures/FX/LUT/LUT_Warm.png",
			"Textures/FX/LUT/LUT_WarmContrast.png",
		};

		Debug::log_info() << "Initializing core objects..." << Debug::end;

		// Init the core objects
		addCoreObjects(g_scene);
		
		Debug::log_info() << "Initializing demo scenes..." << Debug::end;

		// Initialize the various demo scenes
		for (auto const& demoScene : demoScenes())
		{
			Debug::log_info() << "  - " << demoScene.m_name << "..." << Debug::end;

			// Register the object group
			SimulationSettings::createGroup(g_scene, "Scene", demoScene.m_groupName);

			// common objects
			Demo::addDemoSceneCommonObjects(g_scene, demoScene);

			// Scene-specific objects
			demoScene.m_initializer(g_scene);
		}

		Debug::log_info() << "Preparing scene..." << Debug::end;

		// Set up the scene
		Scene::setupScene(g_scene);

		Debug::log_info() << "Loading resources..." << Debug::end;

		// Generate and load resources
		for (auto const& resourceType : Scene::ResourceType_meta.members)
		{
			Debug::log_info() << "  - " << Scene::ResourceType_value_to_string(resourceType.value) << "..." << Debug::end;

			loadResources(g_scene, resourceType.value);
		}

		Debug::log_info() << "Performing post-initialization tasks..." << Debug::end;

		// Carry out the queued delayed jobs to finish object initialization
		DelayedJobs::doJobs(g_scene, Scene::findFirstObject(g_scene, Scene::OBJECT_TYPE_DELAYED_JOBS), false, false);
	}

	////////////////////////////////////////////////////////////////////////////////
	void cleanup()
	{
		Profiler::ScopedCpuPerfCounter perfCounter(g_scene, "Cleanup");

		// Tear down the scene
		Scene::teardownScene(g_scene);

		// Unload resources
		Scene::unloadResources(g_scene);
	}
}