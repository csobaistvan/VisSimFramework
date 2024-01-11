#include "PCH.h"
#include "SpotLight.h"

namespace SpotLight
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(SPOT_LIGHT_GRID);
	DEFINE_OBJECT(SPOT_LIGHT_GRID);
	REGISTER_OBJECT_UPDATE_CALLBACK(SPOT_LIGHT_GRID, AFTER, RENDER_SETTINGS);
	REGISTER_OBJECT_RENDER_CALLBACK(SPOT_LIGHT_GRID, "Voxel Lighting [Spot Light Grid]", OpenGL, AFTER, "Voxel Lighting [Begin]", 1, &SpotLight::voxelLightingOpengl, &SpotLight::voxelLightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, SpotLight::voxelLightingBeginOpenGL, SpotLight::voxelLightingEndOpenGL);
	REGISTER_OBJECT_RENDER_CALLBACK(SPOT_LIGHT_GRID, "Lighting [Spot Light Grid]", OpenGL, AFTER, "Lighting [Begin]", 1, &SpotLight::lightingOpenGL, &SpotLight::lightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, SpotLight::lightingBeginOpenGL, SpotLight::lightingEndOpenGL);

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_defines =
		{
			"LIGHT_TYPE SPOT",
			"LIGHT_SOURCES_PER_BATCH "s + std::to_string(SpotLight::LIGHT_SOURCES_PER_BATCH),
			"VOXEL_GBUFFER_TEXTURE_FORMAT " + RenderSettings::voxelGbufferGlShaderFormat(scene),
			"VOXEL_RADIANCE_TEXTURE_FORMAT " + RenderSettings::voxelRadianceGlShaderFormat(scene)
		};
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			RenderSettings::BrdfModel_meta,
			RenderSettings::VoxelDilationMethod_meta,
			RenderSettings::VoxelShadingMode_meta,
			SpotLight::SpotLightGridComponent::AttenuationMethod_meta,
			ShadowMap::ShadowMapComponent::ShadowMapAlgorithm_meta,
			ShadowMap::ShadowMapComponent::ShadowMapPrecision_meta
		);

		// Load the voxel lighting shader
		Asset::loadShader(scene, "Shading/Voxel/InjectDirectLight", "inject_direct_light", "Shading/voxel_spot_light", shaderParameters);

		// Load the deferred lighting shaders
		Asset::loadShader(scene, "Shading/Deferred/ComputeLight", "compute_light", "Shading/deferred_spot_light", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "SpotLight", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, initGPUBuffers, "Generic GPU Buffers");

		// Set the default shadow map layout
		object.component<ShadowMap::ShadowMapComponent>().m_layout = ShadowMap::ShadowMapComponent::CubeMaps;

		DelayedJobs::postJob(scene, &object, "Init Shadow Map", false, 2, [](Scene::Scene& scene, Scene::Object& object)
		{
			updateLightSources(scene, &object);
			updateShadowMapSlices(scene, &object);
			updateShadowMapTexture(scene, &object);
			ShadowMap::regenerateShadowMap(scene, &object);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Start with updating the light sources
		updateLightSources(scene, object);

		// Keep the shadow map slices updated
		updateShadowMapSlices(scene, object);
		
		// Regenerate the shadow maps if the object's transform has changed
		if (object->component<Transform::TransformComponent>().m_transformChanged)
		{
			ShadowMap::regenerateShadowMap(scene, object);
			RenderSettings::updateVoxelGridRadiance(scene);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateLightSources(Scene::Scene& scene, Scene::Object* object)
	{
		// TODO: use the model matrix

		// Compute the model matrix
		glm::mat4 const& model = Transform::getModelMatrix(object);

		// Compute the number of light sources
		auto numLightSourcesAxis = object->component<SpotLight::SpotLightGridComponent>().m_numLightSources;
		int numLightSources = std::max(1, numLightSourcesAxis.x) * std::max(1, numLightSourcesAxis.y) * std::max(1, numLightSourcesAxis.z);

		// Step value for each axis
		glm::vec3 positionStep(0.0f);
		glm::mat3 orientationStep;
		for (int i = 0; i < 3; ++i)
		{
			if (numLightSourcesAxis[i] > 1)
			{
				positionStep[i] = object->component<Transform::TransformComponent>().m_scale[i] / float(numLightSourcesAxis[i] - 1);
				orientationStep[i] = (2.0f * object->component<SpotLight::SpotLightGridComponent>().m_rotationDelta[i]) / float(numLightSourcesAxis[i] - 1);
			}
		}

		// Corner of the spot light grid
		glm::vec3 cornerPosition = object->component<Transform::TransformComponent>().m_position - positionStep * glm::vec3(numLightSourcesAxis - 1) * 0.5f;
		glm::vec3 cornerOrientation = object->component<SpotLight::SpotLightGridComponent>().m_rotation - orientationStep * glm::vec3(numLightSourcesAxis - 1) * 0.5f;

		// Resize the light source vector
		object->component<SpotLight::SpotLightGridComponent>().m_lightSources.resize(numLightSources);

		// Generate the light source coordinates
		size_t lightId = 0;
		for (int x = 0; x < numLightSourcesAxis.x; ++x)
		for (int y = 0; y < numLightSourcesAxis.y; ++y)
		for (int z = 0; z < numLightSourcesAxis.z; ++z)
		{
			auto& lightSource = object->component<SpotLight::SpotLightGridComponent>().m_lightSources[lightId++];
			glm::vec3 rotation = cornerOrientation + orientationStep * glm::vec3(lightSource.m_gridLocation);
			lightSource.m_gridLocation = glm::ivec3(x, y, z);
			lightSource.m_lightLocation = cornerPosition + glm::vec3(lightSource.m_gridLocation) * positionStep;
			lightSource.m_lightDirection = Transform::getForwardVector(Transform::getModelMatrix(lightSource.m_lightLocation, rotation, glm::vec3(1.0f)));
			lightSource.m_diffuseIntensity = object->component<SpotLight::SpotLightGridComponent>().m_diffuseIntensity;
			lightSource.m_specularItensity = object->component<SpotLight::SpotLightGridComponent>().m_specularItensity;
			lightSource.m_color = object->component<SpotLight::SpotLightGridComponent>().m_color;
			lightSource.m_radius = object->component<SpotLight::SpotLightGridComponent>().m_radius;
			lightSource.m_cosInnerAngle = glm::cos(object->component<SpotLight::SpotLightGridComponent>().m_innerAngle * 0.5f);
			lightSource.m_cosOuterAngle = glm::cos(object->component<SpotLight::SpotLightGridComponent>().m_outerAngle * 0.5f);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	ShadowMap::ShadowMapTransform lightSourceTransform(Scene::Scene& scene, Scene::Object* object, SpotLightSource const& lightSource)
	{
		// Access the render settings to extract the AABB
		auto renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Compute the up vector
		const glm::vec3 up = glm::length2(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), lightSource.m_lightDirection)) < 1e-3f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

		// Generate the result
		ShadowMap::ShadowMapTransform result;
		result.m_near = 0.01f * object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale.x + object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset.x;
		result.m_far = object->component<SpotLight::SpotLightGridComponent>().m_radius * object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale.y + +object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset.y;
		result.m_isPerspective = true;
		result.m_view = glm::lookAt(lightSource.m_lightLocation, lightSource.m_lightLocation + lightSource.m_lightDirection, up);
		result.m_projection = glm::perspective(object->component<SpotLight::SpotLightGridComponent>().m_outerAngle, 1.0f, result.m_near, result.m_far);
		result.m_transform = result.m_projection * result.m_view;
		result.m_frustum = BVH::Frustum(result.m_transform);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMapSlices(Scene::Scene& scene, Scene::Object* object)
	{
		// Start with updating the light sources
		updateLightSources(scene, object);

		// Extract the list of light sources
		auto const& lightSources = object->component<SpotLight::SpotLightGridComponent>().m_lightSources;

		// Compute the total number of light slices
		const int numTotalSlices = lightSources.size();

		// Create the corrent number of slices
		auto& slices = object->component<ShadowMap::ShadowMapComponent>().m_slices;
		slices.resize(numTotalSlices);

		// Number of horizontal and vertical slices necessary for the shadow casters
		const int numHorizontalSlices = glm::max(int(std::ceilf(std::log2(numTotalSlices))), 1);
		const int numVerticalSlices = (numTotalSlices + numHorizontalSlices - 1) / numHorizontalSlices;
		
		// Size of a single shadow map
		glm::ivec2 resolution = glm::ivec2(object->component<ShadowMap::ShadowMapComponent>().m_resolution);

		// Configure the slices
		for (size_t lightId = 0; lightId < lightSources.size(); ++lightId)
		{
			// Access the light source
			auto& lightSource = lightSources[lightId];

			// Horizontal and vertical indices of the slice
			int horId = lightId % numHorizontalSlices;
			int vertId = lightId / numHorizontalSlices;

			// Configure the slice
			auto& slice = slices[lightId];
			slice.m_startCoords = glm::ivec2(horId, vertId) * resolution;
			slice.m_extents = resolution;
			slice.m_transform = lightSourceTransform(scene, object, lightSource);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMapTexture(Scene::Scene& scene, Scene::Object* object)
	{
		// Update the internal shadow map structure
		if (object->component<ShadowMap::ShadowMapComponent>().m_castsShadow)
		{
			ShadowMap::updateShadowMap(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		
		bool recreateShadowMap = false;
		bool refreshShadowMap = false;
		bool lightingChanged = false;

		recreateShadowMap |= ImGui::DragInt3("Number of Sources", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_numLightSources), 1, 0);
		refreshShadowMap |= Transform::generateGui(scene, guiSettings, object);
		refreshShadowMap |= ImGui::DragFloatAngle3("Direction", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_rotation), 0.1f);
		refreshShadowMap |= ImGui::DragFloatAngle3("Direction Delta (X)", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_rotationDelta[0]), 0.1f);
		refreshShadowMap |= ImGui::DragFloatAngle3("Direction Delta (Y)", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_rotationDelta[1]), 0.1f);
		refreshShadowMap |= ImGui::DragFloatAngle3("Direction Delta (Z)", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_rotationDelta[2]), 0.1f);
		refreshShadowMap |= ImGui::DragFloat("Radius", &object->component<SpotLight::SpotLightGridComponent>().m_radius, 10.0f);
		refreshShadowMap |= ImGui::DragFloatAngle("Inner Angle", &object->component<SpotLight::SpotLightGridComponent>().m_innerAngle, 0.01f);
		refreshShadowMap |= ImGui::DragFloatAngle("Outer Angle", &object->component<SpotLight::SpotLightGridComponent>().m_outerAngle, 0.01f);
		lightingChanged |= ImGui::ColorEdit3("Color", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_color));
		lightingChanged |= ImGui::DragFloat("Diffuse Intensity", &object->component<SpotLight::SpotLightGridComponent>().m_diffuseIntensity, 0.01f);
		lightingChanged |= ImGui::DragFloat("Specular Intensity", &object->component<SpotLight::SpotLightGridComponent>().m_specularItensity, 0.01f);
		lightingChanged |= ImGui::Combo("Attenuation Method", &object->component<SpotLight::SpotLightGridComponent>().m_attenuationMethod, SpotLight::SpotLightGridComponent::AttenuationMethod_meta);
		lightingChanged |= ImGui::DragFloat3("Attenuation Factor", glm::value_ptr(object->component<SpotLight::SpotLightGridComponent>().m_attenuationFactor), 0.001f, 0.0f, 4.0f, "%.6f");
		auto shadowMapChanged = ShadowMap::generateGui(scene, guiSettings, object);
		recreateShadowMap |= shadowMapChanged[0];
		refreshShadowMap |= shadowMapChanged[1];

		if (recreateShadowMap || refreshShadowMap) updateShadowMapSlices(scene, object);
		if (recreateShadowMap) updateShadowMapTexture(scene, object);
		if (recreateShadowMap || refreshShadowMap) ShadowMap::regenerateShadowMap(scene, object);
		if (lightingChanged || recreateShadowMap || refreshShadowMap) RenderSettings::updateVoxelGridRadiance(scene);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool castsShadow(Scene::Object* renderSettings, Scene::Object* object)
	{
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_shadowMethod == RenderSettings::ShadowMapping &&
			object->component<ShadowMap::ShadowMapComponent>().m_castsShadow &&
			!object->component<ShadowMap::ShadowMapComponent>().m_slices.empty();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<UniformData> getLightBatches(Scene::Scene& scene, Scene::Object* renderSettings, Scene::Object* object)
	{
		std::vector<UniformData> result;

		// Size of the shadow map
		const glm::vec2 shadowMapSize = glm::vec2(object->component<ShadowMap::ShadowMapComponent>().m_shadowMapDimensions);

		// Uniform structure for holding the light source info
		SpotLight::UniformData batchData;
		batchData.m_numSources = 0;
		batchData.m_attenuationMethod = object->component<SpotLight::SpotLightGridComponent>().m_attenuationMethod;
		batchData.m_attenuation = glm::vec4(object->component<SpotLight::SpotLightGridComponent>().m_attenuationFactor, 0.0f);
		batchData.m_castsShadow = castsShadow(renderSettings, object) ? 1.0f : 0.0f;
		if (castsShadow(renderSettings, object))
		{
			batchData.m_shadowAlgorithm = object->component<ShadowMap::ShadowMapComponent>().m_algorithm;
			batchData.m_shadowPrecision = object->component<ShadowMap::ShadowMapComponent>().m_precision;
			batchData.m_shadowMapNear = object->component<ShadowMap::ShadowMapComponent>().m_slices[0].m_transform.m_near;
			batchData.m_shadowMapFar = object->component<ShadowMap::ShadowMapComponent>().m_slices[0].m_transform.m_far;
			batchData.m_shadowDepthBias = object->component<ShadowMap::ShadowMapComponent>().m_depthBias;
			batchData.m_shadowMinVariance = object->component<ShadowMap::ShadowMapComponent>().m_minVariance;
			batchData.m_shadowLightBleedBias = object->component<ShadowMap::ShadowMapComponent>().m_lightBleedBias;
			batchData.m_shadowMomentsBias = object->component<ShadowMap::ShadowMapComponent>().m_momentsBias;
			batchData.m_shadowExponentialConstants = object->component<ShadowMap::ShadowMapComponent>().m_exponentialConstants;
			batchData.m_shadowMapScale = object->component<ShadowMap::ShadowMapComponent>().m_slices[0].m_textureScale;
		}

		// Go through the grid
		auto const& lightSources = object->component<SpotLight::SpotLightGridComponent>().m_lightSources;
		for (size_t lightId = 0; lightId < lightSources.size(); ++lightId)
		{
			// Access the light source
			auto& lightSource = lightSources[lightId];

			// Append the light parameters
			UniformDataLightSource& lightData = batchData.m_lightSources[batchData.m_numSources];
			lightData.m_position = lightSource.m_lightLocation;
			lightData.m_direction = lightSource.m_lightDirection;
			lightData.m_color = lightSource.m_color;
			lightData.m_diffuseIntensity = lightSource.m_diffuseIntensity;
			lightData.m_specularIntensity = lightSource.m_specularItensity;
			lightData.m_radius = lightSource.m_radius;
			lightData.m_cosInnerAngle = lightSource.m_cosInnerAngle;
			lightData.m_cosOuterAngle = lightSource.m_cosOuterAngle;
			if (castsShadow(renderSettings, object))
			{
				lightData.m_lightTransform = object->component<ShadowMap::ShadowMapComponent>().m_slices[lightId].m_transform.m_transform;
				lightData.m_shadowMapOffset.v = object->component<ShadowMap::ShadowMapComponent>().m_slices[lightId].m_textureOffset;
			}

			// Increment the counter
			++batchData.m_numSources;

			// Render when full
			if (batchData.m_numSources == LIGHT_SOURCES_PER_BATCH)
			{
				result.push_back(batchData);
				batchData.m_numSources = 0;
			}
		}

		// Render the remaining sources
		if (batchData.m_numSources > 0)
		{
			result.push_back(batchData);
		}

		// Return the final result
		return result;
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
	void voxelLightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		Scene::bindShader(scene, "Shading", "voxel_spot_light");
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingOpengl(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Bind the shadow map texture
		if (object->component<ShadowMap::ShadowMapComponent>().m_castsShadow)
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_SHADOW_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO].m_texture);
		}

		// Number of work groups to cover the entire voxel grid
		unsigned numWorkGroups = (glm::ceil(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_buffers.m_numVoxels / 8.0f));

		// Construct light batches
		auto const& lightBatches = getLightBatches(scene, renderSettings, object);
		for (auto const& lightData : lightBatches)
		{
			// Upload the parameters
			uploadBufferData(scene, "SpotLight", lightData);

			// inject radiance at level 0 of texture
			glDispatchCompute(numWorkGroups, numWorkGroups, numWorkGroups);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool lightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName) &&
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_lighting.m_lightingMode != RenderSettings::IndirectOnly;
	}

	////////////////////////////////////////////////////////////////////////////////
	void lightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		RenderSettings::bindShaderMsaaVariant(scene, renderSettings, "Shading", "deferred_spot_light");
	}

	////////////////////////////////////////////////////////////////////////////////
	void lightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void lightingOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Bind the shadow map texture
		if (object->component<ShadowMap::ShadowMapComponent>().m_castsShadow)
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_SHADOW_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO].m_texture);
		}

		// Construct light batches
		auto const& lightBatches = getLightBatches(scene, renderSettings, object);
		for (auto const& lightData : lightBatches)
		{
			// Upload the parameters
			uploadBufferData(scene, "SpotLight", lightData);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}
	}
}