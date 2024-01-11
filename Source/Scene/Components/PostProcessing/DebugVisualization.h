#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace DebugVisualization
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "DebugVisualization";
	static constexpr const char* DISPLAY_NAME = "Debug Visualization";
	static constexpr const char* CATEGORY = "Post Processing";

	////////////////////////////////////////////////////////////////////////////////
	// Various visualization types
	meta_enum(VisualizationTarget, int, GBuffer, VoxelGrid);

	// Various voxel component types
	meta_enum(BufferComponent, int, Albedo, Normal, Depth, Metallic, Roughness, Specular, Velocity, Radiance, Occlusion);

	////////////////////////////////////////////////////////////////////////////////
	// Which face of the voxel to display
	meta_enum(VoxelFace, int, Isotropic, AnisotropicXNeg, AnisotropicXPos, AnisotropicYNeg, AnisotropicYPos, AnisotropicZNeg, AnisotropicZPos);

    ////////////////////////////////////////////////////////////////////////////////
	/** A debug visualiser component. */
	struct DebugVisualizationComponent
	{
		// Which component to visualize
		VisualizationTarget m_visualizationTarget;

		// Which component of the gbuffer to visualize
		BufferComponent m_gbufferComponent = BufferComponent::Albedo;

		// Which component of the gbuffer to visualize
		BufferComponent m_voxelComponent = BufferComponent::Albedo;

		// Voxel face to show
		VoxelFace m_voxelFace = Isotropic;

		// Which layer to visualize
		int m_layer = 0;

		// Lod level to visualize
		int m_lodLevel = 0;

		// Display power value
		float m_displayPower = 1.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer layout for GBuffer visualization. */
	struct UniformDataGBuffer
	{
		GLint m_component;
		GLint m_layer;
		GLint m_lodLevel;
		GLfloat m_displayPower;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer layout. */
	struct UniformDataVoxelGrid
	{
		GLint m_component;
		GLint m_lodLevel;
		GLint m_voxelFace;
		GLfloat m_displayPower;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool visualizeGbufferObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void visualizeGBufferOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool visualizeVoxelGridObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void visualizeVoxelGridOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(DEBUG_VISUALIZATION, DebugVisualizationComponent, DebugVisualization::DebugVisualizationComponent)
DECLARE_OBJECT(DEBUG_VISUALIZATION, COMPONENT_ID_DEBUG_VISUALIZATION, COMPONENT_ID_EDITOR_SETTINGS)