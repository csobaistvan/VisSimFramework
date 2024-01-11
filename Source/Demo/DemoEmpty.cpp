#include "PCH.h"
#include "Demo.h"

namespace DemoEmpty
{
	////////////////////////////////////////////////////////////////////////////////
	// @CONSOLE_VAR(Scene, Object Groups, -object_group, Scene_Empty)
	static const std::string SCENE_NAME = "Empty";
	static const std::string OBJECT_GROUP_NAME = "Scene_" + SCENE_NAME;

	////////////////////////////////////////////////////////////////////////////////
	void initDemoScene(Scene::Scene& scene)
	{
		// Extract some of the used objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cameras
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, EmptyCamera)
		auto& camera = createObject(scene, "EmptyCamera", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(0.0f, 0.0f, 0.0f);
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// LIGHTING
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the main light source
		auto& directionalLight = createObject(scene, "EmptyLightSource", Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_orientation = glm::vec3(glm::radians(-25.0f), glm::radians(20.0f), glm::radians(0.0f));
			object.component<DirectionalLight::DirectionalLightComponent>().m_color = glm::vec3(1.0f);
			object.component<DirectionalLight::DirectionalLightComponent>().m_ambientIntensity = 0.0f;
			object.component<DirectionalLight::DirectionalLightComponent>().m_diffuseIntensity = 5.0f;
			object.component<DirectionalLight::DirectionalLightComponent>().m_specularItensity = 5.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = false;
			object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution();
			object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant = 4.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear = 1.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_precision = ShadowMap::ShadowMapComponent::F32;
			object.component<ShadowMap::ShadowMapComponent>().m_algorithm = ShadowMap::ShadowMapComponent::Moments;
			object.component<ShadowMap::ShadowMapComponent>().m_depthBias = 0.5f;
			object.component<ShadowMap::ShadowMapComponent>().m_momentsBias = 0.001f;
			object.component<ShadowMap::ShadowMapComponent>().m_minVariance = 0.01f;
			object.component<ShadowMap::ShadowMapComponent>().m_lightBleedBias = 0.1f;
			object.component<ShadowMap::ShadowMapComponent>().m_exponentialConstants = glm::vec2(50.0f, 10.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 5.0f;
		}));

		// Disable the voxel GI for this scene
		auto voxelGI = Scene::findFirstObject(scene, OBJECT_GROUP_NAME, Scene::OBJECT_TYPE_VOXEL_GLOBAL_ILLUMINATION);
		if (voxelGI) voxelGI->m_enabled = false;
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		REGISTER_DEMO_SCENE();
	};
}