#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

namespace ShadowMap
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "ShadowMap";
	static constexpr const char* DISPLAY_NAME = "Shadow Caster";
	static constexpr const char* CATEGORY = "Lighting";

	////////////////////////////////////////////////////////////////////////////////
	/** Represents the transformation properties for a shadow map slice. */
	struct ShadowMapTransform
	{
		// The view matrix of the shadow map transformation
		glm::mat4 m_view;

		// The projection matrix of the shadow map transformation
		glm::mat4 m_projection;

		// The transformation matrix used to render the shadow map
		glm::mat4 m_transform;

		// View frustum of the shadow map
		BVH::Frustum m_frustum;

		// Whether the transformation contains a perspective projection or not
		bool m_isPerspective;

		// Near and far clipping planes
		float m_near;
		float m_far;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a single slice of the shadow map. */
	struct ShadowMapSlice
	{
		// Lower left coords of the starting location
		glm::ivec2 m_startCoords;

		// Extents of the shadow map
		glm::ivec2 m_extents;

		// Transformation properties to use for the shadow map
		ShadowMapTransform m_transform;

		// ---- Private members

		glm::vec2 m_textureOffset;
		glm::vec2 m_textureScale;

		// Whether the slice needs to be updated or not
		bool m_needsUpdate;

		// Frame in which the slice was last rendered
		int m_lastUpdated;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A shadow map component. */
	struct ShadowMapComponent
	{
		// Shadow map precision
		meta_enum(UpdatePolicy, int, Cached, Dynamic);

		// Shadow map precision
		meta_enum(ShadowMapPrecision, int, F16, F32);

		// Shadow map precision
		meta_enum(ShadowMapAlgorithm, int, Basic, Variance, Exponential, ExponentialVariance, Moments);

		// Shadow map precision
		meta_enum(ShadowMapLayout, int, Traditional, DualParaboloid, CubeMaps, Cascaded);

		// Whether the light source should cast shadows or not
		bool m_castsShadow = false;

		// Resolution of a single slice
		int m_resolution = 1024;

		// How often to update the shadow map
		UpdatePolicy m_updatePolicy = Cached;

		// Which precision to use
		ShadowMapPrecision m_precision = F32;
		
		// Which shadow map algorithm to use
		ShadowMapAlgorithm m_algorithm = Moments;

		// Near-far plane offset
		glm::vec2 m_clipPlaneOffset = { 1.0f, 0.0f };

		// Near-far plane scale factor
		glm::vec2 m_clipPlaneScale = { 1.0f, 1.0f };

		// Blur strength
		float m_blurStrength = 1.0f;

		// Depth bias
		float m_depthBias = 0.5f;

		// Small offset for the variance
		float m_minVariance = 0.05f;

		// Light bleeding correction factor
		float m_lightBleedBias = 0.1f;

		// Bias for the moments
		float m_momentsBias = 0.001f;

		// ESM constant term
		glm::vec2 m_exponentialConstants = glm::vec2(42.0f, 10.0f);

		// Polygon offsets
		float m_polygonOffsetConstant = 4.0f;
		float m_polygonOffsetLinear = 1.0f;

		// List of materials to ignore
		std::vector<std::string> m_ignoreMaterials;

		// ---- Private members

		// Discrete and linear blur kernels
		std::vector<glm::vec2> m_blurKernelDiscrete;
		std::vector<glm::vec2> m_blurKernelLinear;

		// The number of slices that we need to use
		glm::ivec2 m_numSlices = { 1, 1 };

		// Layout of the shadow map
		ShadowMapLayout m_layout = Traditional;

		// Name of the shadow map FBO
		std::string m_shadowMapFBO;

		// Name of the temporary blur target
		std::string m_shadowMapBlurFBO;

		// Properties of the various shadow map slices
		std::vector<ShadowMapSlice> m_slices;

		// Size of the built shadow map
		glm::ivec2 m_shadowMapDimensions;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::array<bool, 2> generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool blurShadowMapsObjectConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void blurShadowMapsOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseShadowMap(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void regenerateShadowMap(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMap(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Scene::Object*> getShadowCasters(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void updateBlurKernels(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	float getBlurSigma(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	int getBlurKernelDiameter(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	std::string selectBlurShader(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<glm::vec2> const& selectBlurKernel(Scene::Scene& scene, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(SHADOW_MAP, ShadowMapComponent, ShadowMap::ShadowMapComponent)
DECLARE_OBJECT(SHADOW_CASTER, COMPONENT_ID_SHADOW_MAP, COMPONENT_ID_TRANSFORM, COMPONENT_ID_EDITOR_SETTINGS)