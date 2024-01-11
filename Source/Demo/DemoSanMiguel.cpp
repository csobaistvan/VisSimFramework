#include "PCH.h"
#include "Demo.h"

namespace DemoSanMiguel
{
	////////////////////////////////////////////////////////////////////////////////
	// @CONSOLE_VAR(Scene, Object Groups, -object_group, Scene_SanMiguel)
	static const std::string SCENE_NAME = "SanMiguel";
	static const std::string OBJECT_GROUP_NAME = "Scene_" + SCENE_NAME;

	////////////////////////////////////////////////////////////////////////////////
	static const float SCENE_SCALE = 200.0f;

	////////////////////////////////////////////////////////////////////////////////
	// Common settings for the sun light sources
	void sunLightProperties(Scene::Object& object)
	{
		object.component<DirectionalLight::DirectionalLightComponent>().m_color = glm::vec3(1.0f);
		object.component<DirectionalLight::DirectionalLightComponent>().m_ambientIntensity = 0.0f;
		object.component<DirectionalLight::DirectionalLightComponent>().m_diffuseIntensity = 10.0f;
		object.component<DirectionalLight::DirectionalLightComponent>().m_specularItensity = 10.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
		object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution();
		object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant = 4.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear = 1.0f;
		object.component<ShadowMap::ShadowMapComponent>().m_precision = ShadowMap::ShadowMapComponent::F32;
		object.component<ShadowMap::ShadowMapComponent>().m_algorithm = ShadowMap::ShadowMapComponent::Moments;
		object.component<ShadowMap::ShadowMapComponent>().m_depthBias = 0.5f;
		object.component<ShadowMap::ShadowMapComponent>().m_momentsBias = 0.001f;
		object.component<ShadowMap::ShadowMapComponent>().m_minVariance = 0.01f;
		object.component<ShadowMap::ShadowMapComponent>().m_lightBleedBias = 0.5f;
		object.component<ShadowMap::ShadowMapComponent>().m_exponentialConstants = glm::vec2(50.0f, 10.0f);
		object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 10.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initDemoScene(Scene::Scene& scene)
	{
		// Extract some of the used objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// MESHES
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the san miguel object.
		auto& sanMiguel = createObject(scene, "San Miguel", Scene::OBJECT_TYPE_MESH,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Mesh::MeshComponent>().m_meshName = "san-miguel-low-poly.obj";
			object.component<Transform::TransformComponent>().m_position = glm::vec3(-15.0f, 0.0f, 0.0f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_scale = glm::vec3(SCENE_SCALE);

			// Fix materials
			DelayedJobs::postJob(scene, &object, "Backfacing Materials Fixup", false, 3, [](Scene::Scene& scene, Scene::Object& object)
			{
				// Material name prefix
				std::string prefix = "san-miguel-low-poly_";

				// Fix backfacing materials
				for (auto& material : scene.m_materials)
				{
					if (material.first.find(prefix) == std::string::npos) continue;

					std::string diffuseMapName = std::filesystem::path(material.second.m_diffuseMap).stem().string();
					if (diffuseMapName.find("leaf") != std::string::npos ||
						diffuseMapName.find("lef") != std::string::npos ||
						diffuseMapName.find("bark") != std::string::npos ||
						diffuseMapName.find("tronco") != std::string::npos ||
						diffuseMapName.find("petal") != std::string::npos ||
						diffuseMapName.substr(0, 2) == "l0" ||
						diffuseMapName.substr(0, 2) == "l3" ||
						diffuseMapName.substr(0, 2) == "FL" ||
						diffuseMapName.substr(0, 2) == "EU" ||
						diffuseMapName.substr(0, 2) == "BS" ||
						diffuseMapName.substr(0, 2) == "BL" ||
						diffuseMapName.substr(0, 2) == "TR" ||
						diffuseMapName.substr(0, 2) == "HP")
						material.second.m_twoSided = true;
				}

				scene.m_materials[prefix + "materialo"].m_twoSided = true;
				scene.m_materials[prefix + "material_6"].m_twoSided = true;
				scene.m_materials[prefix + "material_19"].m_twoSided = true;
				scene.m_materials[prefix + "material_33"].m_twoSided = true;
				scene.m_materials[prefix + "material_31"].m_twoSided = true;

				// Make elements glossy
				scene.m_materials[prefix + "material_041"].m_roughness = 0.14f; // transparent glasses
				scene.m_materials[prefix + "material_042"].m_roughness = 0.2f; // green glasses
				scene.m_materials[prefix + "materialn"].m_roughness = 0.11f;   // pendant lamp
				scene.m_materials[prefix + "material_79"].m_roughness = 0.2f; // chandelier light bulb
				scene.m_materials[prefix + "material_0"].m_roughness = 0.1f;  // door windows

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

		// Add the main camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraFree)
		auto& cameraFree = createObject(scene, "SanMiguelCameraFree", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(8.15f, 6.60f, -1.95f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-7.333f, -262.81f, 0.0f));
			Demo::commonCameraProperties(scene, object, false);
		}));

		// Add the upper level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraUpper1)
		auto& cameraUpper1 = createObject(scene, "SanMiguelCameraUpper1", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(8.15f, 6.60f, -1.90f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-7.333f, -262.81f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the upper level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraUpper2)
		auto& cameraUpper2 = createObject(scene, "SanMiguelCameraUpper2", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(7.58f, 6.63f, -0.67f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-12.0f, -292.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the upper level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraUpper3)
		auto& cameraUpper3 = createObject(scene, "SanMiguelCameraUpper3", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(4.42f, 6.75f, -0.67f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-4.5f, -100.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the upper level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraUpper4)
		auto& cameraUpper4 = createObject(scene, "SanMiguelCameraUpper4", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(1.86f, 7.26f, 2.48f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-11.5f, -51.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the lower level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraLower1)
		auto& cameraLower1 = createObject(scene, "SanMiguelCameraLower1", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(2.98f, 4.93f, 4.60f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-40.0f, -2.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the lower level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraLower2)
		auto& cameraLower2 = createObject(scene, "SanMiguelCameraLower2", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(-2.62f, 1.94f, -1.78f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-20.5f, -95.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the lower level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraLower3)
		auto& cameraLower3 = createObject(scene, "SanMiguelCameraLower3", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(5.90f, 1.05f, 4.25f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-8.0f, -341.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the lower level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraLower4)
		auto& cameraLower4 = createObject(scene, "SanMiguelCameraLower4", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
			{
				object.m_enabled = true;
				object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
				object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

				object.component<Transform::TransformComponent>().m_position = glm::vec3(5.86f, 1.03f, 4.275f) * SCENE_SCALE;
				object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-7.0f, 5.0f, 0.0f));
				Demo::commonCameraProperties(scene, object, true);
			}));

		// Add the lower level camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraLower5)
		auto& cameraLower5 = createObject(scene, "SanMiguelCameraLower5", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
			{
				object.m_enabled = true;
				object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
				object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

				object.component<Transform::TransformComponent>().m_position = glm::vec3(2.825f, 4.70f, 4.435f) * SCENE_SCALE;
				object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-35.0f, -2.0f, 0.0f));
				Demo::commonCameraProperties(scene, object, true);
			}));

		// Add the teaser image camera.
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraTeaser)
		auto& cameraTeaser = createObject(scene, "SanMiguelCameraTeaser", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(8.17f, 6.62f, -0.51f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-17.5f, -215.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		// Add the flyaround-animation camera
		// @CONSOLE_VAR(Scene, Camera, -camera, SanMiguelCameraFlyAround)
		auto& cameraFlyAround = createObject(scene, "SanMiguelCameraFlyAround", Scene::OBJECT_TYPE_CAMERA,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);
			object.component<EditorSettings::EditorSettingsComponent>().m_singular = true;

			object.component<Transform::TransformComponent>().m_position = glm::vec3(2.825f, 4.70f, 4.435f) * SCENE_SCALE;
			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-35.0f, -2.0f, 0.0f));
			Demo::commonCameraProperties(scene, object, true);
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// LIGHTING
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the main sun light
		auto& directionalLightLower = createObject(scene, "SanMiguelSunLight_Lower", Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-55.0f, -54.0f, 0.0f));
			sunLightProperties(object);
		}));

		// Add the main sun light
		auto& directionalLightUpper = createObject(scene, "SanMiguelSunLight_Upper", Scene::OBJECT_TYPE_DIRECTIONAL_LIGHT,
			Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<Transform::TransformComponent>().m_orientation = glm::radians(glm::vec3(-32.0f, -15.0f, 0.0f));
			sunLightProperties(object);
		}));

		// Add the pendant lights.
		auto& pointLighPendant = createObject(scene, "SanMiguelPendantLamps", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<PointLight::PointLightGridComponent>().m_gridLayout = PointLight::PointLightGridComponent::Custom;
			object.component<PointLight::PointLightGridComponent>().m_customLightPositions =
			{
				// Lower level
				//glm::vec3(-7.265f, 3.10f, -1.73f) * SCENE_SCALE,
				//glm::vec3(-3.815f, 3.10f, -1.73f) * SCENE_SCALE,
				glm::vec3(-0.355f, 3.10f, -1.73f) * SCENE_SCALE,
				glm::vec3(3.205f, 3.10f, -1.73f) * SCENE_SCALE,
				//glm::vec3(3.140f, 3.10f, -1.73f)* SCENE_SCALE,
				glm::vec3(5.755f, 3.10f, -1.73f) * SCENE_SCALE,

				// Lower stairs
				glm::vec3(10.14f, 3.10f, 1.98f)* SCENE_SCALE,

				// Upper stairs
				glm::vec3(10.14f, 9.10f, 1.98f)* SCENE_SCALE
			};
			object.component<PointLight::PointLightGridComponent>().m_radius = 6.0f * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 3.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 3.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
			object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution() / 16;
			object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant = 4.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear = 1.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset = glm::vec2(14.0f, 0.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale = glm::vec2(1.0f, 1.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_precision = ShadowMap::ShadowMapComponent::F32;
			object.component<ShadowMap::ShadowMapComponent>().m_algorithm = ShadowMap::ShadowMapComponent::Moments;
			object.component<ShadowMap::ShadowMapComponent>().m_depthBias = 0.5f;
			object.component<ShadowMap::ShadowMapComponent>().m_momentsBias = 0.003f;
			object.component<ShadowMap::ShadowMapComponent>().m_minVariance = 0.01f;
			object.component<ShadowMap::ShadowMapComponent>().m_lightBleedBias = 0.1f;
			object.component<ShadowMap::ShadowMapComponent>().m_exponentialConstants = glm::vec2(50.0f, 50.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 5.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_ignoreMaterials =
			{
				//"san-miguel-low-poly_materialn"
			};
		}));

		// Add the point light grid.
		auto& pointLightChandelier = createObject(scene, "SanMiguelChandelierLights", Scene::OBJECT_TYPE_POINT_LIGHT_GRID,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<PointLight::PointLightGridComponent>().m_gridLayout = PointLight::PointLightGridComponent::Custom;
			object.component<PointLight::PointLightGridComponent>().m_customLightPositions =
			{
				// 1st from back
				  (glm::vec3(-3.666f, 9.550f, -1.708f) + glm::vec3(-0.284f, 0.0f,  0.5480f)) * SCENE_SCALE,
				  (glm::vec3(-3.666f, 9.550f, -1.708f) + glm::vec3( 0.416f, 0.0f,  0.4380f)) * SCENE_SCALE,
				//(glm::vec3(-3.666f, 9.550f, -1.708f) + glm::vec3( 0.546f, 0.0f, -0.2720f)) * SCENE_SCALE,
				//(glm::vec3(-3.666f, 9.550f, -1.708f) + glm::vec3(-0.084f, 0.0f, -0.6010f)) * SCENE_SCALE,
				//(glm::vec3(-3.666f, 9.550f, -1.708f) + glm::vec3(-0.594f, 0.0f, -0.1120f)) * SCENE_SCALE,

				// 2nd from back
				  (glm::vec3(2.92f, 9.55f, -1.708f) + glm::vec3(-0.284f, 0.0f,  0.5480f)) * SCENE_SCALE,
				//(glm::vec3(2.92f, 9.55f, -1.708f) + glm::vec3( 0.416f, 0.0f,  0.4380f)) * SCENE_SCALE,
				//(glm::vec3(2.92f, 9.55f, -1.708f) + glm::vec3( 0.546f, 0.0f, -0.2720f)) * SCENE_SCALE,
				//(glm::vec3(2.92f, 9.55f, -1.708f) + glm::vec3(-0.084f, 0.0f, -0.6010f)) * SCENE_SCALE,
				//(glm::vec3(2.92f, 9.55f, -1.708f) + glm::vec3(-0.594f, 0.0f, -0.1120f)) * SCENE_SCALE,

				// 3rd from back
				  (glm::vec3(10.22f, 9.55f, -1.708f) + glm::vec3(-0.284f, 0.0f,  0.5480f)) * SCENE_SCALE,
				//(glm::vec3(10.22f, 9.55f, -1.708f) + glm::vec3( 0.416f, 0.0f,  0.4380f)) * SCENE_SCALE,
				//(glm::vec3(10.22f, 9.55f, -1.708f) + glm::vec3( 0.546f, 0.0f, -0.2720f)) * SCENE_SCALE,
				//(glm::vec3(10.22f, 9.55f, -1.708f) + glm::vec3(-0.084f, 0.0f, -0.6010f)) * SCENE_SCALE,
				//(glm::vec3(10.22f, 9.55f, -1.708f) + glm::vec3(-0.594f, 0.0f, -0.1120f)) * SCENE_SCALE,

				// Stairs
				  (glm::vec3(10.25f, 7.25f, 10.00f) + glm::vec3(-0.284f, 0.0f,  0.5480f)) * SCENE_SCALE,
				//(glm::vec3(10.25f, 7.25f, 10.00f) + glm::vec3( 0.416f, 0.0f,  0.4380f)) * SCENE_SCALE,
				//(glm::vec3(10.25f, 7.25f, 10.00f) + glm::vec3( 0.546f, 0.0f, -0.2720f)) * SCENE_SCALE,
				//(glm::vec3(10.25f, 7.25f, 10.00f) + glm::vec3(-0.084f, 0.0f, -0.6010f)) * SCENE_SCALE,
				//(glm::vec3(10.25f, 7.25f, 10.00f) + glm::vec3(-0.594f, 0.0f, -0.1120f)) * SCENE_SCALE,
			};

			object.component<PointLight::PointLightGridComponent>().m_radius = 8.00f * SCENE_SCALE;
			object.component<PointLight::PointLightGridComponent>().m_color = glm::vec3(1.0f);
			object.component<PointLight::PointLightGridComponent>().m_diffuseIntensity = 2.0f;
			object.component<PointLight::PointLightGridComponent>().m_specularItensity = 2.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_castsShadow = true;
			object.component<ShadowMap::ShadowMapComponent>().m_resolution = GPU::shadowMapResolution() / 16;
			object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant = 4.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear = 1.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneOffset = glm::vec2(10.0f, 0.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_clipPlaneScale = glm::vec2(1.0f, 1.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_precision = ShadowMap::ShadowMapComponent::F32;
			object.component<ShadowMap::ShadowMapComponent>().m_algorithm = ShadowMap::ShadowMapComponent::Moments;
			object.component<ShadowMap::ShadowMapComponent>().m_depthBias = 0.5f;
			object.component<ShadowMap::ShadowMapComponent>().m_momentsBias = 0.003f;
			object.component<ShadowMap::ShadowMapComponent>().m_minVariance = 0.01f;
			object.component<ShadowMap::ShadowMapComponent>().m_lightBleedBias = 0.1f;
			object.component<ShadowMap::ShadowMapComponent>().m_exponentialConstants = glm::vec2(50.0f, 50.0f);
			object.component<ShadowMap::ShadowMapComponent>().m_blurStrength = 5.0f;
			object.component<ShadowMap::ShadowMapComponent>().m_ignoreMaterials =
			{
				//"san-miguel-low-poly_material_78",
				//"san-miguel-low-poly_material_79",
			};
		}));

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ANIMATION
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Add the san miguel fly-around camera animation object
		auto& cameraFlyingAnimation = createObject(scene, "SanMiguel Camera Animation [Flying]", Scene::OBJECT_TYPE_KEYFRAMED_ANIM,
			Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, OBJECT_GROUP_NAME);

			object.component<KeyframedAnim::KeyFramedAnimComponent>().m_animFileName = "SanMiguelCameraFlyAround";
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
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		REGISTER_DEMO_SCENE();
	};
}