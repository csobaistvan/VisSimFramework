#include "PCH.h"
#include "RenderSettings.h"
#include "SimulationSettings.h"
#include "DelayedJobs.h"
#include "../Lighting/ShadowMap.h"
#include "../Lighting/VoxelGlobalIllumination.h"
#include "../Rendering/Camera.h"

namespace RenderSettings
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(RENDER_SETTINGS);
	DEFINE_OBJECT(RENDER_SETTINGS);
	REGISTER_OBJECT_UPDATE_CALLBACK(RENDER_SETTINGS, AFTER, SIMULATION_SETTINGS);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Uniforms [Begin]", OpenGL, BEFORE, "", 0, &RenderSettings::uniformsBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Uniforms [End]", OpenGL, AFTER, "Uniforms [Begin]", 0, &RenderSettings::uniformsEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Depth Prepass [Begin]", OpenGL, AFTER, "Uniforms [End]", 0, &RenderSettings::depthPrePassBeginOpenGL, &RenderSettings::depthPrePassTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Depth Prepass [End]", OpenGL, AFTER, "Depth Prepass [Begin]", 0, &RenderSettings::depthPrePassEndOpenGL, &RenderSettings::depthPrePassTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "GBuffer Basepass [Begin]", OpenGL, AFTER, "Depth Prepass [End]", 0, &RenderSettings::gbufferBasePassBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "GBuffer Basepass [End]", OpenGL, AFTER, "GBuffer Basepass [Begin]", 0, &RenderSettings::gbufferBasePassEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Voxel Basepass [Begin]", OpenGL, AFTER, "GBuffer Basepass [End]", 0, &RenderSettings::voxelBasePassBeginOpenGL, &RenderSettings::voxelBasePassTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Voxel Basepass [End]", OpenGL, AFTER, "Voxel Basepass [Begin]", 0, &RenderSettings::voxelBasePassEndOpenGL, &RenderSettings::voxelBasePassTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Shadow Maps [Begin]", OpenGL, AFTER, "Voxel Basepass [End]", 0, &RenderSettings::shadowMapsBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Shadow Maps [End]", OpenGL, AFTER, "Shadow Maps [Begin]", 0, &RenderSettings::shadowMapsEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Voxel Lighting [Begin]", OpenGL, AFTER, "Shadow Maps [End]", 0, &RenderSettings::voxelLightingBeginOpenGL, &RenderSettings::voxelLightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Voxel Lighting [End]", OpenGL, AFTER, "Voxel Lighting [Begin]", 0, &RenderSettings::voxelLightingEndOpenGL, &RenderSettings::voxelLightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Lighting [Begin]", OpenGL, AFTER, "Voxel Lighting [End]", 0, &RenderSettings::lightingBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Lighting [End]", OpenGL, AFTER, "Lighting [Begin]", 0, &RenderSettings::lightingEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Translucency [Begin]", OpenGL, AFTER, "Lighting [End]", 0, &RenderSettings::translucencyBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Translucency [End]", OpenGL, AFTER, "Translucency [Begin]", 0, &RenderSettings::translucencyEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Effects (HDR) [Begin]", OpenGL, AFTER, "Translucency [End]", 0, &RenderSettings::hdrEffectsBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Effects (HDR) [End]", OpenGL, AFTER, "Effects (HDR) [Begin]", 0, &RenderSettings::hdrEffectsEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Tone Map [Begin]", OpenGL, AFTER, "Effects (HDR) [End]", 0, &RenderSettings::toneMapBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Tone Map [End]", OpenGL, AFTER, "Tone Map [Begin]", 0, &RenderSettings::toneMapEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Effects (LDR) [Begin]", OpenGL, AFTER, "Tone Map [End]", 0, &RenderSettings::ldrEffectsBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Effects (LDR) [End]", OpenGL, AFTER, "Effects (LDR) [Begin]", 0, &RenderSettings::ldrEffectsEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Present [Begin]", OpenGL, AFTER, "Effects (LDR) [End]", 0, &RenderSettings::presentBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Present [End]", OpenGL, AFTER, "Present [Begin]", 0, &RenderSettings::presentEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "GUI [Begin]", OpenGL, AFTER, "Present [End]", 0, &RenderSettings::guiBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "GUI [End]", OpenGL, AFTER, "GUI [Begin]", 0, &RenderSettings::guiEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Swap [Begin]", OpenGL, AFTER, "GUI [End]", 0, &RenderSettings::swapBeginOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Swap [End]", OpenGL, AFTER, "Swap [Begin]", 0, &RenderSettings::swapEndOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Init Frame", OpenGL, AFTER, "Uniforms [Begin]", 1, &RenderSettings::initFrameOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Render Uniforms", OpenGL, AFTER, "Init Frame", 1, &RenderSettings::uploadUniformsOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "MSAA Resolve", OpenGL, AFTER, "GBuffer Basepass [End]", 1, &RenderSettings::resolveMsaaOpenGL, &RenderSettings::resolveMsaaTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "GBuffer Blit", OpenGL, AFTER, "Present [Begin]", 1, &RenderSettings::blitFramebufferOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(RENDER_SETTINGS, "Framebuffer Swap", OpenGL, AFTER, "Swap [Begin]", 1, &RenderSettings::swapGBuffersOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	void initGbuffer(Scene::Scene& scene, Scene::Object* object)
	{
		// Max supported resolution
		auto maxResolution = GPU::maxResolution();

		GLenum gbufferDataFormat = gbufferGlTextureFormat(scene, object);

		// Create the OpenGL gbuffer.
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "OpenGL");

			createGBuffer(scene, GPU::numLayers(), maxResolution.x, maxResolution.y, GL_DEPTH32F_STENCIL8, gbufferDataFormat, object->component<RenderSettingsComponent>().m_buffers.m_msaa);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void initVoxelGrid(Scene::Scene& scene, Scene::Object* object)
	{
		// Max supported resolution
		auto voxelGridSize = object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels;
		bool anisotropic = object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_anisotropicVoxels;
		GLenum gbufferDataFormat = voxelGbufferGlTextureFormat(scene, object);
		GLenum radianceDataFormat = voxelRadianceGlTextureFormat(scene, object);

		// Create the OpenGL gbuffer.
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "OpenGL");

			Scene::createVoxelGrid(scene, voxelGridSize, voxelGridSize, voxelGridSize, gbufferDataFormat, radianceDataFormat, anisotropic);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* object)
	{
		// Load some of the common utility shaders
		// -  Buffer copying
		Asset::loadShader(scene, "Misc", "copy_buffer");
		Asset::loadShader(scene, "Misc", "copy_layered_buffer");
		// -  Texture blitting
		Asset::loadShader(scene, "Misc", "blit_texture");
		// -  OIT (order-independent transparency) resolve
		//Asset::loadShader(scene, "Misc", "oit_resolve");
		// -  Gaussian blur
		for (int i = 1; i <= 31; ++i) loadGaussianBlurShader(scene, i);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "Render", GL_SHADER_STORAGE_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_RENDER);
		Scene::createGPUBuffer(scene, "Camera", GL_SHADER_STORAGE_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_CAMERA);
		Scene::createGPUBuffer(scene, "Model", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
		Scene::createGPUBuffer(scene, "Material", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_2);
		Scene::createGPUBuffer(scene, "DeferredBasepass", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_3);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initTextures(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Asset::loadTexture(scene, "default_diffuse_map", "Textures/white255.png");
		Asset::loadTexture(scene, "default_specular_map", "Textures/white255.png");
		Asset::loadTexture(scene, "default_normal_map", "Textures/default_normal_map.png");
		Asset::loadTexture(scene, "default_alpha_map", "Textures/white255.png");
		Asset::loadTexture(scene, "default_displacement_map", "Textures/black255.png");
	}

	////////////////////////////////////////////////////////////////////////////////
	void initMeshes(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		if (scene.m_meshes.find("plane.obj") == scene.m_meshes.end())
			Asset::loadMesh(scene, "plane.obj");
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateMsaaState(Scene::Scene& scene, Scene::Object* object)
	{
		if (isUsingMsaaBuffers(scene, object))
			glEnable(GL_MULTISAMPLE);
		else
			glEnable(GL_MULTISAMPLE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Append the G-buffer initializer
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Mesh, initMeshes, "Meshes");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initTextures, "Textures");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, initGPUBuffers, "Generic GPU Buffers");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GBuffer, initGbuffer, "GBuffer");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GBuffer, initVoxelGrid, "Voxel Grid");

		// List of all the resolutions
		auto& allResolutions = RenderSettings::s_allResolutions;
		auto& resolutions = object.component<RenderSettings::RenderSettingsComponent>().m_resolutions;
		auto& resolutionNames = object.component<RenderSettings::RenderSettingsComponent>().m_resolutionNames;
		auto& resolution = object.component<RenderSettings::RenderSettingsComponent>().m_resolution;
		auto& resolutionId = object.component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId;

		// Extract the current resolution
		resolution = GPU::defaultResolution();

		// Extract all the possible resolutions
		auto const& maxResolutionId = std::distance(allResolutions.begin(), std::find(
			allResolutions.begin(), allResolutions.end(), GPU::maxResolution()));
		resolutions = std::vector<glm::ivec2>(allResolutions.begin() + maxResolutionId, allResolutions.end());

		// Compute the ID of the current resolution
		resolutionId = std::distance(resolutions.begin(), std::find(resolutions.begin(), resolutions.end(), resolution));

		// Generate and store the display names of the available resolutions
		resolutionNames.resize(resolutions.size());
		std::transform(resolutions.begin(), resolutions.end(), resolutionNames.begin(),
			[](glm::ivec2 resolution) { return std::to_string(resolution.x) + "x" + std::to_string(resolution.y); });

		// Update the MSAA state
		updateMsaaState(scene, &object);

		// Update the main camera
		DelayedJobs::postJob(scene, &object, "Init Main Camera", [](Scene::Scene& scene, Scene::Object& object)
		{
			updateMainCamera(scene, &object);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

		// Update the main camera if it's empty
		updateMainCamera(scene, object);

		// Store the width and height of the window
		glfwGetWindowSize(scene.m_context.m_window, &object->component<RenderSettings::RenderSettingsComponent>().m_windowSize.x, &object->component<RenderSettings::RenderSettingsComponent>().m_windowSize.y);

		// Compute the scene's enclosing AABB
		BVH::AABB aabb = BVH::AABB(glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX));
		for (auto meshObject: Scene::filterObjects(scene, Scene::OBJECT_TYPE_MESH, true, false))
		{
			if (!Mesh::isMeshValid(scene, meshObject)) continue;
			auto const& mesh = scene.m_meshes[meshObject->component<Mesh::MeshComponent>().m_meshName];
			auto meshAABB = mesh.m_aabb.transform(Transform::getModelMatrix(meshObject));
			aabb = aabb.extend(meshAABB);
		}
		object->component<RenderSettings::RenderSettingsComponent>().m_sceneAabb = aabb;

		// Determine whether we actually need the voxel grid for anything or not
		object->component<RenderSettings::RenderSettingsComponent>().m_needsVoxelGrid = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_VOXEL_GLOBAL_ILLUMINATION) != nullptr;

		// Update the voxel grid extents
		const glm::vec3 sceneExtents = aabb.getSize();
		object->component<RenderSettings::RenderSettingsComponent>().m_voxelGridExtents = glm::max(sceneExtents.x, glm::max(sceneExtents.y, sceneExtents.z));
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateWindowSize(Scene::Scene& scene, Scene::Object* object)
	{
		glm::ivec2 resolution = object->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		if (object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_fullscreen)
		{
			glfwSetWindowMonitor(scene.m_context.m_window, glfwGetPrimaryMonitor(), 0, 0, resolution.x, resolution.y, 60);
		}
		else
		{
			int left, top, right, bottom;
			glfwGetWindowFrameSize(scene.m_context.m_window, &left, &top, &right, &bottom);
			glfwSetWindowMonitor(scene.m_context.m_window, nullptr, left, top, resolution.x, resolution.y, 60);
			glfwMaximizeWindow(scene.m_context.m_window);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		bool resolutionChanged = false;
		bool lightingChanged = false;
		bool voxelsChanged = false;
		bool msaaChanged = false;

		if (ImGui::BeginTabBar(object->m_name.c_str()) == false)
			return;

		// Restore the selected tab id
		std::string activeTab;
		if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, object, "MainTabBar_SelectedTab#Synced"); activeTabSynced.has_value())
			activeTab = activeTabSynced.value();

		if (ImGui::BeginTabItem("Render", activeTab.c_str()))
		{
			ImGui::Combo("Renderer", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_renderer, RenderSettings::Renderer_meta);
			std::vector<Scene::Object*> cameras = Scene::filterObjects(scene, Scene::OBJECT_TYPE_CAMERA, true, true, true);
			if (cameras.size() > 0)
			{
				std::vector<std::string> cameraNames(cameras.size());
				std::transform(cameras.begin(), cameras.end(), cameraNames.begin(), [](Scene::Object* camera) { return camera->m_name; });
				std::string currentCamera = object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera->m_name;
				ImGui::Combo("Camera", currentCamera, cameraNames);
				object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera = &scene.m_objects[currentCamera];
				object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera->m_enabled = true;
			}

			if (ImGui::Combo("Render Size", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId, object->component<RenderSettings::RenderSettingsComponent>().m_resolutionNames))
			{
				resolutionChanged = true;
				object->component<RenderSettings::RenderSettingsComponent>().m_resolution = object->component<RenderSettings::RenderSettingsComponent>().m_resolutions[object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId];
				if (object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_fullscreen) updateWindowSize(scene, object);
			}
			if (ImGui::Checkbox("Fullscreen", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_fullscreen))
			{
				resolutionChanged = true;
				updateWindowSize(scene, object);
			}
			if (object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_fullscreen == false)
			{
				ImGui::SameLine();
				if (ImGui::Button("Fit Window"))
				{
					updateWindowSize(scene, object);
				}
			}

			ImGui::Combo("Blit Method", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitMode, RenderSettings::BlitMode_meta);
			if (object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitMode == RenderSettings::Scaled)
			{
				ImGui::Combo("Blit Anchor", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitAnchor, RenderSettings::BlitAnchor_meta);
				ImGui::SliderFloat("Blit Scale Factor", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitScale, 0.0f, 8.0f);
			}
			if (object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitMode == RenderSettings::Percentage)
			{
				ImGui::Combo("Blit Anchor", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitAnchor, RenderSettings::BlitAnchor_meta);
				ImGui::SliderFloat2("Window Percentage", glm::value_ptr(object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitPercentage), 0.0f, 1.0f);
			}
			ImGui::Combo("Blit Filtering", &object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitFiltering, RenderSettings::TextureFiltering_meta);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Buffers", activeTab.c_str()))
		{
			voxelsChanged |= ImGui::Combo("GBuffer Data Format", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_gbufferDataFormat, RenderSettings::TextureDataFormat_meta);
			voxelsChanged |= ImGui::Combo("Voxel GBuffer Data Format", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelGbufferDataFormat, RenderSettings::TextureDataFormat_meta);
			voxelsChanged |= ImGui::Combo("Voxel Radiance Data Format", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelRadianceDataFormat, RenderSettings::TextureDataFormat_meta);
			ImGui::SliderInt("Voxel Grid Size", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels, 8, 512); voxelsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			voxelsChanged |= ImGui::Checkbox("Anisotropic Voxel Grid", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_anisotropicVoxels);

			ImGui::SliderInt("MSAA Samples", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_msaa, 1, 16); msaaChanged |= ImGui::IsItemDeactivatedAfterEdit();
			msaaChanged |= ImGui::Checkbox("Force MSAA Buffers", &object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_forceMsaaBuffers);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Layers", activeTab.c_str()))
		{
			ImGui::SliderInt("Depth Layers", &object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers, 1, GPU::numLayers());
			ImGui::Combo("Depth Peeling Method", &object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_depthPeelAlgorithm, RenderSettings::DepthPeelAlgorithm_meta);
			if (object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_depthPeelAlgorithm == RenderSettings::MinimumSeparation)
			{
				ImGui::DragFloat("Depth Gap (m)", &object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_minimumSeparationDepthGap, 0.01f);
			}
			if (object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_depthPeelAlgorithm == RenderSettings::UmbraThresholding)
			{
				ImGui::SliderFloat("Umbra Scale Factor", &object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraScaleFactor, 0.0f, 4.0f);
				ImGui::SliderFloat("Umbra Min", &object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraMin, 0.0f, 1.0f);
				ImGui::SliderFloat("Umbra Max", &object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraMax, 0.0f, 10000.0f);
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Units", activeTab.c_str()))
		{
			ImGui::DragFloat("Units Per Meter", &object->component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter, 1.0f);
			ImGui::DragFloat("Nits Per Unit", &object->component<RenderSettings::RenderSettingsComponent>().m_units.m_nitsPerUnit, 1.0f, 0.0f, 1000.0f, "%.3f");

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Lighting", activeTab.c_str()))
		{
			ImGui::Combo("Shading Mode", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_shadingMode, RenderSettings::ShadingMethod_meta);
			lightingChanged |= ImGui::Combo("Lighting Mode", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingMode, RenderSettings::LightingMode_meta);
			lightingChanged |= ImGui::Combo("BRDF Model", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_brdfModel, RenderSettings::BrdfModel_meta);
			lightingChanged |= ImGui::Combo("Lighting Texture Filtering", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingFiltering, RenderSettings::TextureFiltering_meta);
			lightingChanged |= ImGui::Combo("Voxel Dilation Mode", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_voxelDilationMode, RenderSettings::VoxelDilationMethod_meta);
			lightingChanged |= ImGui::Combo("Voxel Shading Mode", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_voxelShadingMode, RenderSettings::VoxelShadingMode_meta);
			lightingChanged |= ImGui::SliderFloat("Min Specular Power", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_specularPowerMin, 2.0f, 4096.0f);
			lightingChanged |= ImGui::SliderFloat("Max Specular Power", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_specularPowerMax, 2.0f, 4096.0f);
			ImGui::SliderFloat("Gamma", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_gamma, 0.0f, 8.0f);
			ImGui::Checkbox("AO Affects Direct Light", &object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_aoAffectsDirectLighting);
			lightingChanged |= ImGui::Button("Update Lighting");

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Features", activeTab.c_str()))
		{
			lightingChanged |= ImGui::Combo("Backface Culling", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_backfaceCull, RenderSettings::BackfaceCullMode_meta);
			lightingChanged |= ImGui::Combo("Transparency", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_transparencyMethod, RenderSettings::TransparencyMethod_meta);
			lightingChanged |= ImGui::Combo("Normal Mapping", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_normalMapping, RenderSettings::NormalMappingMethod_meta);
			lightingChanged |= ImGui::Combo("Displacement Mapping", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_displacementMapping, RenderSettings::DisplacementMappingMethod_meta);
			lightingChanged |= ImGui::Combo("Shadows", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_shadowMethod, RenderSettings::ShadowMethod_meta);

			ImGui::Checkbox("Depth Pre-pass", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_depthPrepass);
			ImGui::Checkbox("Wireframe Mesh", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_wireframeMesh);
			ImGui::Checkbox("Show Aperture Size", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_showApertureSize);
			ImGui::Checkbox("Background Rendering", &object->component<RenderSettings::RenderSettingsComponent>().m_features.m_backgroundRendering);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// End the tab bar
		ImGui::EndTabBar();

		if (msaaChanged || voxelsChanged)
		{
			DelayedJobs::postJob(scene, object, "Recreate GBuffer", [](Scene::Scene& scene, Scene::Object& object)
			{
				// Recreate the GBuffer
				Scene::reloadResources(scene, Scene::GBuffer);

				// Reload all the shaders
				Scene::reloadResources(scene, Scene::Shader);

				// Update the voxel grid
				updateVoxelGrid(scene, &object);

				// Update the MSAA state
				updateMsaaState(scene, &object);
			});
		}

		// Update the voxel grid
		if (lightingChanged)
		{
			updateShadowMaps(scene, object);
			updateVoxelGrid(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string renderPayloadCategory(std::initializer_list<const char*> categories)
	{
		return std::string_join("::", categories.begin(), categories.end());
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isCurrentCameraValid(Scene::Scene& scene, Scene::Object* object)
	{
		return object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera != nullptr &&
			SimulationSettings::isObjectEnabled(scene, object->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateMainCamera(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		// Set the main camera to the default camera if the current one is empty
		if (!isCurrentCameraValid(scene, renderSettings))
		{
			const std::string mainCameraName = Config::AttribValue("camera").get<std::string>();
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera = Scene::findObject(scene, mainCameraName);
		}

		// Look for another camera if the current one cannot be used
		if (!isCurrentCameraValid(scene, renderSettings))
		{
			auto const& availableCameras = Scene::filterObjects(scene, Scene::OBJECT_TYPE_CAMERA, true, false, true);
			if (availableCameras.empty())
			{
				Debug::log_error() << "No camera available for rendering." << Debug::end;
			}
			else
			{
				Scene::Object* newCamera = availableCameras[0];
				for (auto camera : availableCameras)
				{
					if (camera->component<Camera::CameraComponent>().m_locked == false && camera->component<Camera::CameraComponent>().m_settingsLocked == 0)
					{
						newCamera = camera;
						break;
					}
				}
				renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera = newCamera;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateMainCamera(Scene::Scene& scene)
	{
		updateMainCamera(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* getMainCamera(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		updateMainCamera(scene, renderSettings);
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_mainCamera;
	}

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* getMainCamera(Scene::Scene& scene)
	{
		return getMainCamera(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void bindShaderMsaaVariant(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& folderName, std::string const& shaderName)
	{
		// Bind the proper shader
		if (RenderSettings::isUsingMsaaBuffers(scene, renderSettings))
			Scene::bindShader(scene, folderName, shaderName + "_msaa");
		else
			Scene::bindShader(scene, folderName, shaderName + "_no_msaa");
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isUsingMsaaBuffers(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_msaa >= 2 ||
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_forceMsaaBuffers;
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadShaderMsaaVariants(Scene::Scene& scene, const std::string& folderName, const std::string& fileName,
		const std::string& customShaderName, const Asset::ShaderParameters& shaderParameters)
	{
		// No MSAA version
		std::string shaderNameNoMsaa = customShaderName + "_no_msaa";
		auto definesNoMsaa = shaderParameters;
		definesNoMsaa.m_defines.push_back("MSAA 0");

		// MSAA version
		std::string shaderNameMsaa = customShaderName + "_msaa";
		auto definesMsaa = shaderParameters;
		definesMsaa.m_defines.push_back("MSAA 1");

		// Load the shader variants
		Asset::loadShader(scene, folderName, fileName, shaderNameNoMsaa, definesNoMsaa);
		Asset::loadShader(scene, folderName, fileName, shaderNameMsaa, definesMsaa);
	}

	////////////////////////////////////////////////////////////////////////////////
	int getGaussianBlurTaps(const float sigma)
	{
		return 2 * int(2 * sigma) + 3;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGaussianKernel(Scene::Scene& scene, const float sigma, std::vector<glm::vec2>& discrete, std::vector<glm::vec2>& linear)
	{
		// Compte the number of taps needed
		int numTapsDiscrete = getGaussianBlurTaps(sigma);

		// Generate the discrete kernel
		const int kernelRadiusDiscrete = numTapsDiscrete / 2;
		discrete.resize(numTapsDiscrete);
		float discreteSum = 0.0f;
		for (int i = 0; i < numTapsDiscrete; ++i)
		{
			auto& tap = discrete[i];
			const float x = i - kernelRadiusDiscrete;
			const float t = -(x * x) / (2.0f * sigma * sigma);
			tap.x = glm::exp(t);
			tap.y = x;
			discreteSum += tap.x;
		}

		// Normalize the kernel
		for (int i = 0; i < numTapsDiscrete; ++i)
			discrete[i].x /= discreteSum;

		Debug::log_debug() << "Gaussian kernel for " << numTapsDiscrete << "x" << numTapsDiscrete << " Gaussian blur (discrete): " << discrete << Debug::end;

		// Generate the linear kernel
		const int numTapsLinear = int(glm::ceil(float(numTapsDiscrete) * 0.5f));
		const int kernelRadiusLinear = numTapsLinear / 2;
		linear.resize(numTapsLinear);
		float linearSum = 0.0f;

		std::vector<glm::vec2> weightsOneSide;
		if (numTapsLinear % 2 == 0)
		{
			weightsOneSide.resize(kernelRadiusDiscrete + 1);
			std::copy_n(discrete.begin(), weightsOneSide.size(), weightsOneSide.begin());
			weightsOneSide.back().x *= 0.5f;
		}
		else
		{
			weightsOneSide.resize(kernelRadiusDiscrete);
			std::copy_n(discrete.begin(), weightsOneSide.size(), weightsOneSide.begin());
			linear[kernelRadiusLinear] = discrete[kernelRadiusDiscrete];
			linearSum += discrete[kernelRadiusDiscrete].x;
		}

		for (size_t i = 0; i < weightsOneSide.size() / 2; ++i)
		{
			int t1 = i * 2 + 0;
			int t2 = i * 2 + 1;
			int neg = i;
			int pos = numTapsLinear - i - 1;
			linear[neg].x = linear[pos].x = weightsOneSide[t1].x + weightsOneSide[t2].x;
			linear[neg].y = (weightsOneSide[t1].x * weightsOneSide[t1].y + weightsOneSide[t2].x * weightsOneSide[t2].y) / linear[neg].x;
			linear[pos].y = -linear[neg].y;
			linearSum += linear[neg].x + linear[pos].x;
		}
		// Normalize the kernel
		for (int i = 0; i < numTapsLinear; ++i)
			linear[i].x /= linearSum;

		Debug::log_debug() << "Gaussian kernel for " << numTapsDiscrete << "x" << numTapsDiscrete << " Gaussian blur (linear): " << linear << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadGaussianBlurShader(Scene::Scene& scene, int numTaps)
	{

		// Linear-space Gaussian blur, with linearly sampled kernel weights
		Asset::ShaderParameters shaderParametersLinear;
		shaderParametersLinear.m_defines =
		{
			"NUM_SAMPLES " + std::to_string(numTaps),
			"FILTER_SPACE linear"
		};
		Asset::loadShader(scene, "Misc", "gaussian_blur", "Misc/gaussian_blur_" + std::to_string(numTaps), shaderParametersLinear);

		// Log-space Gaussian blur, with discretely sampled kernel weights
		if (numTaps % 2 == 1)
		{
			Asset::ShaderParameters shaderParametersLogarithmic;
			shaderParametersLogarithmic.m_defines =
			{
				"NUM_SAMPLES " + std::to_string(numTaps),
				"FILTER_SPACE logarithmic"
			};
			Asset::loadShader(scene, "Misc", "gaussian_blur", "Misc/gaussian_blur_log_" + std::to_string(numTaps), shaderParametersLogarithmic);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t convertGbufferId(Scene::Object* renderSettings, GBufferId id)
	{
		return (id == RenderSettings::GB_ReadBuffer) ?
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferRead :
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t convertSubBufferId(GPU::GBuffer const& gbuffer, GBufferId id)
	{
		return (id == RenderSettings::GB_ReadBuffer) ?
			gbuffer.m_readBuffer :
			gbuffer.m_writeBuffer;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer. */
	void bindGbufferOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId, GLenum target)
	{
		size_t gbufId = convertGbufferId(renderSettings, gbufferId);
		if (isUsingMsaaBuffers(scene, renderSettings))
			glBindFramebuffer(target, scene.m_gbuffer[gbufId].m_gbufferMsaa);
		else
			glBindFramebuffer(target, scene.m_gbuffer[gbufId].m_gbuffer);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer layer. */
	void bindGbufferLayersOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId, GBufferId bufferId, GLenum target)
	{
		size_t gbufId = convertGbufferId(renderSettings, gbufferId);
		size_t bufId = convertSubBufferId(scene.m_gbuffer[gbufId], bufferId);
		glBindFramebuffer(target, scene.m_gbuffer[gbufId].m_colorBuffersLayered[bufId]);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer layer. */
	void bindGbufferLayerOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId, GBufferId bufferId, size_t layerId, GLenum target)
	{
		size_t gbufId = convertGbufferId(renderSettings, gbufferId);
		size_t bufId = convertSubBufferId(scene.m_gbuffer[gbufId], bufferId);
		glBindFramebuffer(target, scene.m_gbuffer[gbufId].m_colorBuffersPerLayer[bufId][layerId]);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer. */
	void bindGbufferLayersImageOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId, GBufferId bufferId, GLuint target)
	{
		size_t gbufId = convertGbufferId(renderSettings, gbufferId);
		size_t bufId = convertSubBufferId(scene.m_gbuffer[gbufId], bufferId);
			(target, scene.m_gbuffer[gbufId].m_colorTextures[bufId], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer. */
	void bindGbufferLayerImageOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId, GBufferId bufferId, size_t layerId, GLuint target)
	{
		size_t gbufId = convertGbufferId(renderSettings, gbufferId);
		size_t bufId = convertSubBufferId(scene.m_gbuffer[gbufId], bufferId);
		glBindImageTexture(target, scene.m_gbuffer[gbufId].m_colorTextures[bufId], 0, GL_FALSE, layerId, GL_WRITE_ONLY, GL_RGBA16F);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding GBuffer layer as a texture. */
	void bindGbufferTextureLayersOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId, GBufferId bufferId, bool respectMsaa)
	{
		size_t gbufId = convertGbufferId(renderSettings, gbufferId);
		size_t bufId = convertSubBufferId(scene.m_gbuffer[gbufId], bufferId);

		if (respectMsaa && RenderSettings::isUsingMsaaBuffers(scene, renderSettings))
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_DEPTH_ENUM);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, scene.m_gbuffer[gbufId].m_depthTextureMsaa);
			glActiveTexture(GPU::TextureEnums::TEXTURE_ALBEDO_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, scene.m_gbuffer[gbufId].m_colorTextureMsaa);
			glActiveTexture(GPU::TextureEnums::TEXTURE_NORMAL_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, scene.m_gbuffer[gbufId].m_normalTextureMsaa);
			glActiveTexture(GPU::TextureEnums::TEXTURE_SPECULAR_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, scene.m_gbuffer[gbufId].m_specularTextureMsaa);
		}
		else
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_DEPTH_ENUM);
			glBindTexture(GL_TEXTURE_2D_ARRAY, scene.m_gbuffer[gbufId].m_depthTexture);
			glActiveTexture(GPU::TextureEnums::TEXTURE_ALBEDO_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D_ARRAY, scene.m_gbuffer[gbufId].m_colorTextures[bufId]);
			glActiveTexture(GPU::TextureEnums::TEXTURE_NORMAL_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D_ARRAY, scene.m_gbuffer[gbufId].m_normalTexture);
			glActiveTexture(GPU::TextureEnums::TEXTURE_SPECULAR_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D_ARRAY, scene.m_gbuffer[gbufId].m_specularTexture);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void swapGbuffers(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings)
	{
		// Swap read and write buffers
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferRead = 1 - renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferRead;
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite = 1 - renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite;
	}

	////////////////////////////////////////////////////////////////////////////////
	void swapGbufferBuffers(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings)
	{
		auto& gbuffer = scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite];

		Debug::log_trace() << "Swapping read buffer from " << gbuffer.m_readBuffer << " to " << (1 - gbuffer.m_readBuffer) << Debug::end;
		Debug::log_trace() << "Swapping write buffer from " << gbuffer.m_writeBuffer << " to " << (1 - gbuffer.m_writeBuffer) << Debug::end;

		gbuffer.m_readBuffer = 1 - gbuffer.m_readBuffer;
		gbuffer.m_writeBuffer = 1 - gbuffer.m_writeBuffer;
	}

	////////////////////////////////////////////////////////////////////////////////
	void setupViewportOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, int resolutionId)
	{
		glm::ivec2 resolution = resolutionId < 0 ?
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution :
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolutions[resolutionId];

		glViewport(0, 0, resolution.x, resolution.y);
	}

	////////////////////////////////////////////////////////////////////////////////
	void setupViewportArrayOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, int resolutionId)
	{
		glm::ivec2 resolution = resolutionId < 0 ?
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution :
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolutions[resolutionId];

		if (GPU::enableOptionalExtensions() && glewGetExtension("GL_NV_viewport_array2"))
		{
			for (size_t i = 0; i < renderSettings->component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers; ++i)
			{
				glViewportIndexedf(i, 0, 0, resolution.x, resolution.y);
			}
		}
		else
		{
			glViewport(0, 0, resolution.x, resolution.y);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderFullscreenPlaneOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings)
	{
		glBindVertexArray(scene.m_meshes["plane.obj"].m_vao);
		glDrawElements(GL_TRIANGLES, scene.m_meshes["plane.obj"].m_indexCount, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	////////////////////////////////////////////////////////////////////////////////
	float unitsToMeters(Scene::Object* renderSettings, float units)
	{
		return units / renderSettings->component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter;
	}

	////////////////////////////////////////////////////////////////////////////////
	float unitsToMeters(Scene::Scene& scene, Scene::Object* renderSettings, float units)
	{
		return unitsToMeters(renderSettings, units);
	}

	////////////////////////////////////////////////////////////////////////////////
	float unitsToMeters(Scene::Scene& scene, float units)
	{
		return unitsToMeters(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS), units);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 unitsToMeters(Scene::Object* renderSettings, glm::vec3 units)
	{
		return units / renderSettings->component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 unitsToMeters(Scene::Scene& scene, Scene::Object* renderSettings, glm::vec3 units)
	{
		return unitsToMeters(renderSettings, units);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 unitsToMeters(Scene::Scene& scene, glm::vec3 units)
	{
		return unitsToMeters(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS), units);
	}

	////////////////////////////////////////////////////////////////////////////////
	float metersToUnits(Scene::Object* renderSettings, float meters)
	{
		return meters * renderSettings->component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter;
	}

	////////////////////////////////////////////////////////////////////////////////
	float metersToUnits(Scene::Scene&, Scene::Object* renderSettings, float meters)
	{
		return metersToUnits(renderSettings, meters);
	}

	////////////////////////////////////////////////////////////////////////////////
	float metersToUnits(Scene::Scene& scene, float meters)
	{
		return metersToUnits(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS), meters);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 metersToUnits(Scene::Object* renderSettings, glm::vec3 meters)
	{
		return meters * renderSettings->component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 metersToUnits(Scene::Scene&, Scene::Object* renderSettings, glm::vec3 meters)
	{
		return metersToUnits(renderSettings, meters);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 metersToUnits(Scene::Scene& scene, glm::vec3 meters)
	{
		return metersToUnits(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS), meters);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolutionById(Scene::Scene& scene, Scene::Object* renderSettings, int resolutionId)
	{
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolutions[resolutionId];
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolutionById(Scene::Scene& scene, int resolutionId)
	{
		return getResolutionById(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS), resolutionId);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolution(Scene::Scene& scene, Scene::Object* renderSettings)
	{

		return getResolutionById(scene, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolution(Scene::Scene& scene)
	{
		return getResolution(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum glTextureFormat(TextureDataFormat format)
	{
		switch (format)
		{
		case UI8: return GL_RGBA8;
		case F8: return GL_RGBA8_SNORM;
		case F16: return GL_RGBA16F;
		case F32: return GL_RGBA32F;
		}
		return GL_RGBA8;
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum gbufferGlTextureFormat(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return glTextureFormat(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_gbufferDataFormat);
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum gbufferGlTextureFormat(Scene::Scene& scene)
	{
		return gbufferGlTextureFormat(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelGbufferGlTextureFormat(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return glTextureFormat(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelGbufferDataFormat);
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelGbufferGlTextureFormat(Scene::Scene& scene)
	{
		return voxelGbufferGlTextureFormat(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelRadianceGlTextureFormat(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return glTextureFormat(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelRadianceDataFormat);
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelRadianceGlTextureFormat(Scene::Scene& scene)
	{
		return voxelRadianceGlTextureFormat(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string glShaderFormat(TextureDataFormat format)
	{
		switch (format)
		{
		case UI8: return "rgba8";
		case F8: return "rgba8_snorm";
		case F16: return "rgba16f";
		case F32: return "rgba32f";
		}
		return "";
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string gbufferGlShaderFormat(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return glShaderFormat(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_gbufferDataFormat);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string gbufferGlShaderFormat(Scene::Scene& scene)
	{
		return voxelGbufferGlShaderFormat(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelGbufferGlShaderFormat(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return glShaderFormat(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelGbufferDataFormat);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelGbufferGlShaderFormat(Scene::Scene& scene)
	{
		return voxelGbufferGlShaderFormat(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelRadianceGlShaderFormat(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return glShaderFormat(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_voxelRadianceDataFormat);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelRadianceGlShaderFormat(Scene::Scene& scene)
	{
		return voxelRadianceGlShaderFormat(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMaps(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		// Regenerate all the shadow maps
		for (auto object : ShadowMap::getShadowCasters(scene))
			ShadowMap::regenerateShadowMap(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMaps(Scene::Scene& scene)
	{
		return updateShadowMaps(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGrid(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelGbuffer = true;
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelRadiance = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGrid(Scene::Scene& scene)
	{
		return updateVoxelGrid(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGridRadiance(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelRadiance = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGridRadiance(Scene::Scene& scene)
	{
		return updateVoxelGridRadiance(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void initFrameOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Clean the list of payloads
		auto oldPayloads = renderSettings->component<RenderSettingsComponent>().m_payloads;
		renderSettings->component<RenderSettingsComponent>().m_payloads.clear();

		// Copy over the persistent ones
		for (auto const& payload: oldPayloads)
		{
			if (payload.second.m_persistent)
			{
				renderSettings->component<RenderSettingsComponent>().m_payloads[payload.first] = payload.second;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void uploadUniformsOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Upload the render uniforms
		RenderSettings::UniformData renderData;
		renderData.m_resolution = glm::vec2(object->component<RenderSettings::RenderSettingsComponent>().m_resolution);
		renderData.m_maxResolution = glm::vec2(scene.m_gbuffer[0].m_dimensions);
		renderData.m_uvScale = renderData.m_resolution / renderData.m_maxResolution;
		renderData.m_worldMin = object->component<RenderSettings::RenderSettingsComponent>().m_sceneAabb.m_min;
		renderData.m_worldMax = object->component<RenderSettings::RenderSettingsComponent>().m_sceneAabb.m_max;
		renderData.m_depthPeelAlgorithm = object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_depthPeelAlgorithm;
		renderData.m_brdfModel = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_brdfModel;
		renderData.m_lightingTextureFiltering = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingFiltering;
		renderData.m_voxelShadingMode = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_voxelShadingMode;
		renderData.m_voxelDilationMode = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_voxelDilationMode;
		renderData.m_deltaTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime;
		renderData.m_globalTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime;
		renderData.m_scaledDeltaTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledDeltaTime;
		renderData.m_scaledGlobalTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledGlobalTime;
		renderData.m_metersPerUnit = 1.0f / object->component<RenderSettings::RenderSettingsComponent>().m_units.m_unitsPerMeter;
		renderData.m_nitsPerUnit = object->component<RenderSettings::RenderSettingsComponent>().m_units.m_nitsPerUnit;
		renderData.m_msaa = object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_msaa;
		renderData.m_numLayers = object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers;
		renderData.m_minimumSeparationDepthGap = object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_minimumSeparationDepthGap;
		renderData.m_umbraScaleFactor = object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraScaleFactor;
		renderData.m_umbraMin = object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraMin;
		renderData.m_umbraMax = object->component<RenderSettings::RenderSettingsComponent>().m_layers.m_umbraMax;
		renderData.m_specularPowerMin = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_specularPowerMin;
		renderData.m_specularPowerMax = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_specularPowerMax;
		renderData.m_gamma = object->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_gamma;
		renderData.m_numVoxels = object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels;
		renderData.m_voxelExtents = object->component<RenderSettings::RenderSettingsComponent>().m_voxelGridExtents;
		renderData.m_voxelSize = object->component<RenderSettings::RenderSettingsComponent>().m_voxelGridExtents / object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels;
		uploadBufferData(scene, "Render", renderData);
	}

	////////////////////////////////////////////////////////////////////////////////
	void uniformsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void uniformsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{}

	////////////////////////////////////////////////////////////////////////////////
	bool depthPrePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName) &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_depthPrepass;
	}

	////////////////////////////////////////////////////////////////////////////////
	void depthPrePassBeginOpenGL(Scene::Scene & scene, Scene::Object * simulationSettings, Scene::Object * renderSettings, Scene::Object * camera, std::string const& functionName, Scene::Object * object)
	{
		// Set the write buffer id
		scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite].m_writeBuffer = 0;
		scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite].m_readBuffer = 1;

		// Set the OGL state
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);

		// Bind the framebuffer
		bindGbufferOpenGL(scene, simulationSettings, renderSettings);

		// Clear the framebuffer
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);

		// Bind the previous frame's depth buffer
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings, GB_ReadBuffer);

		// Disable color writing
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void depthPrePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Release the gbuffer so that we can bind the depth texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Re-enable color writing
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Set the write buffer id
		scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite].m_writeBuffer = 0;
		scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite].m_readBuffer = 1;

		// Set the OGL state
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		if (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_depthPrepass)
			glDepthFunc(GL_LEQUAL);
		else
			glDepthFunc(GL_LESS);

		// Bind the framebuffer
		RenderSettings::bindGbufferOpenGL(scene, simulationSettings, renderSettings);
		RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);

		// Clear the framebuffer if no depth pre-pass occured prior
		if (!renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_depthPrepass)
		{
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		// Bind the previous frame's depth buffer
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings, GB_ReadBuffer);
	}

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Reset the depth mask
		glDepthMask(GL_TRUE);

		// Release the gbuffer so that we can bind the depth texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// We are writing to the second buffer from now on
		scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite].m_readBuffer = 0;
		scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite].m_writeBuffer = 1;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return 
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_needsVoxelGrid &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelGbuffer &&
			RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Render to the default FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels, object->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		// clear the voxel textures
		static float zeros[] = { 0, 0, 0, 0 };
		glClearTexImage(scene.m_voxelGrid.m_albedoTexture, 0, GL_RGBA, GL_FLOAT, zeros);
		glClearTexImage(scene.m_voxelGrid.m_normalTexture, 0, GL_RGBA, GL_FLOAT, zeros);
		glClearTexImage(scene.m_voxelGrid.m_specularTexture, 0, GL_RGBA, GL_FLOAT, zeros);

		// Bind the resulting textures
		glBindImageTexture(0, scene.m_voxelGrid.m_albedoTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
		glBindImageTexture(1, scene.m_voxelGrid.m_normalTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
		glBindImageTexture(2, scene.m_voxelGrid.m_specularTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

		// Determine the transformation matrices for the voxel projection
		glm::vec3 const& center = object->component<RenderSettings::RenderSettingsComponent>().m_sceneAabb.getCenter();
		const float halfExtent = object->component<RenderSettings::RenderSettingsComponent>().m_voxelGridExtents / 2.0f;

		// Store the resulting matrices
		auto& matrices = object->component<RenderSettings::RenderSettingsComponent>().m_voxelMatrices;
		auto projection = glm::ortho(-halfExtent, halfExtent, -halfExtent, halfExtent, 0.01f, 2.0f * halfExtent);
		matrices[0] = projection * glm::lookAt(center + glm::vec3(halfExtent, 0.0f, 0.0f), center, glm::vec3(0.0f, 1.0f, 0.0f));
		matrices[1] = projection * glm::lookAt(center + glm::vec3(0.0f, halfExtent, 0.0f), center, glm::vec3(0.0f, 0.0f, -1.0f));
		matrices[2] = projection * glm::lookAt(center + glm::vec3(0.0f, 0.0f, halfExtent), center, glm::vec3(0.0f, 1.0f, 0.0f));

		// Store the inverse matrices
		auto& inverseMatrices = object->component<RenderSettings::RenderSettingsComponent>().m_inverseVoxelMatrices;
		inverseMatrices[0] = glm::inverse(matrices[0]);
		inverseMatrices[1] = glm::inverse(matrices[1]);
		inverseMatrices[2] = glm::inverse(matrices[2]);
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// sync barrier
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		// Reset some of the GL state
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// Mark the voxel GBuffer updated
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelGbuffer = false;
	}

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{}

	////////////////////////////////////////////////////////////////////////////////
	bool resolveMsaaTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return isUsingMsaaBuffers(scene, renderSettings) &&
			firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	void resolveMsaaOpenGL(Scene::Scene & scene, Scene::Object * simulationSettings, Scene::Object * renderSettings, Scene::Object * camera, std::string const& functionName, Scene::Object * object)
	{
		// Set the necessary GL state
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_SCISSOR_TEST);

		// Extract the current gbuffer
		auto const& gbuffer = scene.m_gbuffer[renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite];
		glm::vec2 renderResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;

		// Copy over the depth buffer contents
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.m_gbufferMsaa);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.m_gbuffer);
		glBlitFramebuffer(0, 0, renderResolution[0], renderResolution[1], 0, 0, renderResolution[0], renderResolution[1], GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool voxelLightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_needsVoxelGrid &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelRadiance &&
			RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// clear the voxel texture
		static float zeros[] = { 0, 0, 0, 0 };
		glClearTexImage(scene.m_voxelGrid.m_radianceTexture, 0, GL_RGBA, GL_FLOAT, zeros);

		// Bind the voxel textures as images
		glBindImageTexture(0, scene.m_voxelGrid.m_radianceTexture, 0, GL_TRUE, 0, GL_READ_WRITE, scene.m_voxelGrid.m_radianceDataFormat);

		// Bind the voxel images
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
		glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_albedoTexture);
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_2_ENUM);
		glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_normalTexture);
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_3_ENUM);
		glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_specularTexture);
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_4_ENUM);
		glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_radianceTexture);
		for (int i = 0; i < 6 && scene.m_voxelGrid.m_anisotropic; ++i)
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_5_ENUM + i);
			glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_radianceMipmaps[i]);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// sync barrier
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		// Mark the voxel shading updated
		renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelRadiance = false;
	}

	////////////////////////////////////////////////////////////////////////////////
	void lightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Set the OpenGL state
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_NOTEQUAL);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// Bind the write color buffer and clear it
		RenderSettings::bindGbufferLayersOpenGL(scene, simulationSettings, renderSettings);
		RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Bind the gbuffer textures
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings, GB_WriteBuffer, GB_ReadBuffer, true);
	}

	////////////////////////////////////////////////////////////////////////////////
	void lightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);

		// Bind the gbuffer textures
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings);
	}

	////////////////////////////////////////////////////////////////////////////////
	void translucencyBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// TODO: implement
	}

	////////////////////////////////////////////////////////////////////////////////
	void translucencyEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// TODO: implement
	}

	////////////////////////////////////////////////////////////////////////////////
	void hdrEffectsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void hdrEffectsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void toneMapBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void toneMapEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void ldrEffectsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void ldrEffectsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void guiBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void guiEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void presentBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void presentEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void swapBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void swapEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	int& lastCalledVariable(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& functionName, Scene::Object* object = nullptr)
	{
		if (object) return RenderSettings::renderPayload(scene, renderSettings, renderPayloadCategory({ functionName.c_str(), object->m_name.c_str(), "LastCalled" }), true, -1);
		else        return RenderSettings::renderPayload(scene, renderSettings, renderPayloadCategory({ functionName.c_str(), "LastCalled" }), true, -1);
	}

	////////////////////////////////////////////////////////////////////////////////
	int& callCountVariable(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& functionName, Scene::Object* object = nullptr)
	{
		if (object) return RenderSettings::renderPayload(scene, renderSettings, renderPayloadCategory({ functionName.c_str(), object->m_name.c_str(), "CallCount" }), false, 0);
		else        return RenderSettings::renderPayload(scene, renderSettings, renderPayloadCategory({ functionName.c_str(), "CallCount" }), false, 0);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool firstCallCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, std::string const& functionName, Scene::Object* object = nullptr)
	{
		int& lastCalledVar = lastCalledVariable(scene, renderSettings, functionName, object);
		int& numCallsVar = callCountVariable(scene, renderSettings, functionName, object);
		int frameId = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
		if (lastCalledVar == frameId) return false;

		// Update the last called variable and signal that this was the first call
		lastCalledVar = frameId;
		++numCallsVar;
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool firstCallTypeCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return firstCallCondition(scene, simulationSettings, renderSettings, functionName, nullptr);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool firstCallObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return firstCallCondition(scene, simulationSettings, renderSettings, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool multiCallTypeCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		++callCountVariable(scene, renderSettings, functionName, nullptr);
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool multiCallObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		if (callCountVariable(scene, renderSettings, functionName, object) == callCountVariable(scene, renderSettings, functionName, nullptr))
			return false;

		++callCountVariable(scene, renderSettings, functionName, object);
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void blitFramebufferOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Source and destination coords
		glm::ivec2 srcStart, srcEnd;
		glm::ivec2 dstStart, dstEnd;

		glm::vec2 renderResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		glm::vec2 scaledRenderResolution = glm::vec2(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution) * renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitScale;
		glm::vec2 windowSize = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_windowSize;
		glm::vec2 scaledWindowSize = glm::vec2(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_windowSize) * renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitPercentage;

		switch (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitMode)
		{
		case RenderSettings::Scaled:
			switch (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitAnchor)
			{
			case RenderSettings::Center:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2((glm::vec2(windowSize) - scaledRenderResolution) / 2.0f);
				dstEnd = dstStart + glm::ivec2(scaledRenderResolution);
				break;

			case RenderSettings::BotLeft:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(0, 0);
				dstEnd = dstStart + glm::ivec2(scaledRenderResolution);
				break;

			case RenderSettings::BotRight:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(windowSize.x - scaledRenderResolution.x, 0);
				dstEnd = dstStart + glm::ivec2(scaledRenderResolution);
				break;

			case RenderSettings::TopLeft:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(0, windowSize.y - scaledRenderResolution.y);
				dstEnd = dstStart + glm::ivec2(scaledRenderResolution);
				break;

			case RenderSettings::TopRight:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(windowSize - scaledRenderResolution);
				dstEnd = dstStart + glm::ivec2(scaledRenderResolution);
				break;
			}
			break;
		case RenderSettings::Percentage:
			switch (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitAnchor)
			{
			case RenderSettings::Center:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2((glm::vec2(windowSize) - scaledWindowSize) / 2.0f);
				dstEnd = dstStart + glm::ivec2(scaledWindowSize);
				break;

			case RenderSettings::BotLeft:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(0, 0);
				dstEnd = dstStart + glm::ivec2(scaledWindowSize);
				break;

			case RenderSettings::BotRight:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(windowSize.x - scaledWindowSize.x, 0);
				dstEnd = dstStart + glm::ivec2(scaledWindowSize);
				break;

			case RenderSettings::TopLeft:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(0, windowSize.y - scaledWindowSize.y);
				dstEnd = dstStart + glm::ivec2(scaledWindowSize);
				break;

			case RenderSettings::TopRight:
				srcStart = glm::ivec2(0, 0);
				srcEnd = srcStart + glm::ivec2(renderResolution);
				dstStart = glm::ivec2(windowSize - scaledWindowSize);
				dstEnd = dstStart + glm::ivec2(scaledWindowSize);
				break;
			}
			break;
		case RenderSettings::Fullscreen:
			srcStart = glm::ivec2(0, 0);
			srcEnd = srcStart + glm::ivec2(renderResolution);
			dstStart = glm::ivec2(0, 0);
			dstEnd = srcStart + glm::ivec2(windowSize);
			break;
		}

		// Filtering algorithm
		GLenum filtering;

		switch (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitFiltering)
		{
		case RenderSettings::Point:
			filtering = GL_NEAREST;
			break;
		case RenderSettings::Linear:
			filtering = GL_LINEAR;
			break;
		}

		// Copy the contents into the display buffer
		if (GuiSettings::guiVisible(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_GUI_SETTINGS)) == false && 
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_blitMode != RenderSettings::NoBlit)
		{
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glDisable(GL_SCISSOR_TEST);
			bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings, GB_WriteBuffer, GB_ReadBuffer, 0, GL_READ_FRAMEBUFFER);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_BACK);
			glBlitFramebuffer(srcStart.x, srcStart.y, srcEnd.x, srcEnd.y, dstStart.x, dstStart.y, dstEnd.x, dstEnd.y, GL_COLOR_BUFFER_BIT, filtering);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
		else
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void swapGBuffersOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		swapGbuffers(scene, simulationSettings, renderSettings);
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(Rendering, Renderer, -renderer, OpenGL)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"renderer", "Rendering",
			"Default renderer.",
			"RENDERER", { "OpenGL" },
			{
				{ "OpenGL", "OpenGL renderer" }
			},
			Config::attribRegexString()
		});

		// @CONSOLE_VAR(Rendering, Resolution, -resolution, 3840x2160, 2560x1440, 1920x1080, 1366x768, 1280x720, 960x540, 640x360, 320x180, 160x90)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"resolution", "Rendering",
			"Default render resolution.",
			"WxH", { "1280x720" }, {},
			Config::attribRegexInt(2)
		});

		// @CONSOLE_VAR(Rendering, Max Resolution, -max_resolution, 3840x2160, 2560x1440, 1920x1080, 1366x768, 1280x720, 960x540, 640x360, 320x180, 160x90)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"max_resolution", "Rendering",
			"Maximum possible resolution.",
			"WxH", { "0x0" }, {},
			Config::attribRegexInt(2)
		});

		// @CONSOLE_VAR(Rendering, MSAA, -msaa, 1, 2, 4, 8, 16)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"msaa", "Rendering",
			"MSAA samples.",
			"N", { "1" }, {},
			Config::attribRegexInt()
		});

		// @CONSOLE_VAR(Rendering, Voxel Grid Resolution, -voxel_grid, 512, 256, 128, 64, 32, 16, 8)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"voxel_grid", "Rendering",
			"Voxel grid size.",
			"N", { "256" }, {},
			Config::attribRegexInt()
		});

		// @CONSOLE_VAR(Rendering, Shadow Map Resolution, -shadow_map_resolution, 8192, 4096, 2048, 1024, 512)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"shadow_map_resolution", "Rendering",
			"Reference resolution for shadow maps.",
			"N", { "4096" }, {},
			Config::attribRegexInt()
		});

		// @CONSOLE_VAR(Rendering, Num Layers, -layers, 1, 2, 3, 4)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"layers", "Rendering",
			"Maximum (and default) number of layers.",
			"N", { "1" }, {},
			Config::attribRegexInt()
		});
	};
}