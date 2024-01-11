#include "PCH.h"
#include "VoxelGlobalIllumination.h"

namespace VoxelGlobalIllumination
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(VOXEL_GLOBAL_ILLUMINATION);
	DEFINE_OBJECT(VOXEL_GLOBAL_ILLUMINATION);
	REGISTER_OBJECT_UPDATE_CALLBACK(VOXEL_GLOBAL_ILLUMINATION, AFTER, RENDER_SETTINGS);
	REGISTER_OBJECT_RENDER_CALLBACK(VOXEL_GLOBAL_ILLUMINATION, "Voxel Lighting [Inject Indirect]", OpenGL, BEFORE, "Voxel Lighting [End]", 2, &VoxelGlobalIllumination::injectIndirectLightingOpenGL, &VoxelGlobalIllumination::injectIndirectLightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(VOXEL_GLOBAL_ILLUMINATION, "Lighting [Voxel GI]", OpenGL, AFTER, "Lighting [Begin]", 2, &VoxelGlobalIllumination::lightingOpenGL, &VoxelGlobalIllumination::lightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, VoxelGlobalIllumination::lightingBeginOpenGL, VoxelGlobalIllumination::lightingEndOpenGL);

	static const size_t SIZE = sizeof(UniformDataGIPass);

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
			RenderSettings::BrdfModel_meta,
			RenderSettings::VoxelDilationMethod_meta,
			RenderSettings::VoxelShadingMode_meta,
			VoxelGlobalIllumination::VoxelGlobalIlluminationComponent::LightContribution_meta
		);

		// Voxel mipmaps
		Asset::loadShader(scene, "Shading/Voxel/MipMap", "generate_anisotropic_mipmap", "Shading/generate_anisotropic_voxel_mipmap", shaderParameters);

		// Indirect injection shader
		Asset::loadShader(scene, "Shading/Voxel/InjectIndirectLight", "inject_indirect_light", "Shading/inject_indirect_light", shaderParameters);
		
		// Main GI shader variants
		RenderSettings::loadShaderMsaaVariants(scene, "Shading/Voxel/GlobalIllumination", "global_illumination", "Shading/voxel_global_illumination", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "InjectIndirectPass", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
		Scene::createGPUBuffer(scene, "VoxelGIPass", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
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
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		// Extract the render settings object
		Scene::Object* renderSettings = findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		bool hasIndirectBounce = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces > 1;
		bool lightingChanged = false;

		lightingChanged |= ImGui::Combo("Diffuse Contribution", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseContribution, VoxelGlobalIllumination::VoxelGlobalIlluminationComponent::LightContribution_meta);
		ImGui::Combo("Specular Contribution", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularContribution, VoxelGlobalIllumination::VoxelGlobalIlluminationComponent::LightContribution_meta);
		lightingChanged |= ImGui::SliderInt("Num Diffuse Bounces", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces, 1, 16);
		lightingChanged |= ImGui::SliderFloat("Radiance Decay Rate", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_radianceDecayRate, 0.0f, 10.0f);
		ImGui::SliderFloat("Occlusion Decay Rate", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionDecayRate, 0.0f, 10.0f);
		ImGui::SliderFloat("Occlusion Exponent", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionExponent, 0.0f, 32.0f);
		ImGui::SliderFloat("Minimum Occlusion", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_minOcclusion, 0.0f, 1.0f);
		lightingChanged |= ImGui::SliderFloat("Diffuse Trace Start Offset", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceStartOffset, 0.01f, 8.0f, "%.3f vx");
		ImGui::SliderFloat("Specular Trace Start Offset", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularTraceStartOffset, 0.01f, 8.0f, "%.3f vx");
		lightingChanged |= ImGui::SliderFloat("Diffuse Trace Normal Offset", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceNormalOffset, 0.01f, 8.0f, "%.3f vx");
		ImGui::SliderFloat("Specular Trace Normal Offset", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularTraceNormalOffset, 0.01f, 8.0f, "%.3f vx");
		ImGui::SliderAngle("Specular Aperture (Min)", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularApertureMin, 0.0f, 60.0f);
		ImGui::SliderAngle("Specular Aperture (Max)", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularApertureMax, 0.0f, 60.0f);
		lightingChanged |= ImGui::SliderFloat("Max Trace Distance", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_maxTraceDistance, 0.01f, 10.0f, "%.3f m");
		ImGui::SliderFloat("Diffuse Intensity", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseIntensity, 0.0f, 8.0f);
		ImGui::SliderFloat("Specular Intensity", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularIntensity, 0.0f, 8.0f);
		lightingChanged |= ImGui::Checkbox("Anisotropic Sampling (Diffuse)", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicDiffuse);
		ImGui::SameLine();
		ImGui::Checkbox("Anisotropic Sampling (Specular)", &object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicSpecular);

		hasIndirectBounce |= object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces > 1;
		if (hasIndirectBounce && lightingChanged) RenderSettings::updateVoxelGridRadiance(scene);
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateMipmaps(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Isotropic Mipmaps");

			// sync barrier
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

			// Generate mipmaps
			glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_radianceTexture);
			glGenerateMipmap(GL_TEXTURE_3D);
			for (int i = 0; i < 6; ++i)
			{
				glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_radianceMipmaps[i]);
				glGenerateMipmap(GL_TEXTURE_3D);
			}
		}

		if (scene.m_voxelGrid.m_anisotropic)
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Anisotropic Mipmaps");

			// Bind the shader
			Scene::bindShader(scene, "Shading", "generate_anisotropic_voxel_mipmap");

			// Go through the mip levels
			unsigned mipDimension = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels / 2;
			unsigned mipLevel = 0;
			while (mipDimension >= 1)
			{
				// Bind the target textures
				for (int i = 0; i < 6 && scene.m_voxelGrid.m_anisotropic; ++i)
				{
					glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM + i);
					if (mipLevel == 0)
						glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_radianceTexture);
					else
						glBindTexture(GL_TEXTURE_3D, scene.m_voxelGrid.m_radianceMipmaps[i]);
					glBindImageTexture(i, scene.m_voxelGrid.m_radianceMipmaps[i], mipLevel, GL_TRUE, 0, GL_WRITE_ONLY, scene.m_voxelGrid.m_radianceDataFormat);
				}

				// Set the necessary uniuforms
				const glm::vec3 volumeSize = glm::vec3(mipDimension, mipDimension, mipDimension);
				glUniform3fv(0, 1, glm::value_ptr(volumeSize));
				glUniform1ui(1, mipLevel < 2 ? 0 : mipLevel - 1);

				// Dispatch the shader
				const unsigned workGroups = unsigned(glm::ceil(mipDimension / 8.0f));
				glDispatchCompute(workGroups, workGroups, workGroups);

				// sync barrier
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

				// Go to the next mip level
				mipLevel++;
				mipDimension /= 2;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool injectIndirectLightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelRadiance &&
			RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	void injectIndirectLightingOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Generate mipmaps
		generateMipmaps(scene, renderSettings);

		// Ignore the rest if only a single indirect bounce is requested
		if (object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces <= 1)
			return;

		// Upload the necessary uniforms
		VoxelGlobalIllumination::UniformDataInjectIndirectPass lightData;
		lightData.m_traceParams.m_contribution = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseContribution;
		lightData.m_traceParams.m_aperture = glm::tan(object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseAperture * 0.5f);
		lightData.m_traceParams.m_mipOffset = 1.0f;
		lightData.m_traceParams.m_startOffset = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceStartOffset;
		lightData.m_traceParams.m_normalOffset = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceNormalOffset;
		lightData.m_traceParams.m_maxTraceDistance = RenderSettings::metersToUnits(scene, renderSettings, object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_maxTraceDistance);
		lightData.m_traceParams.m_anisotropic = (object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicDiffuse &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_anisotropicVoxels) ? 1 : 0;
		lightData.m_traceParams.m_radianceDecayRate = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_radianceDecayRate;
		lightData.m_traceParams.m_occlusionDecayRate = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionDecayRate;
		lightData.m_traceParams.m_occlusionExponent = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionExponent;
		lightData.m_traceParams.m_minOcclusion = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_minOcclusion;
		lightData.m_traceParams.m_intensity = 1.0f;
		uploadBufferData(scene, "InjectIndirectPass", lightData);

		// Inject indirect bounces
		for (size_t i = 1; i < object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces && i < 8; ++i)
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Bounce #" + std::to_string(i));

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

			// Bind the proper shader 
			Scene::bindShader(scene, "Shading", "inject_indirect_light");

			// Number of work groups to cover the entire voxel grid
			unsigned numWorkGroups = (glm::ceil(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels / 8.0f));

			// inject radiance at level 0 of texture
			glDispatchCompute(numWorkGroups, numWorkGroups, numWorkGroups);

			// Generate mipmaps
			generateMipmaps(scene, renderSettings);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool lightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName) &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingMode != RenderSettings::DirectOnly &&
			Scene::filterObjects(scene, Scene::OBJECT_TYPE_VOXEL_GLOBAL_ILLUMINATION, true, false, true).size() > 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	void lightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Bind the proper shader
		RenderSettings::bindShaderMsaaVariant(scene, renderSettings, "Shading", "voxel_global_illumination");

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
	void lightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		if (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_aoAffectsDirectLighting)
		{
			// Change the blend func to multiply the results by the AO term
			glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void lightingOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Number of diffuse bounces
		const int numBounces = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_numDiffuseBounces;

		// Intensity scaling, based on the number of indirect bounces
		const float intensityScaling = 1.0f / float(glm::max(1, numBounces));
		
		// Uniform structure for holding the light source info
		VoxelGlobalIllumination::UniformDataGIPass lightData;

		// Diffuse trace parameters
		lightData.m_diffuseTraceParams.m_contribution = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseContribution;
		lightData.m_diffuseTraceParams.m_aperture = glm::tan(object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseAperture * 0.5f);
		lightData.m_diffuseTraceParams.m_mipOffset = 0.0f;
		lightData.m_diffuseTraceParams.m_startOffset = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceStartOffset;
		lightData.m_diffuseTraceParams.m_normalOffset = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseTraceNormalOffset;
		lightData.m_diffuseTraceParams.m_maxTraceDistance = RenderSettings::metersToUnits(scene, renderSettings, object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_maxTraceDistance);
		lightData.m_diffuseTraceParams.m_anisotropic = (object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicDiffuse &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_anisotropicVoxels) ? 1 : 0;
		lightData.m_diffuseTraceParams.m_radianceDecayRate = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_radianceDecayRate;
		lightData.m_diffuseTraceParams.m_occlusionDecayRate = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionDecayRate;
		lightData.m_diffuseTraceParams.m_occlusionExponent = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_occlusionExponent;
		lightData.m_diffuseTraceParams.m_minOcclusion = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_minOcclusion;
		lightData.m_diffuseTraceParams.m_intensity = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_diffuseIntensity * intensityScaling;

		// Specular trace parameters
		lightData.m_specularTraceParams.m_contribution = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularContribution;
		lightData.m_specularTraceParams.m_apertureMin = glm::tan(object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularApertureMin * 0.5f);
		lightData.m_specularTraceParams.m_apertureMax = glm::tan(object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularApertureMax * 0.5f);
		lightData.m_specularTraceParams.m_mipLevelMin = 0.0f;
		lightData.m_specularTraceParams.m_mipLevelMax = glm::max(0.0f, glm::ceil(glm::log2(float(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels))) - 3.0f);
		lightData.m_specularTraceParams.m_startOffset = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularTraceStartOffset;
		lightData.m_specularTraceParams.m_normalOffset = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularTraceNormalOffset;
		lightData.m_specularTraceParams.m_maxTraceDistance = RenderSettings::metersToUnits(scene, renderSettings, object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_maxTraceDistance);
		lightData.m_specularTraceParams.m_anisotropic = (object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_anisotropicSpecular &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_anisotropicVoxels) ? 1 : 0;
		lightData.m_specularTraceParams.m_radianceDecayRate = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_radianceDecayRate;
		lightData.m_specularTraceParams.m_intensity = object->component<VoxelGlobalIllumination::VoxelGlobalIlluminationComponent>().m_specularIntensity * intensityScaling;

		// Upload the parameters
		uploadBufferData(scene, "VoxelGIPass", lightData);

		// Render the fullscreen quad
		RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
	}
}