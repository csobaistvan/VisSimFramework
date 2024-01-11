#include "PCH.h"
#include "DirectionalLight.h"

namespace DirectionalLight
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(DIRECTIONAL_LIGHT);
	DEFINE_OBJECT(DIRECTIONAL_LIGHT);
	REGISTER_OBJECT_UPDATE_CALLBACK(DIRECTIONAL_LIGHT, AFTER, RENDER_SETTINGS);
	REGISTER_OBJECT_RENDER_CALLBACK(DIRECTIONAL_LIGHT, "Voxel Lighting [Directional Light]", OpenGL, AFTER, "Voxel Lighting [Begin]", 1, &DirectionalLight::voxelLightingOpengl, &DirectionalLight::voxelLightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, DirectionalLight::voxelLightingBeginOpenGL, DirectionalLight::voxelLightingEndOpenGL);
	REGISTER_OBJECT_RENDER_CALLBACK(DIRECTIONAL_LIGHT, "Lighting [Directional Light]", OpenGL, AFTER, "Lighting [Begin]", 1, &DirectionalLight::lightingOpenGL, &DirectionalLight::lightingTypePreConditionOpenGL, &RenderSettings::firstCallObjectCondition, DirectionalLight::lightingBeginOpenGL, DirectionalLight::lightingEndOpenGL);

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_defines =
		{
			"LIGHT_TYPE DIRECTIONAL",
			"VOXEL_GBUFFER_TEXTURE_FORMAT " + RenderSettings::voxelGbufferGlShaderFormat(scene),
			"VOXEL_RADIANCE_TEXTURE_FORMAT " + RenderSettings::voxelRadianceGlShaderFormat(scene)
		};
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			RenderSettings::BrdfModel_meta,
			RenderSettings::VoxelDilationMethod_meta,
			RenderSettings::VoxelShadingMode_meta,
			ShadowMap::ShadowMapComponent::ShadowMapAlgorithm_meta,
			ShadowMap::ShadowMapComponent::ShadowMapPrecision_meta
		);

		// Load the voxel lighting shader
		Asset::loadShader(scene, "Shading/Voxel/InjectDirectLight", "inject_direct_light", "Shading/voxel_directional_light", shaderParameters);

		// Load the deferred lighting shaders
		RenderSettings::loadShaderMsaaVariants(scene, "Shading/Deferred/ComputeLight", "compute_light", "Shading/deferred_directional_light", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "DirectionalLight", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, initGPUBuffers, "Generic GPU Buffers");

		// Set the default shadow map layout
		object.component<ShadowMap::ShadowMapComponent>().m_layout = ShadowMap::ShadowMapComponent::Traditional;

		DelayedJobs::postJob(scene, &object, "Init Shadow Map", false, 2, [](Scene::Scene& scene, Scene::Object& object)
		{
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
	glm::vec3 lightSourceDirection(Scene::Scene& scene, Scene::Object* object)
	{
		return Transform::getForwardVector(object);
	}

	////////////////////////////////////////////////////////////////////////////////
	ShadowMap::ShadowMapTransform lightSourceTransform(Scene::Scene& scene, Scene::Object* object)
	{
		// Access the render settings to extract the AABB
		auto renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Compute the scene AABB
		const BVH::AABB sceneAabb = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_sceneAabb;
		const glm::vec3 sceneAabbExtents = sceneAabb.getSize();
		const float sceneAabbExtent = glm::max(sceneAabbExtents.x, glm::max(sceneAabbExtents.y, sceneAabbExtents.z));

		// Calculate the light-space AABB
		const glm::vec3 direction = lightSourceDirection(scene, object);
		const glm::vec3 source = sceneAabb.closestPointTo(direction * 10.0f * -sceneAabbExtent);
		const glm::mat4 testView = glm::lookAt(source, source + direction, glm::vec3(0.0f, 1.0f, 0.0f));
		const BVH::AABB lightSpaceAabb = sceneAabb.transform(testView);
		const glm::vec3 lightSpaceAabbExtents = lightSpaceAabb.getSize();
		const float lightSpaceAabbExtent = glm::max(lightSpaceAabbExtents.x, glm::max(lightSpaceAabbExtents.y, lightSpaceAabbExtents.z));

		// Calculate the projection matrix
		const float left = lightSpaceAabb.m_min.x;
		const float right = lightSpaceAabb.m_max.x;
		const float bottom = lightSpaceAabb.m_min.y;
		const float top = lightSpaceAabb.m_max.y;
		const float near = 0.1f;
		const float far = lightSpaceAabbExtent;

		// Generate the result
		ShadowMap::ShadowMapTransform result;
		result.m_near = near * object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale.x + +object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset.x;
		result.m_far = far * object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale.y + +object->component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset.y;
		result.m_isPerspective = false;
		result.m_view = glm::lookAt(source, source + direction, glm::vec3(0.0f, 1.0f, 0.0f));
		result.m_projection = glm::ortho(left, right, bottom, top, result.m_near, result.m_far);
		result.m_transform = result.m_projection * result.m_view;
		result.m_frustum = BVH::Frustum(result.m_transform);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMapSlices(Scene::Scene& scene, Scene::Object* object)
	{
		// Create the corrent number of slices
		auto& slices = object->component<ShadowMap::ShadowMapComponent>().m_slices;
		slices.resize(1);

		// Configure the slices
		auto& slice = slices.front();
		slice.m_startCoords = glm::ivec2(0, 0);
		slice.m_extents = glm::ivec2(object->component<ShadowMap::ShadowMapComponent>().m_resolution);
		slice.m_transform = lightSourceTransform(scene, object);
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

		refreshShadowMap |= ImGui::DragFloatAngle3("Direction", glm::value_ptr(object->component<Transform::TransformComponent>().m_orientation), 0.1f);
		lightingChanged |= ImGui::ColorEdit3("Color", glm::value_ptr(object->component<DirectionalLight::DirectionalLightComponent>().m_color));
		lightingChanged |= ImGui::DragFloat("Ambient Intensity", &object->component<DirectionalLight::DirectionalLightComponent>().m_ambientIntensity, 0.01f);
		lightingChanged |= ImGui::DragFloat("Diffuse Intensity", &object->component<DirectionalLight::DirectionalLightComponent>().m_diffuseIntensity, 0.01f);
		lightingChanged |= ImGui::DragFloat("Specular Intensity", &object->component<DirectionalLight::DirectionalLightComponent>().m_specularItensity, 0.01f);
		auto shadowMapChanged = ShadowMap::generateGui(scene, guiSettings, object);
		recreateShadowMap |= shadowMapChanged[0];
		refreshShadowMap |= shadowMapChanged[1];

		if (recreateShadowMap || refreshShadowMap) updateShadowMapSlices(scene, object);
		if (recreateShadowMap) updateShadowMapTexture(scene, object);
		if (recreateShadowMap || refreshShadowMap) ShadowMap::regenerateShadowMap(scene, object);
		if (lightingChanged || recreateShadowMap || refreshShadowMap) RenderSettings::updateVoxelGridRadiance(scene);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool castsShadow(Scene::Object * renderSettings, Scene::Object * object)
	{
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_shadowMethod == RenderSettings::ShadowMapping &&
			object->component<ShadowMap::ShadowMapComponent>().m_castsShadow &&
			!object->component<ShadowMap::ShadowMapComponent>().m_slices.empty();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<UniformData> getLightBatches(Scene::Scene& scene, Scene::Object* renderSettings, Scene::Object* object)
	{
		// Uniform structure for holding the light source info
		DirectionalLight::UniformData lightData;

		// Append the light parameters
		lightData.m_direction = lightSourceDirection(scene, object);
		lightData.m_color = object->component<DirectionalLight::DirectionalLightComponent>().m_color;
		lightData.m_ambientIntensity = object->component<DirectionalLight::DirectionalLightComponent>().m_ambientIntensity;
		lightData.m_diffuseIntensity = object->component<DirectionalLight::DirectionalLightComponent>().m_diffuseIntensity;
		lightData.m_specularIntensity = object->component<DirectionalLight::DirectionalLightComponent>().m_specularItensity;
		lightData.m_castsShadow = castsShadow(renderSettings, object) ? 1.0f : 0.0f;
		if (castsShadow(renderSettings, object))
		{
			lightData.m_shadowAlgorithm = object->component<ShadowMap::ShadowMapComponent>().m_algorithm;
			lightData.m_shadowPrecision = object->component<ShadowMap::ShadowMapComponent>().m_precision;
			lightData.m_shadowDepthBias = object->component<ShadowMap::ShadowMapComponent>().m_depthBias;
			lightData.m_shadowMinVariance = object->component<ShadowMap::ShadowMapComponent>().m_minVariance;
			lightData.m_shadowLightBleedBias = object->component<ShadowMap::ShadowMapComponent>().m_lightBleedBias;
			lightData.m_shadowMomentsBias = object->component<ShadowMap::ShadowMapComponent>().m_momentsBias;
			lightData.m_shadowExponentialConstants = object->component<ShadowMap::ShadowMapComponent>().m_exponentialConstants;
			lightData.m_lightSpaceTransform = object->component<ShadowMap::ShadowMapComponent>().m_slices[0].m_transform.m_transform;
		}

		// Return the final result
		return { lightData };
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
		Scene::bindShader(scene, "Shading", "voxel_directional_light");
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
			uploadBufferData(scene, "DirectionalLight", lightData);

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
		if (RenderSettings::isUsingMsaaBuffers(scene, renderSettings))
			Scene::bindShader(scene, "Shading", "deferred_directional_light_msaa");
		else
			Scene::bindShader(scene, "Shading", "deferred_directional_light_no_msaa");
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
			uploadBufferData(scene, "DirectionalLight", lightData);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}
	}
}