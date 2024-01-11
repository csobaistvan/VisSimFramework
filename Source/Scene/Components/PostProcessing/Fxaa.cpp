#include "PCH.h"
#include "Fxaa.h"

namespace Fxaa
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(FXAA);
	DEFINE_OBJECT(FXAA);
	REGISTER_OBJECT_RENDER_CALLBACK(FXAA, "FXAA [HDR][Post-Lighting]", OpenGL, AFTER, "Effects (HDR) [Begin]", 99, &Fxaa::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &Fxaa::renderObjectPreconditionPostLightingHDROpenGL, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(FXAA, "FXAA [HDR][Post-Effects]", OpenGL, BEFORE, "Effects (HDR) [End]", 99, &Fxaa::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &Fxaa::renderObjectPreconditionPostPostprocessingHDROpenGL, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(FXAA, "FXAA [LDR][Post-Lighting]", OpenGL, AFTER, "Effects (LDR) [Begin]", 99, &Fxaa::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &Fxaa::renderObjectPreconditionPostLightingLDROpenGL, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(FXAA, "FXAA [LDR][Post-Effects]", OpenGL, BEFORE, "Effects (LDR) [End]", 99, &Fxaa::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &Fxaa::renderObjectPreconditionPostPostprocessingLDROpenGL, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Asset::loadShader(scene, "PostProcessing/FXAA", "fxaa", "FXAA/fxaa");
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "FXAA", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
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
		ImGui::Combo("Color Space", &object->component<Fxaa::FxaaComponent>().m_colorSpace, Fxaa::FxaaComponent::ColorSpace_meta);
		ImGui::Combo("Domain", &object->component<Fxaa::FxaaComponent>().m_domain, Fxaa::FxaaComponent::ComputationDomain_meta);
		ImGui::SliderFloat("Minium Direction Reduction", &object->component<Fxaa::FxaaComponent>().m_dirReduceMin, 0.0f, 0.01f);
		ImGui::SliderFloat("Direction Reduction Multiplier", &object->component<Fxaa::FxaaComponent>().m_dirReduceMultiplier, 0.0f, 4.0f);
		ImGui::SliderFloat("Max Blur Width", &object->component<Fxaa::FxaaComponent>().m_maxBlur, 0.0f, 8.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPrecondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object, FxaaComponent::ColorSpace colorSpace, FxaaComponent::ComputationDomain computationDomain)
	{
		return object->component<Fxaa::FxaaComponent>().m_colorSpace == colorSpace &&
			object->component<Fxaa::FxaaComponent>().m_domain == computationDomain &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostLightingHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return renderObjectPrecondition(scene, simulationSettings, renderSettings, camera, functionName, object, FxaaComponent::HDR, FxaaComponent::AfterLighting);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostLightingLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return renderObjectPrecondition(scene, simulationSettings, renderSettings, camera, functionName, object, FxaaComponent::LDR, FxaaComponent::AfterLighting);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostPostprocessingHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return renderObjectPrecondition(scene, simulationSettings, renderSettings, camera, functionName, object, FxaaComponent::HDR, FxaaComponent::AfterPostprocessing);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionPostPostprocessingLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return renderObjectPrecondition(scene, simulationSettings, renderSettings, camera, functionName, object, FxaaComponent::LDR, FxaaComponent::AfterPostprocessing);
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Upload the fxaa parameters
		Fxaa::UniformData fxaaData;
		fxaaData.m_dirReduceMin = object->component<Fxaa::FxaaComponent>().m_dirReduceMin;
		fxaaData.m_dirReduceMultiplier = object->component<Fxaa::FxaaComponent>().m_dirReduceMultiplier;
		fxaaData.m_maxBlur = object->component<Fxaa::FxaaComponent>().m_maxBlur;
		uploadBufferData(scene, "FXAA", fxaaData);

		// Bind the new buffer
		RenderSettings::bindGbufferLayersOpenGL(scene, simulationSettings, renderSettings);
		RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);
		Scene::bindShader(scene, "FXAA", "fxaa");

		// Bind the scene texture
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings);

		// Render the fullscreen quad
		RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
	}
}