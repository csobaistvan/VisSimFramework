#include "PCH.h"
#include "DebugVisualization.h"

namespace DebugVisualization
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(DEBUG_VISUALIZATION);
	DEFINE_OBJECT(DEBUG_VISUALIZATION);
	REGISTER_OBJECT_RENDER_CALLBACK(DEBUG_VISUALIZATION, "Debug Visualization [GBuffer]", OpenGL, BEFORE, "Present [Begin]", 1, &DebugVisualization::visualizeGBufferOpenGL, &RenderSettings::firstCallTypeCondition, &DebugVisualization::visualizeGbufferObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(DEBUG_VISUALIZATION, "Debug Visualization [Voxel Grid]", OpenGL, BEFORE, "Tone Map [Begin]", 1, &DebugVisualization::visualizeVoxelGridOpenGL, &RenderSettings::firstCallTypeCondition, &DebugVisualization::visualizeVoxelGridObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_defines =
		{
			"VOXEL_GBUFFER_TEXTURE_FORMAT " + RenderSettings::voxelGbufferGlShaderFormat(scene),
			"VOXEL_RADIANCE_TEXTURE_FORMAT " + RenderSettings::voxelRadianceGlShaderFormat(scene)
		};
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			DebugVisualization::BufferComponent_meta,
			DebugVisualization::BufferComponent_meta, 
			DebugVisualization::VoxelFace_meta
		);

		// GBuffer visualization shader
		RenderSettings::loadShaderMsaaVariants(scene, "PostProcessing/DebugVisualization/GBuffer", "visualize_gbuffer", "DebugVisualization/visualize_gbuffer", shaderParameters);

		// Voxel grid visualization shader
		Asset::loadShader(scene, "PostProcessing/DebugVisualization/VoxelGrid", "visualize_voxel_grid", "DebugVisualization/visualize_voxel_grid", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "DebugVisualization", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
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
		ImGui::Combo("Visualization Target", &object->component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget, DebugVisualization::VisualizationTarget_meta);
		if (object->component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget == DebugVisualization::GBuffer)
		{
			ImGui::Combo("Component", &object->component<DebugVisualization::DebugVisualizationComponent>().m_gbufferComponent, DebugVisualization::BufferComponent_meta);
			ImGui::SliderInt("Layer", &object->component<DebugVisualization::DebugVisualizationComponent>().m_layer, 0, GPU::numLayers() - 1);
			ImGui::SliderInt("LOD Level", &object->component<DebugVisualization::DebugVisualizationComponent>().m_lodLevel, 0, 16);;
		}
		if (object->component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget == DebugVisualization::VoxelGrid)
		{
			ImGui::Combo("Component", &object->component<DebugVisualization::DebugVisualizationComponent>().m_voxelComponent, DebugVisualization::BufferComponent_meta);
			ImGui::Combo("Face", &object->component<DebugVisualization::DebugVisualizationComponent>().m_voxelFace, DebugVisualization::VoxelFace_meta);
			ImGui::SliderInt("LOD Level", &object->component<DebugVisualization::DebugVisualizationComponent>().m_lodLevel, 0, 16);
		}

		ImGui::SliderFloat("Display Power", &object->component<DebugVisualization::DebugVisualizationComponent>().m_displayPower, 0.0f, 8.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool visualizeGbufferObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return object->component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget == DebugVisualization::GBuffer &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void visualizeGBufferOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Upload the parameters
		DebugVisualization::UniformDataGBuffer uniformData;
		uniformData.m_component = (GLint)object->component<DebugVisualization::DebugVisualizationComponent>().m_gbufferComponent;
		uniformData.m_layer = object->component<DebugVisualization::DebugVisualizationComponent>().m_layer;
		uniformData.m_lodLevel = object->component<DebugVisualization::DebugVisualizationComponent>().m_lodLevel;
		uniformData.m_displayPower = object->component<DebugVisualization::DebugVisualizationComponent>().m_displayPower;
		uploadBufferData(scene, "DebugVisualization", uniformData);

		// Bind the new buffer
		RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings);
		RenderSettings::setupViewportOpenGL(scene, simulationSettings, renderSettings);

		// Bind the shader
		RenderSettings::bindShaderMsaaVariant(scene, renderSettings, "DebugVisualization", "visualize_gbuffer");

		// Bind the proper textures
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings, 
			RenderSettings::GB_WriteBuffer, RenderSettings::GB_ReadBuffer, true);

		// Render the fullscreen quad
		RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool visualizeVoxelGridObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return object->component<DebugVisualization::DebugVisualizationComponent>().m_visualizationTarget == DebugVisualization::VoxelGrid &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void visualizeVoxelGridOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Set the OpenGL state
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);

		// Upload the parameters
		DebugVisualization::UniformDataVoxelGrid uniformData;
		uniformData.m_component = (GLint)object->component<DebugVisualization::DebugVisualizationComponent>().m_voxelComponent;
		uniformData.m_lodLevel = object->component<DebugVisualization::DebugVisualizationComponent>().m_lodLevel;
		uniformData.m_voxelFace = object->component<DebugVisualization::DebugVisualizationComponent>().m_voxelFace;
		uniformData.m_displayPower = object->component<DebugVisualization::DebugVisualizationComponent>().m_displayPower;
		uploadBufferData(scene, "DebugVisualization", uniformData);

		// Bind the new buffer
		RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings);
		RenderSettings::setupViewportOpenGL(scene, simulationSettings, renderSettings);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Also bind the shader
		Scene::bindShader(scene, "DebugVisualization", "visualize_voxel_grid");

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
		
		// Dummy VAO that is necessary for rendering
		static GLuint dummyVao = 0;
		if (dummyVao == 0)
			glGenVertexArrays(1, &dummyVao);

		// Render all the voxels
		glBindVertexArray(dummyVao);
		glDrawArrays(GL_POINTS, 0, std::pow(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels, 3));

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
	}
}
