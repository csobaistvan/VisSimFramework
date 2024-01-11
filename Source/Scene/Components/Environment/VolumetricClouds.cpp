#include "PCH.h"
#include "VolumetricClouds.h"

#include "../Lighting/DirectionalLight.h"

// Based on: https://github.com/clayjohn/godot-volumetric-cloud-demo

namespace VolumetricClouds
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(VOLUMETRTIC_CLOUDS);
	DEFINE_OBJECT(VOLUMETRTIC_CLOUDS);
	REGISTER_OBJECT_UPDATE_CALLBACK(VOLUMETRTIC_CLOUDS, AFTER, INPUT);
	REGISTER_OBJECT_RENDER_CALLBACK(VOLUMETRTIC_CLOUDS, "Volumetric Clouds", OpenGL, AFTER, "Lighting [End]", 1, &VolumetricClouds::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	static const char* s_weatherMapName = "Textures/VolumetricClouds/weather.bmp";
	static const char* s_worlNoiseName = "Textures/VolumetricClouds/worlnoise.bmp";
	static const char* s_perlWorlNoiseName = "Textures/VolumetricClouds/perlworlnoise.tga";

	////////////////////////////////////////////////////////////////////////////////
	void initTextures(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Asset::loadTexture(scene, s_weatherMapName, s_weatherMapName);
		Asset::load3DTexture(scene, s_worlNoiseName, s_worlNoiseName);
		Asset::load3DTexture(scene, s_perlWorlNoiseName, s_perlWorlNoiseName);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			RenderSettings::DepthPeelAlgorithm_meta
		);

		Asset::loadShader(scene, "Environment/VolumetricClouds", "volumetric_clouds", "", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "VolumetricClouds", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initTextures, "Textures");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, initGPUBuffers, "Generic GPU Buffers");

		DelayedJobs::postJob(scene, &object, "Assign Sun", true, 2,
			[](Scene::Scene& scene, Scene::Object& object)
			{
				Scene::Object* sun = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT);
				if (sun != nullptr)
					object.component<VolumetricClouds::VolumetricCloudsComponent>().m_sunName = sun->m_name;
			});
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* getSun(Scene::Scene& scene, Scene::Object* object)
	{
		// Make sure the object exists
		std::string const& sunName = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunName;
		if (scene.m_objects.find(sunName) == scene.m_objects.end()) return nullptr;
		// Make sure it has a directiona light component
		Scene::Object* sun = &scene.m_objects[sunName];
		if (!sun->hasComponent<DirectionalLight::DirectionalLightComponent>()) return nullptr;
		// All good, return the selected object
		return sun;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Update the sun, if necessary
		if (getSun(scene, object) == nullptr)
			if (Scene::Object* sun = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT); sun != nullptr)
				object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunName = sun->m_name;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::SliderFloat("Density", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_density, 0.01f, 0.2f);
		ImGui::SliderFloat("Cloud Coverage", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_cloudCoverage, 0.1f, 1.0f);

		ImGui::SliderFloat("Rayleigh", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_rayleigh, 0.0f, 64.0f);
		ImGui::ColorEdit3("Rayleigh Color", glm::value_ptr(object->component<VolumetricClouds::VolumetricCloudsComponent>().m_rayleighColor));

		ImGui::SliderFloat("Mie", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_mie, 0.0f, 0.1f);
		ImGui::SliderFloat("Mie Eccentricity", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_mieEccentricity, -1.0f, 1.0f);
		ImGui::ColorEdit3("Mie Color", glm::value_ptr(object->component<VolumetricClouds::VolumetricCloudsComponent>().m_mieColor));

		ImGui::SliderFloat("Turbidity", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_turbidity, 0.0f, 1000.0f);
		ImGui::SliderFloat("Sun Energy Scale", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunEnergyScale, 0.0f, 10000.0f);
		ImGui::SliderFloat("Sun Disk Scale", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunDiskScale, 0.0f, 360.0f);
		ImGui::SliderFloat("Exposure", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_exposure, 0.0f, 128.0f);
		ImGui::ColorEdit3("Ground Color", glm::value_ptr(object->component<VolumetricClouds::VolumetricCloudsComponent>().m_groundColor));

		ImGui::Checkbox("Render To All Layers", &object->component<VolumetricClouds::VolumetricCloudsComponent>().m_renderToAllLayers);

		GuiSettings::objectPicker(scene, "Sun", object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunName, Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT,
			true, false, false);

		if (ImGui::Button("Remove"))
		{
			removeObject(scene, *object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Bind the new buffer
		RenderSettings::bindGbufferLayersOpenGL(scene, simulationSettings, renderSettings,
			RenderSettings::GB_WriteBuffer, RenderSettings::GB_ReadBuffer);
		RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);

		// Set the OGL state
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_EQUAL);
		glDisable(GL_BLEND);
		glDepthMask(GL_FALSE);

		// Bind the corresponding shader
		Scene::bindShader(scene, "Environment/VolumetricClouds", "volumetric_clouds");
		
		// Upload the necessary uniforms
		VolumetricClouds::UniformData uniformData;
		uniformData.m_renderToAllLayers = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_renderToAllLayers ? 1.0f : 0.0f;
		uniformData.m_density = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_density;
		uniformData.m_cloudCoverage = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_cloudCoverage;
		uniformData.m_rayleigh = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_rayleigh;
		uniformData.m_mie = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_mie;
		uniformData.m_mieEccentricity = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_mieEccentricity;
		uniformData.m_turbidity = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_turbidity;
		uniformData.m_sunEnergyScale = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunEnergyScale;
		uniformData.m_sunDiskScale = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_sunDiskScale;
		uniformData.m_exposure = object->component<VolumetricClouds::VolumetricCloudsComponent>().m_exposure;
		uniformData.m_rayleighColor = glm::vec4(object->component<VolumetricClouds::VolumetricCloudsComponent>().m_rayleighColor, 1.0f);
		uniformData.m_mieColor = glm::vec4(object->component<VolumetricClouds::VolumetricCloudsComponent>().m_mieColor, 1.0f);
		uniformData.m_groundColor = glm::vec4(object->component<VolumetricClouds::VolumetricCloudsComponent>().m_groundColor, 1.0f);
		if (Scene::Object* sun = getSun(scene, object); sun != nullptr) // upload the sun parameters
		{
			uniformData.m_mainLightColor = glm::vec4(sun->component<DirectionalLight::DirectionalLightComponent>().m_color, 1.0f);
			uniformData.m_mainLightDir = glm::vec4(-Transform::getForwardVector(sun), 0.0f);
			uniformData.m_mainLightEnergy = sun->component<DirectionalLight::DirectionalLightComponent>().m_diffuseIntensity;
		}
		uploadBufferData(scene, "VolumetricClouds", uniformData);

		// Bind the necessary noise textures
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
		glBindTexture(GL_TEXTURE_3D, scene.m_textures[s_worlNoiseName].m_texture);
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_2_ENUM);
		glBindTexture(GL_TEXTURE_3D, scene.m_textures[s_perlWorlNoiseName].m_texture);
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_3_ENUM);
		glBindTexture(GL_TEXTURE_2D, scene.m_textures[s_weatherMapName].m_texture);

		// Bind the previous frame's depth buffer
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings, RenderSettings::GB_ReadBuffer);

		// Render the fullscreen quad
		RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
	}
}