#include "PCH.h"
#include "Mesh.h"
#include "Scene/Components/Rendering/Includes.h"
#include "Scene/Components/Lighting/Includes.h"

namespace Mesh
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(MESH);
	DEFINE_OBJECT(MESH);
	REGISTER_OBJECT_UPDATE_CALLBACK(MESH, AFTER, ACTOR);
	REGISTER_OBJECT_RENDER_CALLBACK(MESH, "Depth Prepass [Mesh]", OpenGL, AFTER, "Depth Prepass [Begin]", 2, &Mesh::depthPrepassOpenGL, &Mesh::depthPrepassTypePreConditionOpenGL, &Mesh::depthPrepassObjectCondition, &Mesh::depthPrepassBeginOpenGL, &Mesh::depthPrepassEndOpenGL);
	REGISTER_OBJECT_RENDER_CALLBACK(MESH, "GBuffer Basepass [Mesh]", OpenGL, AFTER, "GBuffer Basepass [Begin]", 2, &Mesh::gbufferBasePassOpenGL, &Mesh::gbufferBasePassTypePreConditionOpenGL, &Mesh::gbufferBasePassObjectCondition, &Mesh::gbufferBasePassBeginOpenGL, &Mesh::gbufferBasePassEndOpenGL);
	REGISTER_OBJECT_RENDER_CALLBACK(MESH, "Voxel Basepass [Mesh]", OpenGL, AFTER, "Voxel Basepass [Begin]", 2, &Mesh::voxelBasePassOpenGL, &Mesh::voxelBasePassTypePreConditionOpenGL, &Mesh::voxelBasePassObjectCondition, &Mesh::voxelBasePassBeginOpenGL, &Mesh::voxelBasePassEndOpenGL);
	REGISTER_OBJECT_RENDER_CALLBACK(MESH, "Shadow Maps [Mesh]", OpenGL, AFTER, "Shadow Maps [Begin]", 1, &Mesh::shadowMapOpenGL, &Mesh::shadowMapTypePreConditionOpenGL, &Mesh::shadowMapObjectCondition, &Mesh::shadowMapBeginOpenGL, &Mesh::shadowMapEndOpenGL);

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
			RenderSettings::VoxelDilationMethod_meta,
			RenderSettings::DepthPeelAlgorithm_meta,
			RenderSettings::NormalMappingMethod_meta,
			RenderSettings::DisplacementMappingMethod_meta,
			ShadowMap::ShadowMapComponent::ShadowMapPrecision_meta,
			ShadowMap::ShadowMapComponent::ShadowMapAlgorithm_meta
		);

		// Depth pre-pass
		Asset::loadShader(scene, "Mesh/DepthPrepass", "depth_prepass", "Mesh/depth_prepass", shaderParameters);

		// Shadow map pass
		Asset::loadShader(scene, "Mesh/ShadowMap", "shadow_map", "Mesh/shadow_map", shaderParameters);

		// G-buffer base pass
		Asset::loadShader(scene, "Mesh/GBufferBasepass", "gbuffer_basepass", "Mesh/gbuffer_basepass", shaderParameters);

		// Voxel grid base pass
		Asset::loadShader(scene, "Mesh/VoxelBasepass", "voxel_basepass", "Mesh/voxel_basepass", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initMeshes(Scene::Scene& scene, Scene::Object* object)
	{
		// Load the mesh if not present
		if (!isMeshValid(scene, object))
			Asset::loadMesh(scene, object->component<Mesh::MeshComponent>().m_meshName);

		// Update the material list
		if (isMeshValid(scene, object))
		{
			updateMaterialList(scene, object);
		}

		// Update the last seen mesh name
		object->component<Mesh::MeshComponent>().m_lastMeshName = object->component<Mesh::MeshComponent>().m_meshName;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Mesh, initMeshes, "Meshes");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Access the render settings object
		Scene::Object* renderSettings = findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Name of the mesh
		std::string const& meshName = object->component<Mesh::MeshComponent>().m_meshName;
		bool valid = isMeshValid(scene, object);

		// Extract the new material names
		if (valid && object->component<Mesh::MeshComponent>().m_lastMeshName != meshName)
		{
			updateMaterialList(scene, object);
		}

		// Store the new mesh name
		object->component<Mesh::MeshComponent>().m_lastMeshName = meshName;

		// Mark the voxel grid for update if transform has changed
		if (object->component<Transform::TransformComponent>().m_transformChanged)
		{
			RenderSettings::updateVoxelGrid(scene);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateMaterialList(Scene::Scene& scene, Scene::Object* object)
	{
		object->component<Mesh::MeshComponent>().m_materials.clear();
		std::transform(
			scene.m_meshes[object->component<Mesh::MeshComponent>().m_meshName].m_materials.begin(), 
			scene.m_meshes[object->component<Mesh::MeshComponent>().m_meshName].m_materials.end(),
			std::back_inserter(object->component<Mesh::MeshComponent>().m_materials),
			[](auto const& material) { return material.m_name; });
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isMeshValid(Scene::Scene& scene, Scene::Object* object)
	{
		return scene.m_meshes.find(object->component<Mesh::MeshComponent>().m_meshName) != scene.m_meshes.end() && 
			scene.m_meshes[object->component<Mesh::MeshComponent>().m_meshName].m_subMeshes.empty() == false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isAABBVisible(BVH::Frustum const& frustum, glm::mat4 const& model, BVH::AABB const& aabb)
	{
		return frustum.intersection(aabb.transform(model)) != BVH::Outside;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isMeshVisible(Scene::Scene& scene, Scene::Object* object, Scene::Object* camera)
	{
		return isAABBVisible(
			camera->component<Camera::CameraComponent>().m_viewFrustum, 
			Transform::getModelMatrix(object), 
			scene.m_meshes[object->component<Mesh::MeshComponent>().m_meshName].m_aabb);
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		Transform::generateGui(scene, guiSettings, object);
		if (ImGui::InputTextPreset("Mesh", object->component<Mesh::MeshComponent>().m_meshName, scene.m_meshes, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			Asset::loadMesh(scene, object->component<Mesh::MeshComponent>().m_meshName);
			updateMaterialList(scene, object);
		}
		ImGui::SliderFloat("Mesh To UV Mapping", &object->component<MeshComponent>().m_meshToUv, 0.0f, 1.0f);

		if (ImGui::TreeNode("Materials"))
		{
			for (size_t i = 0; i < object->component<Mesh::MeshComponent>().m_materials.size(); ++i)
			{
				ImGui::PushID(i);

				std::string label = "Material " + std::to_string(i + 1);
				ImGui::Combo(label.c_str(), object->component<Mesh::MeshComponent>().m_materials[i], scene.m_materials);
				ImGui::SameLine();
				if (ImGui::Button("Edit"))
				{
					GuiSettings::startEditingMaterial(scene, object->component<Mesh::MeshComponent>().m_materials[i]);
				}

				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	struct SubmeshFilterParams
	{
		Scene::Scene& m_scene;
		Scene::Object* m_simulationSettings;
		Scene::Object* m_renderSettings;
		Scene::Object* m_camera;
		Scene::Object* m_object;
		GPU::Mesh const& m_mesh;
		GPU::SubMesh const& m_subMesh;
		GPU::Material const& m_material;

		SubmeshFilterParams(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, Scene::Object* object,
			GPU::Mesh const& mesh, GPU::SubMesh const& subMesh, GPU::Material const& material):
			m_scene(scene),
			m_simulationSettings(simulationSettings),
			m_renderSettings(renderSettings),
			m_camera(camera),
			m_object(object),
			m_mesh(mesh),
			m_subMesh(subMesh),
			m_material(material)
		{}
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename P>
	void renderMesh(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, Scene::Object* object, P const& pred)
	{
		// Extract the mesh
		auto const& mesh = scene.m_meshes[object->component<Mesh::MeshComponent>().m_meshName];

		// Extract the submeshes and sort them by material
		std::vector<GPU::SubMesh> sortedSubMeshes = mesh.m_subMeshes;
		std::sort(sortedSubMeshes.begin(), sortedSubMeshes.end(), [](GPU::SubMesh const& a, GPU::SubMesh const& b)
		{
			return a.m_materialId < b.m_materialId;
		});

		// Index of the last used material
		size_t lastMaterialId = -1;

		// Get the initial face culling state
		GLboolean cullFace = false;
		glGetBooleanv(GL_CULL_FACE, &cullFace);

		// Render the mesh
		glBindVertexArray(mesh.m_vao);
		for (size_t submeshId = 0; submeshId < sortedSubMeshes.size(); ++submeshId)
		{
			// Extract the relevant submesh and material
			auto const& subMesh = sortedSubMeshes[submeshId];
			auto const& material = scene.m_materials[object->component<Mesh::MeshComponent>().m_materials[subMesh.m_materialId]];

			// Apply the submesh filter
			if (!pred(SubmeshFilterParams(scene, simulationSettings, renderSettings, camera, object, mesh, subMesh, material))) continue;

			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Submesh #" + std::to_string(submeshId) + " (" + subMesh.m_name + ")");

			// Upload the material uniforms
			if (subMesh.m_materialId != lastMaterialId)
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Material Uniforms");

				// Upload the material data
				auto const& material = scene.m_materials[object->component<Mesh::MeshComponent>().m_materials[subMesh.m_materialId]];

				const bool twoSided = material.m_twoSided;
				const bool hasDiffuseMap = !material.m_diffuseMap.empty() && material.m_diffuseMap != "default_diffuse_map";
				const bool hasSpecularMap = !material.m_specularMap.empty() && material.m_specularMap != "default_specular_map";
				const bool hasAlphaMap = !material.m_alphaMap.empty() && material.m_alphaMap != "default_alpha_map" &&
					renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_transparencyMethod != RenderSettings::DisableTransparency;
				const bool hasNormalMap = !material.m_normalMap.empty() && material.m_normalMap != "default_normal_map" &&
					renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_normalMapping != RenderSettings::DisableNormalMapping;
				const bool hasDisplacementMap = !material.m_displacementMap.empty() && material.m_displacementMap != "default_displacement_map";

				// Set backface culling
				if (!twoSided && cullFace)
					glEnable(GL_CULL_FACE);
				else
					glDisable(GL_CULL_FACE);

				#define BOOLF(B) (B ? 1.0f : 0.0f)

				glUniform4f(0, BOOLF(twoSided), BOOLF(hasDiffuseMap), BOOLF(hasNormalMap), BOOLF(hasSpecularMap));
				glUniform4f(1, BOOLF(hasAlphaMap), BOOLF(hasDisplacementMap), 0.0f, 0.0f);
				glUniform3fv(2, 1, glm::value_ptr(material.m_diffuse));
				glUniform3fv(3, 1, glm::value_ptr(material.m_emissive));
				glUniform1f(4, material.m_opacity);
				glUniform1f(5, material.m_metallic);
				glUniform1f(6, material.m_roughness);
				glUniform1f(7, material.m_specular);
				glUniform1f(8, material.m_normalMapStrength);
				glUniform1f(9, material.m_displacementScale);
				glUniform1ui(10, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_normalMapping);
				glUniform1ui(11, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_displacementMapping);
				glUniform4fv(12, 1, glm::value_ptr(material.m_specularMask));
				glUniform4fv(13, 1, glm::value_ptr(material.m_roughnessMask));
				glUniform4fv(14, 1, glm::value_ptr(material.m_metallicMask));

				// Bind the textures
				if (hasDiffuseMap)
				{
					glActiveTexture(GPU::TextureEnums::TEXTURE_ALBEDO_MAP_ENUM);
					glBindTexture(GL_TEXTURE_2D, scene.m_textures[material.m_diffuseMap].m_texture);
				}

				if (hasNormalMap)
				{
					glActiveTexture(GPU::TextureEnums::TEXTURE_NORMAL_MAP_ENUM);
					glBindTexture(GL_TEXTURE_2D, scene.m_textures[material.m_normalMap].m_texture);
				}

				if (hasSpecularMap)
				{
					glActiveTexture(GPU::TextureEnums::TEXTURE_SPECULAR_MAP_ENUM);
					glBindTexture(GL_TEXTURE_2D, scene.m_textures[material.m_specularMap].m_texture);
				}

				if (hasAlphaMap)
				{
					glActiveTexture(GPU::TextureEnums::TEXTURE_ALPHA_MAP_ENUM);
					glBindTexture(GL_TEXTURE_2D, scene.m_textures[material.m_alphaMap].m_texture);
				}

				if (hasDisplacementMap)
				{
					glActiveTexture(GPU::TextureEnums::TEXTURE_DISPLACEMENT_MAP_ENUM);
					glBindTexture(GL_TEXTURE_2D, scene.m_textures[material.m_displacementMap].m_texture);
				}
			}

			// Render the mesh
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Render");

				glDrawElementsBaseVertex(GL_TRIANGLES, subMesh.m_indexCount, GL_UNSIGNED_INT, (const void*)(subMesh.m_indexStartID * sizeof(GL_UNSIGNED_INT)), subMesh.m_vertexStartID);
			}

			// Update the last material id
			lastMaterialId = subMesh.m_materialId;
		}
		glBindVertexArray(0);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool depthPrepassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Early out: no depth pre-pass
		if (!renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_depthPrepass)
			return false;

		// Fall back to the default implementation
		return Mesh::gbufferBasePassTypePreConditionOpenGL(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool depthPrepassObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return Mesh::gbufferBasePassObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void depthPrepassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		Mesh::gbufferBasePassBeginOpenGL(scene, simulationSettings, renderSettings, camera, functionName);

		// Bind the corresponding shader
		Scene::bindShader(scene, "Mesh", "depth_prepass");
	}

	////////////////////////////////////////////////////////////////////////////////
	void depthPrepassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		Mesh::gbufferBasePassEndOpenGL(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool depthPrepassSubmeshFilter(SubmeshFilterParams const& params)
	{
		return 
			// Visibility test for the camera frustum
			isAABBVisible(params.m_camera->component<Camera::CameraComponent>().m_viewFrustum, Transform::getModelMatrix(params.m_object), params.m_subMesh.m_aabb) &&

			// Ignore invisible submeshes
			params.m_material.m_opacity >= 0.01f &&
		
			// Ignore non-opaque meshes
			(params.m_material.m_alphaMap.empty() || params.m_material.m_alphaMap == "default_alpha_map");
	}

	////////////////////////////////////////////////////////////////////////////////
	void depthPrepassOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Model Uniforms");

			// Upload the model uniforms
			glm::mat4 model = Transform::getModelMatrix(object);
			glm::mat4 prevModel = Transform::getPrevModelMatrix(object);
			glUniformMatrix4fv(16, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(17, 1, GL_FALSE, glm::value_ptr(prevModel));
			glUniform1f(18, object->component<MeshComponent>().m_meshToUv);
		}

		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Render");

			renderMesh(scene, simulationSettings, renderSettings, camera, object, depthPrepassSubmeshFilter);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool gbufferBasePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool gbufferBasePassObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return isMeshValid(scene, object) && isMeshVisible(scene, object, camera) &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Bind the corresponding shader
		Scene::bindShader(scene, "Mesh", "gbuffer_basepass");

		if (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_wireframeMesh)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		// Set backface culling
		if (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_backfaceCull == RenderSettings::DisableBackfaceCull)
		{
			glDisable(GL_CULL_FACE);
		}
		else
		{
			glEnable(GL_CULL_FACE);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Reset the polygon mode
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool gbufferBasePassSubmeshFilter(SubmeshFilterParams const& params)
	{
		return
			// Visibility test for the camera frustum
			isAABBVisible(params.m_camera->component<Camera::CameraComponent>().m_viewFrustum, Transform::getModelMatrix(params.m_object), params.m_subMesh.m_aabb) &&

			// Ignore invisible submeshes
			params.m_material.m_opacity >= 0.01f;
	}

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Model Uniforms");

			// Upload the model uniforms
			const glm::mat4 model = Transform::getModelMatrix(object);
			const glm::mat4 normal = Transform::getNormalMatrix(object);
			const glm::mat4 prevModel = Transform::getPrevModelMatrix(object);
			const glm::mat4 prevNormal = Transform::getPrevNormalMatrix(object);
			glUniformMatrix4fv(16, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(17, 1, GL_FALSE, glm::value_ptr(normal));
			glUniformMatrix4fv(18, 1, GL_FALSE, glm::value_ptr(prevModel));
			glUniformMatrix4fv(19, 1, GL_FALSE, glm::value_ptr(prevNormal));
			glUniform1f(20, object->component<MeshComponent>().m_meshToUv);
		}

		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Render");

			renderMesh(scene, simulationSettings, renderSettings, camera, object, gbufferBasePassSubmeshFilter);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		return renderSettings->component<RenderSettings::RenderSettingsComponent>().m_updateVoxelGbuffer && 
			RenderSettings::firstCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return isMeshValid(scene, object) && RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Bind the corresponding shader
		Scene::bindShader(scene, "Mesh", "voxel_basepass");
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassSubmeshFilter(SubmeshFilterParams const& params)
	{
		return params.m_material.m_opacity >= 0.01f;
	}

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Model Uniforms");

			// Upload the model uniforms
			glm::mat4 model = Transform::getModelMatrix(object);
			glm::mat4 normal = Transform::getNormalMatrix(object);
			glUniformMatrix4fv(16, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(17, 1, GL_FALSE, glm::value_ptr(normal));
			glUniform1f(18, object->component<MeshComponent>().m_meshToUv);
			glUniformMatrix4fv(19, 1, GL_FALSE, glm::value_ptr(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_voxelMatrices[0]));
			glUniformMatrix4fv(20, 1, GL_FALSE, glm::value_ptr(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_voxelMatrices[1]));
			glUniformMatrix4fv(21, 1, GL_FALSE, glm::value_ptr(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_voxelMatrices[2]));
			glUniformMatrix4fv(22, 1, GL_FALSE, glm::value_ptr(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_inverseVoxelMatrices[0]));
			glUniformMatrix4fv(23, 1, GL_FALSE, glm::value_ptr(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_inverseVoxelMatrices[1]));
			glUniformMatrix4fv(24, 1, GL_FALSE, glm::value_ptr(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_inverseVoxelMatrices[2]));
		}

		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Render");

			renderMesh(scene, simulationSettings, renderSettings, camera, object, voxelBasePassSubmeshFilter);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Scene::Object*>& getShadowCasters(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& functionName)
	{
		return RenderSettings::renderPayload<std::vector<Scene::Object*>>(scene, renderSettings, RenderSettings::renderPayloadCategory({ functionName.c_str(), "ShadowMap", "ShadowCasters" }), false, [&]()
		{
			return ShadowMap::getShadowCasters(scene);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* getShadowCaster(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& functionName)
	{
		return RenderSettings::renderPayload<Scene::Object*>(scene, renderSettings, RenderSettings::renderPayloadCategory({ functionName.c_str(), "ShadowMap", "ShadowCaster" }));
	}

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* setShadowCaster(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& functionName, Scene::Object* shadowCaster)
	{
		return RenderSettings::renderPayload<Scene::Object*>(scene, renderSettings, RenderSettings::renderPayloadCategory({ functionName.c_str(), "ShadowMap", "ShadowCaster" })) = shadowCaster;
	}

	////////////////////////////////////////////////////////////////////////////////
	int& getShadowCasterId(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& functionName)
	{
		return RenderSettings::renderPayload<int>(scene, renderSettings, RenderSettings::renderPayloadCategory({ functionName.c_str(), "ShadowMap", "ShadowCasterId" }), false, -1);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shadowMapTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Early condition: shadow mapping is globally disabled
		if (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_shadowMethod != RenderSettings::ShadowMapping)
			return false;

		// Get a list of all the shadow caster objects
		auto const& shadowCasters = getShadowCasters(scene, renderSettings, functionName);

		// Get a reference to the shadow caster id
		int& shadowCasterId = getShadowCasterId(scene, renderSettings, functionName);

		// Nothing to do if we ran out of shadow casters
		if (++shadowCasterId >= shadowCasters.size())
			return false;

		// Get a reference to the current shadow caster
		setShadowCaster(scene, renderSettings, functionName, shadowCasters[shadowCasterId]);

		// Continue rendering
		return RenderSettings::multiCallTypeCondition(scene, simulationSettings, renderSettings, camera, functionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shadowMapObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return isMeshValid(scene, object) && RenderSettings::multiCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Access the current shadow caster
		Scene::Object* shadowCaster = getShadowCaster(scene, renderSettings, functionName);

		// Set the OpenGL state
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);
		glEnable(GL_SCISSOR_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_CULL_FACE);
		glPolygonOffset(shadowCaster->component<ShadowMap::ShadowMapComponent>().m_polygonOffsetLinear, shadowCaster->component<ShadowMap::ShadowMapComponent>().m_polygonOffsetConstant);

		// Bind the corresponding shader
		Scene::bindShader(scene, "Mesh", "shadow_map");

		// Unbind the shadow map texture, if any
		glActiveTexture(GPU::TextureEnums::TEXTURE_SHADOW_MAP_ENUM);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Bind the target FBO
		glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures[shadowCaster->component<ShadowMap::ShadowMapComponent>().m_shadowMapFBO].m_framebuffer);
	}

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName)
	{
		// Reset some of the special state
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shadowMapSubmeshFilter(SubmeshFilterParams const& params, Scene::Object* shadowCaster, ShadowMap::ShadowMapSlice const& slice)
	{
		auto const& ignoreMaterials = shadowCaster->component<ShadowMap::ShadowMapComponent>().m_ignoreMaterials;

		return
			// Visibility test for the camera frustum
			isAABBVisible(slice.m_transform.m_frustum, Transform::getModelMatrix(params.m_object), params.m_subMesh.m_aabb) &&

			// Ignore invisible submeshes
			params.m_material.m_opacity >= 0.01f &&

			// Ignore the materials listed on the ignore list
			std::find(ignoreMaterials.begin(), ignoreMaterials.end(), params.m_material.m_name) == ignoreMaterials.end();
	}

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Access the current shadow caster
		Scene::Object* shadowCaster = getShadowCaster(scene, renderSettings, functionName);
		auto const& slices = shadowCaster->component<ShadowMap::ShadowMapComponent>().m_slices;

		Profiler::ScopedGpuPerfCounter perfCounter(scene, shadowCaster->m_name, true);

		// Upload the model uniforms
		glm::mat4 model = Transform::getModelMatrix(object);
		glUniformMatrix4fv(16, 1, GL_FALSE, glm::value_ptr(model));
		glUniform1f(17, object->component<MeshComponent>().m_meshToUv);

		// Render to each slice of the shadow map
		auto const& mesh = scene.m_meshes[object->component<Mesh::MeshComponent>().m_meshName];
		glBindVertexArray(mesh.m_vao);
		for (size_t sliceId = 0; sliceId < slices.size(); ++sliceId)
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Slice " + std::to_string(sliceId));

			// Extract the slice
			auto const& slice = slices[sliceId];
			auto const& transform = slice.m_transform;

			// Skip slices that don't need to be updated
			if (!slice.m_needsUpdate) continue;

			// Configure the shadow map viewports
			glViewport(slice.m_startCoords.x, slice.m_startCoords.y, slice.m_extents.x, slice.m_extents.y);
			glScissor(slice.m_startCoords.x, slice.m_startCoords.y, slice.m_extents.x, slice.m_extents.y);

			// Clear the slice
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Slice-specific attributes
			glUniform1ui(18, shadowCaster->component<ShadowMap::ShadowMapComponent>().m_algorithm);
			glUniform1ui(19, shadowCaster->component<ShadowMap::ShadowMapComponent>().m_precision);
			glUniform2fv(20, 1, glm::value_ptr(shadowCaster->component<ShadowMap::ShadowMapComponent>().m_exponentialConstants));

			// Upload the light transform
			glUniformMatrix4fv(21, 1, GL_FALSE, glm::value_ptr(transform.m_transform));
			glUniform1f(22, transform.m_isPerspective ? 1.0f : 0.0f);
			glUniform1f(23, transform.m_near);
			glUniform1f(24, transform.m_far);

			// Render the mesh
			renderMesh(scene, simulationSettings, renderSettings, camera, object, [&](SubmeshFilterParams const& params)
			{
				return shadowMapSubmeshFilter(params, shadowCaster, slice);
			});
		}
		glBindVertexArray(0);
	}
}