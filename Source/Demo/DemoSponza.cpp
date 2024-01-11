#include "PCH.h"
#include "Demo.h"

namespace DemoSponza
{
	////////////////////////////////////////////////////////////////////////////////
	// @CONSOLE_VAR(Scene, Object Groups, -object_group, Scene_Sponza)
	static const std::string SCENE_NAME = "Sponza";
	static const std::string OBJECT_GROUP_NAME = "Scene_" + SCENE_NAME;

	////////////////////////////////////////////////////////////////////////////////
	static const float SCENE_SCALE = 1.0f;

	////////////////////////////////////////////////////////////////////////////////
	// Common settings for cameras
	void commonPointLightParameters(Scene::Object& object)
	{
		object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
		object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution() / 16;
		object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset = glm::vec2(10.0f, 0.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale = glm::vec2(1.0f, 2.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant = 4.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear = 1.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_precision = ShadowMap::ShadowMapComponent::F32;
		object.component<ShadowMap::ShadowMapComponent>().m_algorithm = ShadowMap::ShadowMapComponent::Moments;
		object.component<ShadowMap::ShadowMapComponent>().m_depthBias = 5.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_momentsBias = 0.005f;
		object.component<ShadowMap::ShadowMapComponent>().m_minVariance = 0.005f;
		object.component<ShadowMap::ShadowMapComponent>().m_lightBleedBias = 0.1f;
		object.component<ShadowMap::ShadowMapComponent>().m_exponentialConstants = glm::vec2(50.0f, 50.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 5.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Common settings for cameras
	void commonSpotLightParameters(Scene::Object& object)
	{
		object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
		object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution() / 16;
		object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset = glm::vec2(5.0f, 0.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale = glm::vec2(1.0f, 10.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_precision = ShadowMap::ShadowMapComponent::F32;
		object.component<ShadowMap::ShadowMapComponent>().m_algorithm = ShadowMap::ShadowMapComponent::Moments;
		object.component<ShadowMap::ShadowMapComponent>().m_depthBias = 5.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_momentsBias = 0.00003f;
		object.component<ShadowMap::ShadowMapComponent>().m_minVariance = 0.01f;
		object.component<ShadowMap::ShadowMapComponent>().m_lightBleedBias = 0.2f;
		object.component<ShadowMap::ShadowMapComponent>().m_exponentialConstants = glm::vec2(50.0f, 5.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 5.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initDemoScene(Scene::Scene& scene)
	{
		// Extract some of the used objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// MESHES
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the sponza object.
		auto& sponza = createObject(scene, "Sponza (OBJ)", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Mesh::MeshComponent>().m_meshName = "sponza.obj";
			object.component<Transform::TransformComponent>().m_position = glm::vec3(0.0f);
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(SCENE_SCALE);
		}));

		auto& sponzaGltf = createObject(scene, "Sponza (GLTF)", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Mesh::MeshComponent>().m_meshName = "sponza-pbr.glb";
			object.component<Transform::TransformComponent>().m_position = glm::vec3(0.0f);
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(SCENE_SCALE);

			DelayedJobs::postJob(scene, &object, "Materials Fixup", false, 3, [](Scene::Scene& scene, Scene::Object& object)
			{
				// Fix glossy vegetation
				scene.m_materials["sponza-pbr_material0"].m_specularMap = "default_specular_map";
				scene.m_materials["sponza-pbr_material0"].m_roughness = 0.95f;
				scene.m_materials["sponza-pbr_material1"].m_specularMap = "default_specular_map";
				scene.m_materials["sponza-pbr_material1"].m_roughness = 0.95f;

				// Fix backfacing materials
				scene.m_materials["sponza-pbr_material6"].m_twoSided = true;
			});
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cameras
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the main camera
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraFree)
		auto& cameraFree = createObject(scene, "SponzaCameraFree", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(1200.0f, 150.0f, -30.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::vec3(glm::radians(0.0f), glm::radians(90.0f), glm::radians(0.0f));
			Demo::commonCameraProperties(scene, object, false);
		}));

		// Add the camera on the lower level
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraLower1)
		auto& cameraLower1 = createObject(scene, "SponzaCameraLower1", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(1190.0f, 175.0f, -565.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::vec3(glm::radians(-5.0f), glm::radians(149.0f), glm::radians(0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the camera on the lower level
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraLower2)
		auto& cameraLower2 = createObject(scene, "SponzaCameraLower2", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-1023.0f, 66.0f, 137.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(0.0f, 273.5f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the camera on the upper level
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraUpper1)
		auto& cameraUpper1 = createObject(scene, "SponzaCameraUpper1", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-327.0f, 669.0f, 198.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-11.5f, 54.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the camera on the upper level
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraUpper2)
		auto& cameraUpper2 = createObject(scene, "SponzaCameraUpper2", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-1140.0f, 585.0f, -140.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-12.15f, 270.5f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the flyaround-animation camera
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraFlyAround)
		auto& cameraFlyAround = createObject(scene, "SponzaCameraFlyAround", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(1190.0f, 175.0f, -565.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::vec3(glm::radians(-5.0f), glm::radians(149.0f), glm::radians(0.0f));
			Demo::commonCameraProperties(scene, object, true);
			object.component<Camera::CameraComponent>().m_apertureMethod = Camera::CameraComponent::Physical;
		}));

		// Add the parameter-animation
		// @CONSOLE_VAR(Scene, Camera, -camera, SponzaCameraParameters)
		auto& cameraParameters = createObject(scene, "SponzaCameraParameters", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(449.0f, 513.0f, -232.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-14.0f, 104.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the parameter-animation
		auto& cameraTeaser = createObject(scene, "SponzaCameraTeaser", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-1021.0f, 83.0f, -241.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-4.0f, 250.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// LIGHTING
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the main sun light
		auto& directionalLight = createObject(scene, "SponzaSunLight", Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-155.0f, -85.0f, 0.0f));
			object.component<DirectionalLight::DirectionalLightComponent>().m_color = glm::vec3(1.0f);
			object.component<DirectionalLight::DirectionalLightComponent>().m_ambientIntensity = 0.0f;
			object.component<DirectionalLight::DirectionalLightComponent>().m_diffuseIntensity = 20.0f;
			object.component<DirectionalLight::DirectionalLightComponent>().m_specularItensity = 20.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
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
			object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 20.0f;
		}));

		// Add the point light grid of the lower level
		auto& pointLightLowerSide = createObject(scene, "SponzaLightGrid_LowerSide", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-60.0f, 155.0f, -50.0f) * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_gridSize = glm::vec3(2200.0f, 400.0f, 900.0f);
			object.component<PointLight::PointLightGridComponent>().m_numLightSources = glm::ivec3(5, 1, 2);
			object.component<PointLight::PointLightGridComponent>().m_radius = 330.0f * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 8.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 8.0f;
			commonPointLightParameters(object);
		}));

		// Add the point light grid of the lower level
		auto& pointLightLowerCenter = createObject(scene, "SponzaLightGrid_LowerCenter", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-60.0f, 75.0f, -50.0f) * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_gridSize = glm::vec3(2300.0f, 400.0f, 900.0f);
			object.component<PointLight::PointLightGridComponent>().m_numLightSources = glm::ivec3(5, 1, 1);
			object.component<PointLight::PointLightGridComponent>().m_radius = 350.0f * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 10.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 10.0f;
			commonPointLightParameters(object);
		}));

		// Add the point light grid of the upper level
		auto& pointLightUpper = createObject(scene, "SponzaLightGrid_Upper", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-50.0f, 675.0f, -50.0f) * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_gridSize = glm::vec3(1850.0f, 400.0f, 300.0f);
			object.component<PointLight::PointLightGridComponent>().m_numLightSources = glm::ivec3(3, 1, 3);
			object.component<PointLight::PointLightGridComponent>().m_radius = 700.0f * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 5.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 5.0f;
			commonPointLightParameters(object);
		}));

		// Add the small orange light sources
		auto& pointLightVase = createObject(scene, "SponzaVaseLightGrid", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-72.0f, 130.0f, -37.0f) * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_gridSize = glm::vec3(1120.0f, 0.0f, 330.0f);
			object.component<PointLight::PointLightGridComponent>().m_numLightSources = glm::ivec3(2, 1, 2);
			object.component<PointLight::PointLightGridComponent>().m_radius = 110.0f * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(0.699f, 0.411f, 0.187f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 20.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 20.0f;
			commonPointLightParameters(object);
		}));

		// Add the spot center spot light
		auto& spotLight = createObject(scene, "SponzaSpotLightGrid", Scene::OBJECT_TYPE_SPOT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-64.0f, 850.0f, -37.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(1630.0f, 0.0f, 0.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_rotation = glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_rotationDelta[0] = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_rotationDelta[1] = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_rotationDelta[2] = glm::radians(glm::vec3(-7.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_numLightSources = glm::ivec3(6, 1, 2);
			object.component<SpotLight::SpotLightGridComponent>().m_radius = 1200.0f * SCENE_SCALE;
			object.component<SpotLight::SpotLightGridComponent>().m_innerAngle = glm::radians(15.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_outerAngle = glm::radians(25.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_diffuseIntensity = 0.5f;
			object.component<SpotLight::SpotLightGridComponent>().m_specularItensity = 0.5f;
			commonSpotLightParameters(object);
		}));

		// Add the spot lights for the lions
		auto& spotLightLions = createObject(scene, "SponzaSpotLightLions", Scene::OBJECT_TYPE_SPOT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = false;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-72.0f, 400.0f, -37.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(2450.0f, 520.0f, 330.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_rotation = glm::radians(glm::vec3(-90.0f, -90.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_rotationDelta[0] = glm::radians(glm::vec3(20.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_rotationDelta[1] = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_rotationDelta[2] = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
			object.component<SpotLight::SpotLightGridComponent>().m_numLightSources = glm::ivec3(2, 1, 1);
			object.component<SpotLight::SpotLightGridComponent>().m_radius = 500.0f * SCENE_SCALE;
			object.component<SpotLight::SpotLightGridComponent>().m_innerAngle = glm::radians(40.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_outerAngle = glm::radians(75.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<SpotLight::SpotLightGridComponent>().m_diffuseIntensity = 1.0f;
			object.component<SpotLight::SpotLightGridComponent>().m_specularItensity = 1.0f;
			commonSpotLightParameters(object);
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ANIMATION
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the sponza fly-around camera animation object
		auto& cameraFlyingAnimation = createObject(scene, "Sponza Camera Animation [Flying]", Scene::OBJECT_TYPE_KEYFRAMED_ANIM,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_animFileName = "SponzaCameraFlyAround";
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_length = 5.0f;
			//object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playbackStart = 0.8f;
			//object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playbackSpeed = 1.5f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playDirection = KeyframedAnim::KeyFramedAnimComponent::Forward;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playbackType = KeyframedAnim::KeyFramedAnimComponent::Synced;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_recordPlayback = true;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_looping = false;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_generateWhileStopped = false;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_record.m_timePerFrame = 1.0f / 60.0f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_record.m_linearThreshold = 0.0f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_tracks =
			{
				// Position tracks
				{
					"Position X",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Transform::TransformComponent>().m_position.x }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},
				{
					"Position Y",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Transform::TransformComponent>().m_position.y }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},
				{
					"Position Z",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Transform::TransformComponent>().m_position.z }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},

					// Orientation tracks
					{
						"Rotation X",
						KeyframedAnim::KeyFramedAnimTrack
						{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Transform::TransformComponent>().m_orientation.x }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},
				{
					"Rotation Y",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Transform::TransformComponent>().m_orientation.y }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},
				{
					"Rotation Z",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Transform::TransformComponent>().m_orientation.z }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},
				{
					"Pupil Diameter",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Camera::CameraComponent>().m_fixedAperture }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},

				{
					"Focus Distance",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraFlyAround.component<Camera::CameraComponent>().m_focusDistance }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				}
			};
		}));

		// Add the sponza camera parameter animation object
		auto& cameraParametersAnimation = createObject(scene, "Sponza Camera Animation [Parameters]", Scene::OBJECT_TYPE_KEYFRAMED_ANIM,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_animFileName = "SponzaCameraParameters";
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_length = 5.0f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playbackSpeed = 1.0f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playDirection = KeyframedAnim::KeyFramedAnimComponent::Forward;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_playbackType = KeyframedAnim::KeyFramedAnimComponent::Synced;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_recordPlayback = true;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_looping = false;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_generateWhileStopped = false;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_record.m_timePerFrame = 1.0f / 60.0f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_record.m_linearThreshold = 0.0f;
			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_tracks =
			{
				{
					"Pupil Diameter",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraParameters.component<Camera::CameraComponent>().m_fixedAperture }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				},

				{
					"Focus Distance",
					KeyframedAnim::KeyFramedAnimTrack
					{
						// Interpolation method						
						KeyframedAnim::KeyFramedAnimTrack::Linear,

						// Variables
						{ { "Coordinate", &cameraParameters.component<Camera::CameraComponent>().m_focusDistance }, },

						// Frames
						{
							KeyframedAnim::KeyFramedAnimNode{ 0.0f, 19.0f },
							KeyframedAnim::KeyFramedAnimNode{ 1.0f, 11.0f },
						}
					}
				}
			};
		}));
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		REGISTER_DEMO_SCENE();
	};
}