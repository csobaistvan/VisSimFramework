#include "PCH.h"
#include "PointLight.h"

namespace PointLight
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(POINT_LIGHT_GRID);
	DEFINE_OBJECT(POINT_LIGHT_GRID);
	REGISTER_OBJECT_UPDATE_CALLBACK(POINT_LIGHT_GRID, AFTER, RENDER_SETTINGS);
	REGISTER_OBJECT_RENDER_CALLBACK(POINT_LIGHT_GRID, "Voxel Lighting [Point Light Grid]", OpenGL, AFTER, "Voxel Lighting [Begin]", 1, &PointLight::voxelLightingOpengl, &PointLight::voxelLightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, PointLight::voxelLightingBeginOpenGL, PointLight::voxelLightingEndOpenGL);
	REGISTER_OBJECT_RENDER_CALLBACK(POINT_LIGHT_GRID, "Lighting [Point Light Grid]", OpenGL, AFTER, "Lighting [Begin]", 1, &PointLight::lightingOpenGL, &PointLight::lightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, PointLight::lightingBeginOpenGL, PointLight::lightingEndOpenGL);

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_defines =
		{
			"LIGHT_TYPE POINT",
			"LIGHT_SOURCES_PER_BATCH "s + std::to_string(PointLight::LIGHT_SOURCES_PER_BATCH),
			"VOXEL_GBUFFER_TEXTURE_FORMAT " + RenderSettings::voxelGbufferGlShaderFormat(scene),
			"VOXEL_RADIANCE_TEXTURE_FORMAT " + RenderSettings::voxelRadianceGlShaderFormat(scene)
		};
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			RenderSettings::BrdfModel_meta,
			RenderSettings::VoxelDilationMethod_meta,
			RenderSettings::VoxelShadingMode_meta,
			PointLight::PointLightGridComponent::AttenuationMethod_meta,
			ShadowMap::ShadowMapComponent::ShadowMapAlgorithm_meta,
			ShadowMap::ShadowMapComponent::ShadowMapPrecision_meta
		);

		// Load the voxel lighting shader
		Asset::loadShader(scene, "Shading/Voxel/InjectDirectLight", "inject_direct_light", "Shading/voxel_point_light", shaderParameters);

		// Load the deferred lighting shaders
		RenderSettings::loadShaderMsaaVariants(scene, "Shading/Deferred/ComputeLight", "compute_light", "Shading/deferred_point_light", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "PointLight", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
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
	void updateLightSourcesRegularGrid(Scene::Scene& scene, Scene::Object* object)
	{
		// Compute the model matrix
		glm::mat4 const& model = Transform::getModelMatrix(object);

		// Extract the light source vector
		auto& lightSources = object->component<PointLight::PointLightGridComponent>().m_lightSources;

		// Compute the number of light sources
		const glm::ivec3 numLightSourcesAxis = object->component<PointLight::PointLightGridComponent>().m_numLightSources;
		const int numLightSources = std::max(1, numLightSourcesAxis.x) * std::max(1, numLightSourcesAxis.y) * std::max(1, numLightSourcesAxis.z);
		const glm::vec3 gridSize = object->component<PointLight::PointLightGridComponent>().m_gridSize;

		// Step value and corner for each axis
		const glm::vec3 step = gridSize / glm::max(glm::vec3(numLightSourcesAxis - 1), 1.0f);
		const glm::vec3 gridCorner = -0.5f * gridSize * glm::min(glm::vec3(numLightSourcesAxis - 1), 1.0f);

		// Resize the light source vector
		lightSources.resize(numLightSources);

		// Generate the light source coordinates
		size_t lightId = 0;
		for (int x = 0; x < numLightSourcesAxis.x; ++x)
		for (int y = 0; y < numLightSourcesAxis.y; ++y)
		for (int z = 0; z < numLightSourcesAxis.z; ++z)
		{
			auto& lightSource = lightSources[lightId++];
			lightSource.m_gridLocation = glm::ivec3(x, y, z);
			lightSource.m_lightLocation = glm::vec3(model * glm::vec4(gridCorner + glm::vec3(lightSource.m_gridLocation) * step, 1.0f));
			lightSource.m_color = object->component<PointLight::PointLightGridComponent>().m_color;
			lightSource.m_diffuseIntensity = object->component<PointLight::PointLightGridComponent>().m_diffuseIntensity;
			lightSource.m_specularItensity = object->component<PointLight::PointLightGridComponent>().m_specularItensity;
			lightSource.m_radius = object->component<PointLight::PointLightGridComponent>().m_radius;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateLightSourcesCustom(Scene::Scene& scene, Scene::Object* object)
	{
		// Compute the model matrix
		glm::mat4 const& model = Transform::getModelMatrix(object);

		// Compute the number of light sources
		size_t numLightSources = object->component<PointLight::PointLightGridComponent>().m_customLightPositions.size();

		// Resize the light source vector
		object->component<PointLight::PointLightGridComponent>().m_lightSources.resize(numLightSources);

		// Generate the light source coordinates
		for (size_t lightId = 0; lightId < object->component<PointLight::PointLightGridComponent>().m_customLightPositions.size(); ++lightId)
		{
			auto& lightSource = object->component<PointLight::PointLightGridComponent>().m_lightSources[lightId];
			lightSource.m_gridLocation = glm::ivec3(0, 0, 0);
			lightSource.m_lightLocation = glm::vec3(model * glm::vec4(object->component<PointLight::PointLightGridComponent>().m_customLightPositions[lightId], 1.0f));
			lightSource.m_color = object->component<PointLight::PointLightGridComponent>().m_color;
			lightSource.m_diffuseIntensity = object->component<PointLight::PointLightGridComponent>().m_diffuseIntensity;
			lightSource.m_specularItensity = object->component<PointLight::PointLightGridComponent>().m_specularItensity;
			lightSource.m_radius = object->component<PointLight::PointLightGridComponent>().m_radius;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateLightSources(Scene::Scene& scene, Scene::Object* object)
	{
		// Regular layout
		if (object->component<PointLight::PointLightGridComponent>().m_gridLayout == PointLight::PointLightGridComponent::Regular)
			updateLightSourcesRegularGrid(scene, object);

		// Customized layout
		else if (object->component<PointLight::PointLightGridComponent>().m_gridLayout == PointLight::PointLightGridComponent::Custom)
			updateLightSourcesCustom(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Struct hold the data for the shadow map faces
	struct ShadowMapFaces
	{
		float m_fov;
		std::vector<glm::vec3> m_directions;
		std::vector<glm::vec3> m_ups;
	};

	////////////////////////////////////////////////////////////////////////////////
	// The various directions in which to render the shadow maps
	static std::unordered_map<ShadowMap::ShadowMapComponent::ShadowMapLayout, ShadowMapFaces> s_shadowMapFaces =
	{
		// Dual-paraboloid (2 slices)
		{
			ShadowMap::ShadowMapComponent::DualParaboloid,
			ShadowMapFaces
			{
				180.0f,
				{
					glm::vec3(0.0f, 0.0f, -1.0f),
					glm::vec3(0.0f, 0.0f, 1.0f),
				},
				{
					glm::vec3(0.0f, 1.0f, 0.0f),
					glm::vec3(0.0f, 1.0f, 0.0f),
				}
			}
		},

		// Cube maps (6 slices)
		{
			ShadowMap::ShadowMapComponent::CubeMaps,
			ShadowMapFaces
			{
				90.0f,
				{
					glm::vec3(-1.0f,  0.0f,  0.0f),
					glm::vec3( 1.0f,  0.0f,  0.0f),
					glm::vec3( 0.0f, -1.0f,  0.0f),
					glm::vec3( 0.0f,  1.0f,  0.0f),
					glm::vec3( 0.0f,  0.0f, -1.0f),
					glm::vec3( 0.0f,  0.0f,  1.0f),
				},
				{
					glm::vec3( 0.0f,  1.0f,  0.0f),
					glm::vec3( 0.0f,  1.0f,  0.0f),
					glm::vec3( 0.0f,  0.0f,  1.0f),
					glm::vec3( 0.0f,  0.0f,  1.0f),
					glm::vec3( 0.0f,  1.0f,  0.0f),
					glm::vec3( 0.0f,  1.0f,  0.0f),
				}
			}
		},
	};

	////////////////////////////////////////////////////////////////////////////////
	ShadowMap::ShadowMapTransform lightSourceTransform(Scene::Scene& scene, Scene::Object* object, PointLightSource const& lightSource, int resolution, float fov, glm::vec3 direction, glm::vec3 up)
	{
		// Access the render settings to extract the AABB
		auto renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Scale the FOV based on the shadow map resolution
		const int s_scaleMin = 64;
		const int s_scaleMax = 512;
		const int s_fovOffsetMin = 5.0f;
		const int s_fovOffsetMax = 30.0f;
		fov += s_fovOffsetMin + (s_fovOffsetMax - s_fovOffsetMin) * (1.0f - glm::clamp(float(resolution - s_scaleMin) / float(s_scaleMax - s_scaleMin), 0.0f, 1.0f));

		// Generate the result
		ShadowMap::ShadowMapTransform result;
		result.m_near = 0.01f * object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale.x + object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset.x;
		result.m_far = object->component<PointLight::PointLightGridComponent>().m_radius * object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale.y + +object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset.y;
		result.m_isPerspective = true;
		result.m_view = glm::lookAt(lightSource.m_lightLocation, lightSource.m_lightLocation + direction, up);
		result.m_projection = glm::perspective(glm::radians(fov), 1.0f, result.m_near, result.m_far);
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
		auto const& lightSources = object->component<PointLight::PointLightGridComponent>().m_lightSources;

		// Extract the projection directions
		ShadowMapFaces mapFaces = s_shadowMapFaces[object->component<ShadowMap::ShadowMapComponent>().m_layout];

		// Compute the total number of light slices
		const int numTotalSlices = lightSources.size() * mapFaces.m_directions.size();

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

			// Configure the slices
			for (int s = 0; s < mapFaces.m_directions.size(); ++s)
			{
				// Index of the slice
				int sliceId = lightId * mapFaces.m_directions.size() + s;

				// Horizontal and vertical indices of the slice
				int horId = sliceId % numHorizontalSlices;
				int vertId = sliceId / numHorizontalSlices;

				// Configure the slice
				auto& slice = slices[sliceId];
				slice.m_startCoords = glm::ivec2(horId, vertId) * resolution;
				slice.m_extents = resolution;
				slice.m_transform = lightSourceTransform(scene, object, lightSource, object->component<ShadowMap::ShadowMapComponent>().m_resolution, mapFaces.m_fov, mapFaces.m_directions[s], mapFaces.m_ups[s]);
			}
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

		ImGui::Combo("Grid Layout", &object->component<PointLight::PointLightGridComponent>().m_gridLayout, PointLight::PointLightGridComponent::GridLayoutMethod_meta);

		// Regular layout
		if (object->component<PointLight::PointLightGridComponent>().m_gridLayout == PointLight::PointLightGridComponent::Regular)
		{
			recreateShadowMap |= ImGui::DragInt3("Number of Sources", glm::value_ptr(object->component<PointLight::PointLightGridComponent>().m_numLightSources), 1, 0);
			refreshShadowMap |= ImGui::DragFloat3("Grid Size", glm::value_ptr(object->component<PointLight::PointLightGridComponent>().m_gridSize), 0.1f);
		}

		// Customized layout
		else if (object->component<PointLight::PointLightGridComponent>().m_gridLayout == PointLight::PointLightGridComponent::Custom)
		{
			int numSources = object->component<PointLight::PointLightGridComponent>().m_customLightPositions.size();
			recreateShadowMap |= ImGui::DragInt("Number of Sources", &numSources, 1, 0);
			if (numSources != object->component<PointLight::PointLightGridComponent>().m_customLightPositions.size())
				object->component<PointLight::PointLightGridComponent>().m_customLightPositions.resize(numSources);

			if (ImGui::TreeNode("Custom Light Sources"))
			{
				for (int i = 0; i < numSources; ++i)
				{
					std::string const& label = "Position #" + std::to_string(i + 1);
					refreshShadowMap |= ImGui::DragFloat3(label.c_str(), glm::value_ptr(object->component<PointLight::PointLightGridComponent>().m_customLightPositions[i]), 10.0f);
				}
				ImGui::TreePop();
			}
		}

		refreshShadowMap |= Transform::generateGui(scene, guiSettings, object);
		refreshShadowMap |= ImGui::DragFloat("Radius", &object->component<PointLight::PointLightGridComponent>().m_radius, 10.0f);
		lightingChanged |= ImGui::ColorEdit3("Color", glm::value_ptr(object->component<PointLight::PointLightGridComponent>().m_color));
		lightingChanged |= ImGui::DragFloat("Diffuse Intensity", &object->component<PointLight::PointLightGridComponent>().m_diffuseIntensity, 0.01f);
		lightingChanged |= ImGui::DragFloat("Specular Intensity", &object->component<PointLight::PointLightGridComponent>().m_specularItensity, 0.01f);
		lightingChanged |= ImGui::Combo("Attenuation Method", &object->component<PointLight::PointLightGridComponent>().m_attenuationMethod, PointLight::PointLightGridComponent::AttenuationMethod_meta);
		lightingChanged |= ImGui::DragFloat3("Attenuation Factor", glm::value_ptr(object->component<PointLight::PointLightGridComponent>().m_attenuationFactor), 0.001f, 0.0f, 4.0f, "%.6f");
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

		// Extract the projection directions
		ShadowMapFaces const& mapFaces = s_shadowMapFaces[object->component<ShadowMap::ShadowMapComponent>().m_layout];

		// Uniform structure for holding the light source info
		PointLight::UniformData batchData;
		batchData.m_numSources = 0;
		batchData.m_attenuationMethod = object->component<PointLight::PointLightGridComponent>().m_attenuationMethod;
		batchData.m_attenuation = glm::vec4(object->component<PointLight::PointLightGridComponent>().m_attenuationFactor, 0.0f);
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

		// Bind the shadow map texture
		if (object->component<ShadowMap::ShadowMapComponent>().m_castsShadow)
		{
			glActiveTexture(GPU::TextureEnums::TEXTURE_SHADOW_MAP_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[object->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO].m_texture);
		}

		// Go through the grid
		auto const& lightSources = object->component<PointLight::PointLightGridComponent>().m_lightSources;
		for (size_t lightId = 0; lightId < lightSources.size(); ++lightId)
		{
			// Access the light source
			auto& lightSource = lightSources[lightId];
			UniformDataLightSource& lightData = batchData.m_lightSources[batchData.m_numSources];

			// Append the common light parameters
			lightData.m_position = lightSource.m_lightLocation;
			lightData.m_color = lightSource.m_color;
			lightData.m_diffuseIntensity = lightSource.m_diffuseIntensity;
			lightData.m_specularIntensity = lightSource.m_specularItensity;
			lightData.m_radius = lightSource.m_radius;

			// Upload the shadow map parameters
			for (size_t i = 0; i < mapFaces.m_directions.size() && castsShadow(renderSettings, object); ++i)
			{
				size_t sliceId = (lightId * mapFaces.m_directions.size()) + i;
				auto const& slice = object->component<ShadowMap::ShadowMapComponent>().m_slices[sliceId];
				lightData.m_lightTransforms[i] = slice.m_transform.m_transform;
				lightData.m_shadowMapOffsets[i].v = object->component<ShadowMap::ShadowMapComponent>().m_slices[sliceId].m_textureOffset;
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

		// Render the leftover light sources, if any
		if (batchData.m_numSources > 0)
			result.push_back(batchData);

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
		Scene::bindShader(scene, "Shading", "voxel_point_light");
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
			uploadBufferData(scene, "PointLight", lightData);

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
		RenderSettings::bindShaderMsaaVariant(scene, renderSettings, "Shading", "deferred_point_light");
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
			uploadBufferData(scene, "PointLight", lightData);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}
	}
}