#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "Transform.h"

namespace Mesh
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "Mesh";
	static constexpr const char* DISPLAY_NAME = "Mesh";
	static constexpr const char* CATEGORY = "Actor";

	////////////////////////////////////////////////////////////////////////////////
	/** A mesh component. */
	struct MeshComponent
	{
		// Name of the mesh.
		std::string m_meshName = "sphere.obj";

		// Mesh to UV space mapping
		float m_meshToUv = 0.0f;

		// ---- Private members

		std::string m_lastMeshName;

		// List of materials to override
		std::vector<std::string> m_materials;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the model data. */
	struct UniformDataModel
	{
		// Object's model matrix.
		glm::mat4 m_model;

		// Object's and camera's combined normal matrix (i.e. transforms to view space).
		glm::mat4 m_normal;

		// Previous model matrix.
		glm::mat4 m_prevModel;

		// Previous normal matrix.
		glm::mat4 m_prevNormal;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the material data. */
	struct UniformDataMaterial
	{
		GLfloat m_hasDiffuseMap;
		GLfloat m_hasNormalMap;
		GLfloat m_hasSpecularMap;
		GLfloat m_hasAlphaMap;
		glm::vec4 m_diffuseTint;
		glm::vec4 m_specularTint;
		glm::vec4 m_emissiveColor;
		GLfloat m_opacity;
		GLfloat m_shininess;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool isMeshValid(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateMaterialList(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool depthPrepassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	bool depthPrepassObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void depthPrepassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void depthPrepassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void depthPrepassOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool gbufferBasePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	bool gbufferBasePassObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool shadowMapTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	bool shadowMapObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(MESH, MeshComponent, Mesh::MeshComponent)
DECLARE_OBJECT(MESH, COMPONENT_ID_TRANSFORM, COMPONENT_ID_MESH, COMPONENT_ID_EDITOR_SETTINGS)