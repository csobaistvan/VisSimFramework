#include "PCH.h"
#include "ShadowMap.h"

namespace ShadowMap
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(SHADOW_MAP);
	DEFINE_OBJECT(SHADOW_CASTER);
	REGISTER_OBJECT_RENDER_CALLBACK(SHADOW_CASTER, "Blur Shadow Maps", OpenGL, BEFORE, "Shadow Maps [End]", 2, &ShadowMap::blurShadowMapsOpenGL, &RenderSettings::firstCallTypeCondition, &ShadowMap::blurShadowMapsObjectConditionOpenGL, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	std::array<bool, 2> generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		bool textureSettingsChanged = false;
		bool shadowSettingsChanged = false;

		textureSettingsChanged |= ImGui::Checkbox("Casts Shadow", &object->component<ShadowMap::ShadowMapComponent>().m_castsShadow);
		if (!object->component<ShadowMap::ShadowMapComponent>().m_castsShadow)
			return { textureSettingsChanged, shadowSettingsChanged };

		textureSettingsChanged |= ImGui::SliderInt("Shadow Map Resolution", &object->component<ShadowMap::ShadowMapComponent>().m_resolution, 1, 16384);
		textureSettingsChanged |= ImGui::Combo("Update Policy", &object->component<ShadowMap::ShadowMapComponent>().m_updatePolicy, ShadowMap::ShadowMapComponent::UpdatePolicy_meta);
		textureSettingsChanged |= ImGui::Combo("Precision", &object->component<ShadowMap::ShadowMapComponent>().m_precision, ShadowMap::ShadowMapComponent::ShadowMapPrecision_meta);
		shadowSettingsChanged |= ImGui::DragFloat("Polygon Offset Constant", &object->component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant, 0.1f, 0.0f);
		shadowSettingsChanged |= ImGui::DragFloat("Polygon Offset Linear", &object->component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear, 0.01f, 0.0f);
		shadowSettingsChanged |= ImGui::DragFloat2("Clip Plane Offset", glm::value_ptr(object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset), 0.1f);
		shadowSettingsChanged |= ImGui::DragFloat2("Clip Plane Scale", glm::value_ptr(object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale), 0.1f);
		textureSettingsChanged |= ImGui::Combo("Shadow Map Algorithm", &object->component<ShadowMap::ShadowMapComponent>().m_algorithm, ShadowMap::ShadowMapComponent::ShadowMapAlgorithm_meta);
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Basic ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Moments)
		{
			ImGui::SliderFloat("Depth Bias", &object->component<ShadowMap::ShadowMapComponent>().m_depthBias, 0.0f, 1.0f, "%.8f");
		}
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Variance ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::ExponentialVariance)
		{
			ImGui::DragFloat("Min Variance", &object->component<ShadowMap::ShadowMapComponent>().m_minVariance, 1e-6f, 0.0f, 1.0f, "%.8f");
		}
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Variance ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::ExponentialVariance ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Moments)
		{
			ImGui::SliderFloat("Light Bleed Bias", &object->component<ShadowMap::ShadowMapComponent>().m_lightBleedBias, 0.0f, 1.0f, "%.6f");
		}
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Exponential)
		{
			shadowSettingsChanged |= ImGui::DragFloat("Exponential Constant", &object->component<ShadowMap::ShadowMapComponent>().m_exponentialConstants.x, 0.1f);
		}
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::ExponentialVariance)
		{
			shadowSettingsChanged |= ImGui::DragFloat2("Exponential Constants", glm::value_ptr(object->component<ShadowMap::ShadowMapComponent>().m_exponentialConstants), 0.1f);
		}
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Moments)
		{
			shadowSettingsChanged |= ImGui::DragFloat("Moments Bias", &object->component<ShadowMap::ShadowMapComponent>().m_momentsBias, 0.1f, 0.0f, 1.0f, "%.6f");
		}

		if (ImGui::SliderFloat("Blur Strength", &object->component<ShadowMap::ShadowMapComponent>().m_blurStrength, 0.0f, 8.0f))
		{
			shadowSettingsChanged = true;
			updateBlurKernels(scene, object);
		}

		if (ImGui::TreeNode("Material Ignore List"))
		{
			auto& ignoreMaterials = object->component<ShadowMap::ShadowMapComponent>().m_ignoreMaterials;
			int numIgnoreMaterials = ignoreMaterials.size();
			shadowSettingsChanged |= ImGui::DragInt("Number of Materials", &numIgnoreMaterials, 1, 0);
			if (numIgnoreMaterials != ignoreMaterials.size())
				ignoreMaterials.resize(numIgnoreMaterials);

			for (size_t i = 0; i < numIgnoreMaterials; ++i)
			{
				std::string const& label = "Material #" + std::to_string(i + 1);
				if (scene.m_materials.find(ignoreMaterials[i]) == scene.m_materials.end())
					ignoreMaterials[i] = scene.m_materials.begin()->first;
				shadowSettingsChanged |= ImGui::Combo(label.c_str(), ignoreMaterials[i], scene.m_materials);
			}

			ImGui::TreePop();
		}

		return { textureSettingsChanged, shadowSettingsChanged };
	}

	////////////////////////////////////////////////////////////////////////////////
	bool handleUpdatePolicy(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object, ShadowMap::ShadowMapSlice& slice)
	{
		// Ignore slices that were not updated in this frame
		if (!slice.m_needsUpdate) return false;

		// Store the last slice update time
		slice.m_lastUpdated = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;

		// Unset its dirty flag if we can use caching
		if (object->component<ShadowMap::ShadowMapComponent>().m_updatePolicy == ShadowMap::ShadowMapComponent::Cached)
			slice.m_needsUpdate = false;

		// Mark it for updating
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool blurShadowMapsObjectConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return object->component<ShadowMap::ShadowMapComponent>().m_castsShadow &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void blurShadowMapSlice(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object, ShadowMap::ShadowMapSlice const& slice)
	{
		// Blur kernel
		const auto kernel = ShadowMap::selectBlurKernel(scene, object);

		// Name of the shadow map textures
		const std::string shadowMapName = object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO;
		const std::string shadowMapBlurName = object->component<ShadowMap::ShadowMapComponent>().m_shadowMapBlurFBO;

		// Shadow map dimensions
		const glm::ivec2 shadowMapSize = glm::ivec2(scene.m_textures[shadowMapName].m_width, scene.m_textures[shadowMapName].m_height);

		const glm::vec2 uvMin = slice.m_textureOffset;
		const glm::vec2 uvScale = slice.m_textureScale;
		const glm::vec2 uvMax = uvMin + uvScale;
		const glm::vec2 stepSize = glm::vec2(1.0f) / glm::vec2(shadowMapSize);

		// Set the common uniforms
		glUniform2fv(1, 1, glm::value_ptr(uvScale));            // UV scale
		glUniform2fv(2, 1, glm::value_ptr(uvMin));              // min UV
		glUniform2fv(3, 1, glm::value_ptr(uvMax));              // max UV
		glUniform1ui(4, kernel.size());                         // kernel size
		glUniform2fv(5, kernel.size(), (float*)kernel.data());  // kernel weights

		// Horizontal pass
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Horizontal");

			// Bind the previous luminance texture
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[shadowMapName].m_texture);

			// Bind the intermediate buffer
			glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures[shadowMapBlurName].m_framebuffer);
			glViewport(slice.m_startCoords.x, slice.m_startCoords.y, slice.m_extents.x, slice.m_extents.y);

			// Set the blur direction uniform
			glUniform2f(0, stepSize.x, 0.0f);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Vertical pass
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Vertical");

			// Bind the previous luminance texture
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[shadowMapBlurName].m_texture);

			// Bind the output buffer
			glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures[shadowMapName].m_framebuffer);
			glViewport(slice.m_startCoords.x, slice.m_startCoords.y, slice.m_extents.x, slice.m_extents.y);

			// Set the blur direction uniform
			glUniform2f(0, 0.0f, stepSize.y);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void blurShadowMapsOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Access the current shadow caster
		auto& slices = object->component<ShadowMap::ShadowMapComponent>().m_slices;

		// Skip if the blur radius if too small
		if (object->component<ShadowMap::ShadowMapComponent>().m_blurStrength <= 1e-2f || ShadowMap::getBlurKernelDiameter(scene, object) <= 1)
		{
			// Mark the slices as updated if we don't need to do blurring
			Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
			for (auto& slice : slices)
			{
				handleUpdatePolicy(scene, simulationSettings, object, slice);
			}
			return;
		}

		// Get the name of the blur shader
		std::string const& shaderName = "Misc/" + ShadowMap::selectBlurShader(scene, object);

		// Load the shadow if not found
		if (scene.m_shaders.find(shaderName) == scene.m_shaders.end())
			RenderSettings::loadGaussianBlurShader(scene, ShadowMap::getBlurKernelDiameter(scene, object));

		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Bind the shader
		Scene::bindShader(scene, shaderName);

		// Name of the shadow map textures
		const std::string shadowMapName = object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO;
		const std::string shadowMapBlurName = object->component<ShadowMap::ShadowMapComponent>().m_shadowMapBlurFBO;

		// Blur the shadow maps
		for (auto& slice : slices)
		{
			// Handle the update policy and determine if we need an update
			if (!handleUpdatePolicy(scene, simulationSettings, object, slice)) continue;

			// Blur the slice
			blurShadowMapSlice(scene, simulationSettings, renderSettings, camera, functionName, object, slice);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseShadowMap(Scene::Scene& scene, Scene::Object* object)
	{
		// Delete the FBOs
		if (auto it = scene.m_textures.find(object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO); it != scene.m_textures.end())
		{
			glDeleteTextures(1, &it->second.m_texture);
			glDeleteFramebuffers(1, &it->second.m_framebuffer);
		}
		if (auto it = scene.m_textures.find(object->component<ShadowMap::ShadowMapComponent>().m_shadowMapBlurFBO); it != scene.m_textures.end())
		{
			glDeleteTextures(1, &it->second.m_texture);
			glDeleteFramebuffers(1, &it->second.m_framebuffer);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void regenerateShadowMap(Scene::Scene& scene, Scene::Object* object)
	{
		// Mark all slices for update
		for (auto& slice : object->component<ShadowMap::ShadowMapComponent>().m_slices)
			slice.m_needsUpdate = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	static std::unordered_map<ShadowMap::ShadowMapComponent::ShadowMapAlgorithm, int> s_numComponents =
	{
		{ ShadowMap::ShadowMapComponent::Basic, 1 },
		{ ShadowMap::ShadowMapComponent::Variance, 2 },
		{ ShadowMap::ShadowMapComponent::Exponential, 1 },
		{ ShadowMap::ShadowMapComponent::ExponentialVariance, 4 },
		{ ShadowMap::ShadowMapComponent::Moments, 4 },
	};

	////////////////////////////////////////////////////////////////////////////////
	static std::vector<std::unordered_map<ShadowMap::ShadowMapComponent::ShadowMapPrecision, GLenum>> s_formats =
	{
		{}, // 0
		{   // 1
			{ ShadowMap::ShadowMapComponent::F16, GL_R16F },
			{ ShadowMap::ShadowMapComponent::F32, GL_R32F },
		},
		{   // 2
			{ ShadowMap::ShadowMapComponent::F16, GL_RG16F },
			{ ShadowMap::ShadowMapComponent::F32, GL_RG32F },
		},
		{   // 3
			{ ShadowMap::ShadowMapComponent::F16, GL_RGB16F },
			{ ShadowMap::ShadowMapComponent::F32, GL_RGB32F },
		},
		{   // 4
			{ ShadowMap::ShadowMapComponent::F16, GL_RGBA16F },
			{ ShadowMap::ShadowMapComponent::F32, GL_RGBA32F },
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	static std::vector<GLenum> s_layouts =
	{
		GL_NONE, GL_RED, GL_RG, GL_RGB, GL_RGBA
	};

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMap(Scene::Scene& scene, Scene::Object* object)
	{
		// Early out if the object doesn't cast shadow
		if (!object->component<ShadowMap::ShadowMapComponent>().m_castsShadow || object->component<ShadowMap::ShadowMapComponent>().m_slices.empty())
			return;

		// Set the shadow map names
		object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO = object->m_name + "_ShadowMap";
		object->component<ShadowMap::ShadowMapComponent>().m_shadowMapBlurFBO = object->m_name + "_ShadowMapBlur";

		// Get a reference to the shadow map slices
		auto& slices = object->component<ShadowMap::ShadowMapComponent>().m_slices;

		// Compute the total size of the shadow map
		glm::ivec2 shadowMapDimensions{ 0, 0 };
		for (auto& slice: slices)
		{
			shadowMapDimensions = glm::max(shadowMapDimensions, slice.m_startCoords + slice.m_extents);
		}

		// Texture format and layout
		const int numComponents = s_numComponents[object->component<ShadowMap::ShadowMapComponent>().m_algorithm];
		const GLenum format = s_formats[numComponents][object->component<ShadowMap::ShadowMapComponent>().m_precision];
		const GLenum layout = s_layouts[numComponents];

		// Early out if we have the same shadow map setup
		if (object->component<ShadowMap::ShadowMapComponent>().m_shadowMapDimensions == shadowMapDimensions &&
			scene.m_textures[object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO].m_format == format &&
			scene.m_textures[object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO].m_layout == layout)
			return;

		// Store the updated shadow map size
		object->component<ShadowMap::ShadowMapComponent>().m_shadowMapDimensions = shadowMapDimensions;

		// Generate the framebuffer
		Scene::createTexture(scene, object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO, GL_TEXTURE_2D, 
			shadowMapDimensions.x, shadowMapDimensions.y, format, layout,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_DEPTH_COMPONENT16);
		
		// Generate the temporary blur buffer
		Scene::createTexture(scene, object->component<ShadowMap::ShadowMapComponent>().m_shadowMapBlurFBO, GL_TEXTURE_2D,
			shadowMapDimensions.x, shadowMapDimensions.y, 1, format, layout,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

		// Re-generate the blur kernels
		updateBlurKernels(scene, object);

		// Update the slice texture parameters
		for (size_t i = 0; i < object->component<ShadowMap::ShadowMapComponent>().m_slices.size(); ++i)
		{
			auto& slice = object->component<ShadowMap::ShadowMapComponent>().m_slices[i];
			const float texelOffset = 0.5f;
			slice.m_textureOffset = (glm::vec2(slice.m_startCoords) + texelOffset) / glm::vec2(shadowMapDimensions);
			slice.m_textureScale = (glm::vec2(object->component<ShadowMap::ShadowMapComponent>().m_resolution) - 2.0f * texelOffset) / glm::vec2(shadowMapDimensions);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Scene::Object*> getShadowCasters(Scene::Scene& scene)
	{
		return Scene::filterObjects(scene, Scene::OBJECT_TYPE_SHADOW_CASTER, [](Scene::Object* object)
		{
			return object->component<ShadowMap::ShadowMapComponent>().m_castsShadow;
		}, false, false);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateBlurKernels(Scene::Scene& scene, Scene::Object* object)
	{
		RenderSettings::generateGaussianKernel(scene,
			ShadowMap::getBlurSigma(scene, object),
			object->component<ShadowMap::ShadowMapComponent>().m_blurKernelDiscrete,
			object->component<ShadowMap::ShadowMapComponent>().m_blurKernelLinear);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getBlurSigma(Scene::Scene& scene, Scene::Object* object)
	{
		// Length of the shadow frustum
		const float near = object->component<ShadowMap::ShadowMapComponent>().m_slices[0].m_transform.m_near;
		const float far = object->component<ShadowMap::ShadowMapComponent>().m_slices[0].m_transform.m_far;
		const float length = far - near;

		// Length of a single shadow map texel
		auto resolution = object->component<ShadowMap::ShadowMapComponent>().m_resolution;
		const float texelLength = length / resolution;

		// Look for the first blur sigma that matches the desired blur strength
		static const float s_sigmaMin = 0.1f;
		static const float s_sigmaMax = 10.0f;
		static const float s_sigmaStep = 0.1f;
		for (float sigma = s_sigmaMin; sigma < s_sigmaMax; sigma += s_sigmaStep)
		{
			const int numTaps = RenderSettings::getGaussianBlurTaps(sigma);
			const float blurLength = numTaps * texelLength;
			if (blurLength > object->component<ShadowMap::ShadowMapComponent>().m_blurStrength) return sigma;
		}

		// Return the highest blur sigma
		return s_sigmaMax;
	}

	////////////////////////////////////////////////////////////////////////////////
	int getBlurKernelDiameter(Scene::Scene& scene, Scene::Object* object)
	{
		return selectBlurKernel(scene, object).size();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string selectBlurShader(Scene::Scene& scene, Scene::Object* object)
	{
		static std::unordered_map<ShadowMap::ShadowMapComponent::ShadowMapAlgorithm, std::string> s_blurShaders =
		{
			{ ShadowMap::ShadowMapComponent::Basic, "gaussian_blur" },
			{ ShadowMap::ShadowMapComponent::Variance, "gaussian_blur" },
			{ ShadowMap::ShadowMapComponent::Exponential, "gaussian_blur_log" },
			{ ShadowMap::ShadowMapComponent::ExponentialVariance, "gaussian_blur" },
			{ ShadowMap::ShadowMapComponent::Moments, "gaussian_blur" },
		};
		return s_blurShaders[object->component<ShadowMap::ShadowMapComponent>().m_algorithm] + "_" + std::to_string(getBlurKernelDiameter(scene, object));
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<glm::vec2> const& selectBlurKernel(Scene::Scene& scene, Scene::Object* object)
	{
		// Basic and variance SM's are filtered in linear space; they can use the linear kernel
		if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Basic ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Variance ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::ExponentialVariance ||
			object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Moments)
		{
			return object->component<ShadowMap::ShadowMapComponent>().m_blurKernelLinear;
		}

		// Exponential SM's are filtered in log space; must resort to the discrete kernel
		else if (object->component<ShadowMap::ShadowMapComponent>().m_algorithm == ShadowMap::ShadowMapComponent::Exponential)
		{
			return object->component<ShadowMap::ShadowMapComponent>().m_blurKernelDiscrete;
		}

		// Fall back to the discrete kernel
		return object->component<ShadowMap::ShadowMapComponent>().m_blurKernelDiscrete;
	}
}