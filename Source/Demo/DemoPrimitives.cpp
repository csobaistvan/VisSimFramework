#include "PCH.h"
#include "Demo.h"

namespace DemoPrimitives
{
	////////////////////////////////////////////////////////////////////////////////
	// @CONSOLE_VAR(Scene, Object Groups, -object_group, Scene_Primitives)
	static const std::string SCENE_NAME = "Primitives";
	static const std::string OBJECT_GROUP_NAME = "Scene_" + SCENE_NAME;

	////////////////////////////////////////////////////////////////////////////////
	void initDemoScene(Scene::Scene& scene)
	{
		// Extract some of the used objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// MESHES
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the snellen E object.
		auto& letter = createObject(scene, "Primitives Letter", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([&renderSettings](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			if (Config::AttribValue("fov").get<int>() >= 50)
			{
				object.component<Transform::TransformComponent>().m_position = glm::vec3(0.0f, 100.0f, -750.0f);
			}
			else if (Config::AttribValue("fov").get<int>() >= 60)
			{
				object.component<Transform::TransformComponent>().m_position = glm::vec3(0.0f, 190.0f, -750.0f);
			}
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(220.0f);
			object.component<Mesh::MeshComponent>().m_meshName = "plane.obj";
			
			// Generate the materials
			DelayedJobs::postJob(scene, &object, "Generate Materials", false, 3, [](Scene::Scene& scene, Scene::Object& object)
			{
				Asset::loadTexture(scene, "Textures/FX/Snellen/snellen_e.png", "Textures/FX/Snellen/snellen_e.png");

				GPU::Material material;
				material.m_diffuseMap = "Textures/FX/Snellen/snellen_e.png";
				material.m_specular = 0.0f;
				material.m_roughness = 0.0f;
				material.m_metallic = 0.0f;
				scene.m_materials["Snellen_E"] = material;
				object.component<Mesh::MeshComponent>().m_materials.back() = "Snellen_E";
			});
		}));

		// Add the sphere object.
		auto& sphere = createObject(scene, "Primitives Sphere", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			if (Config::AttribValue("fov").get<int>() >= 50)
			{
				object.component<Transform::TransformComponent>().m_position = glm::vec3(14.0f, -3.5f, -30.0f);
				object.component<Transform::TransformComponent>().m_scale = glm::vec3(8.0f);
			}
			else if (Config::AttribValue("fov").get<int>() >= 60)
			{
				object.component<Transform::TransformComponent>().m_position = glm::vec3(10.5f, -3.5f, -20.0f);
				object.component<Transform::TransformComponent>().m_scale = glm::vec3(6.5f);
			}
			object.component<Mesh::MeshComponent>().m_meshName = "sphere.obj";

			// Generate the materials
			DelayedJobs::postJob(scene, &object, "Generate Materials", false, 3, [](Scene::Scene& scene, Scene::Object& object)
			{
				GPU::Material material;
				material.m_diffuse = glm::vec3(0.388f, 0.287f, 0.069f);
				material.m_roughness = 0.35f;
				material.m_metallic = 0.0f;
				scene.m_materials["PrimitiveSphere"] = material;
				object.component<Mesh::MeshComponent>().m_materials.back() = "PrimitiveSphere";
			});
		}));

		// Add the cube mesh.
		auto& cube = createObject(scene, "Primitives Cube", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			if (Config::AttribValue("fov").get<int>() >= 50)
			{
				object.component<Transform::TransformComponent>().m_position = glm::vec3(-31.0, -7.0f, -63.0f);
				object.component<Transform::TransformComponent>().m_scale = glm::vec3(11.5f);
			}
			else if (Config::AttribValue("fov").get<int>() >= 60)
			{
				object.component<Transform::TransformComponent>().m_position = glm::vec3(-30.0, -7.0f, -50.0f);
				object.component<Transform::TransformComponent>().m_scale = glm::vec3(11.0f);
			}
			object.component<Transform::TransformComponent>().m_orientation = glm::vec3(glm::radians(12.0f), glm::radians(60.0f), glm::radians(8.0f));
			object.component<Mesh::MeshComponent>().m_meshName = "cube.obj";

			// Generate the materials
			DelayedJobs::postJob(scene, &object, "Generate Materials", false, 3, [](Scene::Scene& scene, Scene::Object& object)
				{
					GPU::Material material;
					material.m_diffuse = glm::vec3(0.318f, 0.628f, 0.765f);
					material.m_roughness = 1.0f;
					material.m_metallic = 0.75f;
					scene.m_materials["PrimitiveCube"] = material;
					object.component<Mesh::MeshComponent>().m_materials.back() = "PrimitiveCube";
				});
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cameras
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, PrimitivesCamera)
		auto& camera = createObject(scene, "PrimitivesCamera", Scene::OBJECT_TYPE_CAMERA,
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
		auto& directionalLight = createObject(scene, "PrimitivesLightSource", Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT,
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