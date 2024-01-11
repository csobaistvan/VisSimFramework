#include "PCH.h"
#include "Demo.h"

namespace DemoSibenik
{
	////////////////////////////////////////////////////////////////////////////////
	// @CONSOLE_VAR(Scene, Object Groups, -object_group, Scene_Sibenik)
	static const std::string SCENE_NAME = "Sibenik";
	static const std::string OBJECT_GROUP_NAME = "Scene_" + SCENE_NAME;

	////////////////////////////////////////////////////////////////////////////////
	static const float SCENE_SCALE = 50.0f;

	////////////////////////////////////////////////////////////////////////////////
	// Common settings for cameras
	void commonPointLightParameters(Scene::Object& object)
	{
		object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
		object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution() / 16;
		object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset = glm::vec2(5.0f, 0.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale = glm::vec2(1.0f, 1.0f);
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
	void initDemoScene(Scene::Scene& scene)
	{
		// Extract some of the used objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// MESHES
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the sibenik object.
		auto& sibenik = createObject(scene, "Sibenik", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Mesh::MeshComponent>().m_meshName = "sibenik.obj";
			object.component<Transform::TransformComponent>().m_position = glm::vec3(0.0f, 750.0f, 0.0f);
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(50.0f);
			// Fix materials
			DelayedJobs::postJob(scene, &object, "Backfacing Materials Fixup", false, 3, [](Scene::Scene& scene, Scene::Object& object)
			{
				// Fix backfacing materials
				std::string prefix = "sibenik_";

				// Make elements glossy
				scene.m_materials[prefix + "pod"].m_roughness = 0.30f; //floor
				scene.m_materials[prefix + "pod"].m_specular = 1.0f; //floor

				// Make elements metallic
				scene.m_materials[prefix + "material_038"].m_roughness = 0.35f; // knives
				scene.m_materials[prefix + "material_038"].m_metallic = 1.0f;
				scene.m_materials[prefix + "material_280"].m_metallic = 1.0f; // coffee chairs
				scene.m_materials[prefix + "material_280"].m_roughness = 0.65f;
				scene.m_materials[prefix + "material_280"].m_specular = 1.0f;
				scene.m_materials[prefix + "material_81"].m_metallic = 1.0f; // pendant lamps
				scene.m_materials[prefix + "material_81"].m_roughness = 0.5f;
				scene.m_materials[prefix + "material_81"].m_specular = 1.0f;
				scene.m_materials[prefix + "material_76"].m_metallic = 1.0f; // pendant chains
				scene.m_materials[prefix + "material_76"].m_roughness = 0.5f;
				scene.m_materials[prefix + "material_76"].m_specular = 1.0f;
				scene.m_materials[prefix + "material_52"].m_metallic = 1.0f; // door knob
				scene.m_materials[prefix + "material_52"].m_roughness = 0.5f;
				scene.m_materials[prefix + "material_52"].m_specular = 1.0f;
				scene.m_materials[prefix + "material_77"].m_metallic = 1.0f; // chandelier light
				scene.m_materials[prefix + "material_77"].m_roughness = 0.55f;
				scene.m_materials[prefix + "material_77"].m_specular = 0.5f;
				scene.m_materials[prefix + "material_72"].m_metallic = 1.0f; // fences
				scene.m_materials[prefix + "material_72"].m_roughness = 0.8f;
				scene.m_materials[prefix + "material_72"].m_specular = 1.0f;
				scene.m_materials[prefix + "material_73"].m_metallic = 1.0f; // fences-circles
				scene.m_materials[prefix + "material_73"].m_roughness = 0.8f;
				scene.m_materials[prefix + "material_73"].m_specular = 1.0f;
			});
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cameras
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SibenikCamera)
		auto& camera = createObject(scene, "SibenikCamera", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-350.0f, 235.0f, -160.0f);
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-5.0f, 262.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, false);
		}));

		// Add the camera.
		auto& cameraOld = createObject(scene, "SibenikCameraOld", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-600.0f, 200.0f, 0.0f);
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(0.0f, -90.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, false);
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// LIGHTING
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the point light grid.
		auto& pointLight = createObject(scene, "SibenikLightGrid", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-100.0f, 320.0f, 0.0f);
			object.component<PointLight::PointLightGridComponent>().m_gridSize = glm::vec3(1300.0f, 0.0f, 0.0f);
			object.component<PointLight::PointLightGridComponent>().m_numLightSources = glm::ivec3(3, 1, 1);
			object.component<PointLight::PointLightGridComponent>().m_radius = 440.0f;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 5.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 5.0f;

			commonPointLightParameters(object);
		}));

		// Tweak the GI parameters
		auto voxelGI = Scene::findFirstObject(scene, OBJECT_GROUP_NAME, Scene::OBJECT_TYPE_VOXEL_GLOBAL_ILLUMINATION);
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		REGISTER_DEMO_SCENE();
	};
}