#include "PCH.h"
#include "MotionBlur.h"

namespace MotionBlur
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(MOTION_BLUR);
	DEFINE_OBJECT(MOTION_BLUR);
	REGISTER_OBJECT_RENDER_CALLBACK(MOTION_BLUR, "Motion Blur", OpenGL, AFTER, "Effects (LDR) [Begin]", 1, &MotionBlur::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getTileSize(Scene::Scene& scene, Scene::Object* renderSettings, Scene::Object* object)
	{
		const glm::ivec2 resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		const int tileSize = object->component<MotionBlur::MotionBlurComponent>().m_tileSize;
		return ((resolution + tileSize - 1) / tileSize);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getTileSize(Scene::Scene& scene, Scene::Object* object)
	{
		return getTileSize(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS), object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Asset::loadShader(scene, "PostProcessing/MotionBlur", "tile_max", "MotionBlur/tile_max");
		Asset::loadShader(scene, "PostProcessing/MotionBlur", "neighbor_max", "MotionBlur/neighbor_max");
		Asset::loadShader(scene, "PostProcessing/MotionBlur", "composite", "MotionBlur/composite");
	}

	////////////////////////////////////////////////////////////////////////////////
	void initFramebuffers(Scene::Scene& scene, Scene::Object* object)
	{
		glm::ivec2 textureSize = getTileSize(scene, object);
		Scene::createTexture(scene, "MotionBlur_TileMax", GL_TEXTURE_2D, textureSize[0], textureSize[1], 1, GL_RG16F, GL_RG, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
		Scene::createTexture(scene, "MotionBlur_NeighborMax", GL_TEXTURE_2D, textureSize[0], textureSize[1], 1, GL_RG16F, GL_RG, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "MotionBlur", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initFramebuffers, "FrameBuffers");
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
		bool recreateBuffers = false;
		ImGui::SliderFloat("Velocity Scale Factor", &object->component<MotionBlur::MotionBlurComponent>().m_velocityScaleFactor, 0.0f, 8.0f);
		ImGui::SliderFloat("Max Velocity", &object->component<MotionBlur::MotionBlurComponent>().m_maxVelocity, 0.0f, 8.0f);
		ImGui::SliderInt("Num Taps", &object->component<MotionBlur::MotionBlurComponent>().m_numTaps, 0, 64);
		recreateBuffers |= ImGui::SliderInt("Tile Size", &object->component<MotionBlur::MotionBlurComponent>().m_tileSize, 20, 120);
		ImGui::SliderFloat("Center Weight", &object->component<MotionBlur::MotionBlurComponent>().m_centerWeight, 0.0f, 2.0f);
		ImGui::SliderFloat("Tile Falloff", &object->component<MotionBlur::MotionBlurComponent>().m_tileFalloff, 0.0f, 2.0f);
		ImGui::SliderFloat("Interpolation Threshold", &object->component<MotionBlur::MotionBlurComponent>().m_interpolationThreshold, 0.0f, 8.0f);
		ImGui::SliderFloat("Jitter Scale", &object->component<MotionBlur::MotionBlurComponent>().m_jitterScale, 0.0f, 8.0f);

		if (recreateBuffers) initFramebuffers(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Extract the tile size
		const glm::ivec2 tileSize = getTileSize(scene, renderSettings, object);

		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Upload the fxaa parameters
		UniformData motionBlurData;
		motionBlurData.m_velocityScaleFactor = object->component<MotionBlur::MotionBlurComponent>().m_velocityScaleFactor;
		motionBlurData.m_maxVelocity = object->component<MotionBlur::MotionBlurComponent>().m_maxVelocity;
		motionBlurData.m_numTaps = object->component<MotionBlur::MotionBlurComponent>().m_numTaps;
		motionBlurData.m_tileSize = object->component<MotionBlur::MotionBlurComponent>().m_tileSize;
		motionBlurData.m_centerWeight = object->component<MotionBlur::MotionBlurComponent>().m_centerWeight;
		motionBlurData.m_tileFalloff = object->component<MotionBlur::MotionBlurComponent>().m_tileFalloff;
		motionBlurData.m_interpolationThreshold = object->component<MotionBlur::MotionBlurComponent>().m_interpolationThreshold;
		motionBlurData.m_jitterScale = object->component<MotionBlur::MotionBlurComponent>().m_jitterScale;
		uploadBufferData(scene, "MotionBlur", motionBlurData);

		// Bind the proper textures and shader
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings);

		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Tile Max");

			// Compute the scene luminance
			glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures["MotionBlur_TileMax"].m_framebuffer);
			glViewport(0, 0, tileSize.x, tileSize.y);
			Scene::bindShader(scene, "MotionBlur", "tile_max");

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Neighbor Max");

			// Compute the scene luminance
			glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures["MotionBlur_NeighborMax"].m_framebuffer);
			glViewport(0, 0, tileSize.x, tileSize.y);
			Scene::bindShader(scene, "MotionBlur", "neighbor_max");

			// Bind the tile max buffer
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures["MotionBlur_TileMax"].m_texture);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Composite");

			// Bind the neighbor buffer
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_2_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures["MotionBlur_NeighborMax"].m_texture);

			// Bind the new buffer
			RenderSettings::bindGbufferLayersOpenGL(scene, simulationSettings, renderSettings);
			RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);
			Scene::bindShader(scene, "MotionBlur", "composite");

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
	}
}
