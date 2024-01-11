#include "PCH.h"
#include "Skybox.h"

namespace SkyBox
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(SKYBOX);
	DEFINE_OBJECT(SKYBOX);
	REGISTER_OBJECT_RENDER_CALLBACK(SKYBOX, "Skybox", OpenGL, AFTER, "Lighting [End]", 1, &SkyBox::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	void initTextures(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		for (auto skybox : scene.m_skyboxNames)
		{
			Asset::loadCubeMap(scene, skybox, skybox);
		}
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

		Asset::loadShader(scene, "Environment/Skybox", "skybox", "", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "Skybox", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initTextures, "Textures");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, initGPUBuffers, "Generic GPU Buffers");
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::ColorEdit3("Color", glm::value_ptr(object->component<SkyBox::SkyBoxComponent>().m_tint));
		if (ImGui::InputTextPreset("Texture", object->component<SkyBox::SkyBoxComponent>().m_textureName, scene.m_skyboxNames, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			Asset::loadCubeMap(scene, object->component<SkyBox::SkyBoxComponent>().m_textureName, object->component<SkyBox::SkyBoxComponent>().m_textureName);
		}
		ImGui::Checkbox("Render To All Layers", &object->component<SkyBox::SkyBoxComponent>().m_renderToAllLayers);
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
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_EQUAL);
		glDisable(GL_BLEND);
		glDepthMask(GL_FALSE);

		// Bind the corresponding shader
		Scene::bindShader(scene, "Environment/Skybox", "skybox");

		// Upload the necessary uniforms
		SkyBox::UniformData skyboxData;
		skyboxData.m_color = object->component<SkyBox::SkyBoxComponent>().m_tint;
		skyboxData.m_hasTexture = object->component<SkyBox::SkyBoxComponent>().m_textureName.empty() ? 0.0f : 1.0f;
		skyboxData.m_renderToAllLayers = object->component<SkyBox::SkyBoxComponent>().m_renderToAllLayers ? 1.0f : 0.0f;
		uploadBufferData(scene, "Skybox", skyboxData);

		// Bind the cubemap, if any
		if (!object->component<SkyBox::SkyBoxComponent>().m_textureName.empty())
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_ALBEDO_MAP_ENUM);
			glBindTexture(GL_TEXTURE_CUBE_MAP, scene.m_textures[object->component<SkyBox::SkyBoxComponent>().m_textureName].m_texture);
		}

		// Bind the previous frame's depth buffer
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings, RenderSettings::GB_ReadBuffer);

		// Render the fullscreen quad
		RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
	}
}