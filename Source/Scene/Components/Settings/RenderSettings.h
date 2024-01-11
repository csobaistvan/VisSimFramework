#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "EditorSettings.h"

namespace RenderSettings
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "RenderSettings";
	static constexpr const char* DISPLAY_NAME = "Render Settings";
	static constexpr const char* CATEGORY = "Settings";

	////////////////////////////////////////////////////////////////////////////////
	// Renderer type
	meta_enum(Renderer, int, OpenGL);

	// Shading method
	meta_enum(ShadingMethod, int, ForwardShading, DeferredShading);

	// Normal mapping mode
	meta_enum(NormalMappingMethod, int, DisableNormalMapping, NormalMapping);

	// Displacement mapping mode
	meta_enum(DisplacementMappingMethod, int, DisableDisplacementMapping, BasicParallaxMapping, SteepParallaxMapping, ParallaxOcclusionMapping);

	// Transparency handling method
	meta_enum(TransparencyMethod, int, DisableTransparency, GBuffeMaskedOnly, OrderIndependentTransparency);

	// Shadow method
	meta_enum(ShadowMethod, int, DisableShadows, ShadowMapping, VoxelShadows);

	// Backface culling mode
	meta_enum(BackfaceCullMode, int, DisableBackfaceCull, FrontLayer, BackLayers, AllLayers);

	// Various depth peeling algorithms.
	meta_enum(DepthPeelAlgorithm, int, MinimumSeparation, UmbraThresholding);

	// Blit mode
	meta_enum(BlitMode, int, NoBlit, Fullscreen, Percentage, Scaled);

	// Blit anchor
	meta_enum(BlitAnchor, int, Center, BotLeft, BotRight, TopLeft, TopRight);

	// Blit filtering
	meta_enum(TextureFiltering, int, Point, Linear);

	// BRDF to use
	meta_enum(BrdfModel, int, Phong, BlinnPhong, CookTorrance);

	// Which lighting mode to use
	meta_enum(LightingMode, int, DirectOnly, IndirectOnly, DirectAndIndirect);

	// Voxel dilation mode
	meta_enum(VoxelDilationMethod, int, NoDilation, NormalDilation);

	// Voxel shading mode
	meta_enum(VoxelShadingMode, int, NormalOnly, NormalWeighted);

	// The various texture formats
	meta_enum(TextureDataFormat, int, UI8, F8, F16, F32);

	////////////////////////////////////////////////////////////////////////////////
	/** All the resolutions available. */
	const std::vector<glm::ivec2> s_allResolutions =
	{
		glm::ivec2(7680, 4320),
		glm::ivec2(3840, 2160),
		glm::ivec2(2566, 1440),
		glm::ivec2(1920, 1080),
		glm::ivec2(1440, 900),
		glm::ivec2(1366, 768),
		glm::ivec2(1280, 720),
		glm::ivec2(960, 540),
		glm::ivec2(640, 360),
		glm::ivec2(320, 180),
		glm::ivec2(160, 90),
		glm::ivec2(80, 45),
		glm::ivec2(40, 22),
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a render payload instance with some additional helper functions. */
	struct RenderPayload
	{
		std::any m_value;
		bool m_persistent;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct RenderingSettings
	{
		// Which renderer to use.
		Renderer m_renderer;

		// The main camera for rendering
		Scene::Object* m_mainCamera = nullptr;

		// Id of the current resolution 
		int m_resolutionId = 2;

		// Whether we are in fullscreen mode or not
		bool m_fullscreen = false;

		// Blitting method to use
		BlitMode m_blitMode;

		// Blit anchor
		BlitAnchor m_blitAnchor;

		// Blit scale factor
		float m_blitScale;

		// Blit percentage
		glm::vec2 m_blitPercentage;

		// Blit filtering to use.
		TextureFiltering m_blitFiltering;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct BufferSettings
	{
		// MSAA samples
		int m_msaa = 1;

		// Whether we always use MSAA buffers or not
		bool m_forceMsaaBuffers = false;

		// Dimensions of the voxel grid
		int m_numVoxels = 128;

		// Whether we are using anisotropic voxels or not
		bool m_anisotropicVoxels = false;

		// Deferred shading gbuffer data format
		TextureDataFormat m_gbufferDataFormat = F16;

		// Voxel gbuffer data format
		TextureDataFormat m_voxelGbufferDataFormat = F8;

		// Voxel radiance data format
		TextureDataFormat m_voxelRadianceDataFormat = F16;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct UnitsSettings
	{
		// How many units is equal to one meter
		float m_unitsPerMeter = 100.0f;

		// How many nits (cd/m2) a single unit corresponds to.
		float m_nitsPerUnit = 1.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct LayerSettings
	{
		// Number of depth layers
		int m_numLayers = 1;

		// The depth peeling algorithm to use.
		DepthPeelAlgorithm m_depthPeelAlgorithm;

		// Depth gap between the individual layers.
		float m_minimumSeparationDepthGap = 1.0f;

		// Scaling factor applied to the umbra threshold.
		float m_umbraScaleFactor = 1.0f;

		// Minimum umbra value
		float m_umbraMin = 0.0f;

		// Maximum umbra value
		float m_umbraMax = FLT_MAX;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct LightingSettings
	{
		// Type of shading mode to use
		ShadingMethod m_shadingMode = DeferredShading;

		// Which lighting model to use
		BrdfModel m_brdfModel = BlinnPhong;

		// Which lighting mode to use
		LightingMode m_lightingMode = DirectAndIndirect;

		// Texture filtering during lighting
		TextureFiltering m_lightingFiltering = Linear;

		// Type of dilation to use for generating voxels
		VoxelDilationMethod m_voxelDilationMode = NoDilation;

		// Type of shading to apply to voxels
		VoxelShadingMode m_voxelShadingMode = NormalWeighted;

		// Minimum specular power
		float m_specularPowerMin = 2.0f;

		// Maximum specular power
		float m_specularPowerMax = 2048.0f;

		// Gamma used lookup
		float m_gamma = 2.2f;

		// Whether AO (ambient oclusion) affects direct lighting or not
		bool m_aoAffectsDirectLighting = true;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct Features
	{
		// What backface culling method to use
		BackfaceCullMode m_backfaceCull = DisableBackfaceCull;

		// How to handle transparent materials
		TransparencyMethod m_transparencyMethod = GBuffeMaskedOnly;

		// Which normal mapping method to use
		NormalMappingMethod m_normalMapping = NormalMapping;

		// Parallax mapping method
		DisplacementMappingMethod m_displacementMapping = DisableDisplacementMapping;

		// Whether shadows should be enabled or not
		ShadowMethod m_shadowMethod = ShadowMapping;

		// Whether depth pre-pass should be enabled or not
		bool m_depthPrepass = true;

		// Whether the world should be rendered in wireframe mode or not
		bool m_wireframeMesh = false;

		// Whether we should be renderign while in the background or not.
		bool m_backgroundRendering = false;

		// To print the aperture size or not
		bool m_showApertureSize = true;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Rendering settings component. */
	struct RenderSettingsComponent
	{
		// The various sub-settings types
		RenderingSettings m_rendering;
		BufferSettings m_buffers;
		UnitsSettings m_units;
		LayerSettings m_layers;
		LightingSettings m_lighting;
		Features m_features;

		// ---- Private members

		// List of render phaes that are disabled in the current frame
		std::set<std::string> m_disabledFunctions;

		// Whether we actually need the voxel grid for anything
		bool m_needsVoxelGrid = false;

		// Whether the voxel Gbuffer needs updating or not
		bool m_updateVoxelGbuffer = true;

		// Whether the voxel needs to be reshaded or not
		bool m_updateVoxelRadiance = true;
		
		// Voxel grid extents
		float m_voxelGridExtents = 0.0f;
		
		// List of matrices for building the voxel grid
		std::array<glm::mat4, 3> m_voxelMatrices;
		std::array<glm::mat4, 3> m_inverseVoxelMatrices;

		// Current AABB for the scene (in world space)
		BVH::AABB m_sceneAabb;

		// The list of resolutions currently available.
		std::vector<glm::ivec2> m_resolutions;

		// Names of the resolutions.
		std::vector<std::string> m_resolutionNames;

		// Render resolution
		glm::ivec2 m_resolution;

		// Current window size
		glm::ivec2 m_windowSize;

		// Id of the current read g-buffer
		int m_gbufferRead = 0;

		// Id of the current write g-buffer
		int m_gbufferWrite = 1;

		// Key-value pairs, for object intermediate storage
		std::unordered_map<std::string, RenderPayload> m_payloads;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the render settings data. */
	struct UniformData
	{
		glm::vec2 m_resolution;
		glm::vec2 m_maxResolution;
		glm::vec2 m_uvScale;
		alignas(sizeof(glm::vec4)) glm::vec3 m_worldMin;
		alignas(sizeof(glm::vec4)) glm::vec3 m_worldMax;
		GLuint m_depthPeelAlgorithm;
		GLuint m_brdfModel;
		GLuint m_lightingTextureFiltering;
		GLuint m_voxelDilationMode;
		GLuint m_voxelShadingMode;
		GLfloat m_deltaTime;
		GLfloat m_globalTime;
		GLfloat m_scaledDeltaTime;
		GLfloat m_scaledGlobalTime;
		GLfloat m_metersPerUnit;
		GLfloat m_nitsPerUnit;
		GLint m_msaa;
		GLint m_numLayers;
		GLfloat m_minimumSeparationDepthGap;
		GLfloat m_umbraScaleFactor;
		GLfloat m_umbraMin;
		GLfloat m_umbraMax;
		GLfloat m_specularPowerMin;
		GLfloat m_specularPowerMax;
		GLfloat m_gamma;
		GLuint m_numVoxels;
		GLfloat m_voxelExtents;
		GLfloat m_voxelSize;
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
	template<typename T>
	void instantiateRenderPayload(RenderPayload& payload, bool persistent, T const& defaultValue)
	{
		// Store the default value
		payload.m_value = defaultValue;

		// Store the persistence flag
		payload.m_persistent = persistent;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void instantiateRenderPayload(Scene::Scene& scene, Scene::Object* object, std::string const& category, bool persistent = false, T const& defaultValue = T())
	{
		instantiateRenderPayload(object->component<RenderSettings::RenderSettingsComponent>().m_payloads[category], persistent, defaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T& renderPayload(Scene::Scene& scene, Scene::Object* object, std::string const& category, bool persistent = false, T const& defaultValue = T())
	{
		// Property list
		auto& payloads = object->component<RenderSettings::RenderSettingsComponent>().m_payloads;

		// Create a new property if none is found
		if (payloads.find(category) == payloads.end())
		{
			instantiateRenderPayload(scene, object, category, persistent, defaultValue);
		}

		// Return a reference to the stored value
		return std::any_cast<T&>(payloads[category].m_value);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename Fn, typename = std::enable_if<std::is_trivially_callable<Fn>::value>::type>
	T& renderPayload(Scene::Scene& scene, Scene::Object* object, std::string const& category, bool persistent, Fn const& initializer)
	{
		// Property list
		auto& payloads = object->component<RenderSettings::RenderSettingsComponent>().m_payloads;

		// Create a new property if none is found
		if (payloads.find(category) == payloads.end())
		{
			instantiateRenderPayload(scene, object, category, persistent, initializer());
		}

		// Return a reference to the stored value
		return std::any_cast<T&>(payloads[category].m_value);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::optional<T> consumeRenderPayload(Scene::Scene& scene, Scene::Object* object, std::string const& category)
	{
		// Property list
		auto& payloads = object->component<RenderSettings::RenderSettingsComponent>().m_payloads;

		// Look for the property
		auto it = payloads.find(category);

		// Create a new property if none is found
		if (it == payloads.end())
			return std::optional<T>();

		// Copy the contents of the property
		auto payload = it->second;

		// Erase the property
		payloads.erase(it);

		// Return the value of the consumed property
		return std::any_cast<T>(payload.m_value);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string renderPayloadCategory(std::initializer_list<const char*> categories);

	////////////////////////////////////////////////////////////////////////////////
	// Utility functions
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	void updateMainCamera(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void updateMainCamera(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* getMainCamera(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	Scene::Object* getMainCamera(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	bool isUsingMsaaBuffers(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void bindShaderMsaaVariant(Scene::Scene& scene, Scene::Object* renderSettings, std::string const& folderName, std::string const& shaderName);

	////////////////////////////////////////////////////////////////////////////////
	void loadShaderMsaaVariants(Scene::Scene& scene, const std::string& folderName, const std::string& fileName,
		const std::string& customShaderName = "", const Asset::ShaderParameters& shaderParameters = {});

	////////////////////////////////////////////////////////////////////////////////
	int getGaussianBlurTaps(const float sigma);

	////////////////////////////////////////////////////////////////////////////////
	void generateGaussianKernel(Scene::Scene& scene, const float sigma, std::vector<glm::vec2>& discrete, std::vector<glm::vec2>& linear);

	////////////////////////////////////////////////////////////////////////////////
	void loadGaussianBlurShader(Scene::Scene& scene, int numTaps);

	////////////////////////////////////////////////////////////////////////////////
	meta_enum(GBufferId, int, GB_ReadBuffer, GB_WriteBuffer);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer. */
	void bindGbufferOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId = GB_WriteBuffer, GLenum target = GL_FRAMEBUFFER);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer layer. */
	void bindGbufferLayersOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId = GB_WriteBuffer, GBufferId bufferId = GB_WriteBuffer, GLenum target = GL_FRAMEBUFFER);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer layer. */
	void bindGbufferLayerOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId = GB_WriteBuffer, GBufferId bufferId = GB_WriteBuffer, size_t layerId = 0, GLenum target = GL_FRAMEBUFFER);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer. */
	void bindGbufferLayersImageOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId = GB_WriteBuffer, GBufferId bufferId = GB_WriteBuffer, GLuint target = 0);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding the GBuffer. */
	void bindGbufferLayerImageOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId = GB_WriteBuffer, GBufferId bufferId = GB_WriteBuffer, size_t layerId = 0, GLuint target = 0);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding GBuffer layer as a texture. */
	void bindGbufferTextureLayersOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, GBufferId gbufferId = GB_WriteBuffer, GBufferId bufferId = GB_ReadBuffer, bool respectMsaa = false);

	////////////////////////////////////////////////////////////////////////////////
	void swapGbuffers(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void swapGbufferBuffers(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void setupViewportOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, int resolutionId = -1);

	////////////////////////////////////////////////////////////////////////////////
	void setupViewportArrayOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, int resolutionId = -1);

	////////////////////////////////////////////////////////////////////////////////
	void renderFullscreenPlaneOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	float unitsToMeters(Scene::Object* renderSettings, float units);

	////////////////////////////////////////////////////////////////////////////////
	float unitsToMeters(Scene::Scene& scene, Scene::Object* renderSettings, float units);

	////////////////////////////////////////////////////////////////////////////////
	float unitsToMeters(Scene::Scene& scene, float units);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 unitsToMeters(Scene::Object* renderSettings, glm::vec3 units);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 unitsToMeters(Scene::Scene& scene, Scene::Object* renderSettings, glm::vec3 units);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 unitsToMeters(Scene::Scene& scene, glm::vec3 units);

	////////////////////////////////////////////////////////////////////////////////
	float metersToUnits(Scene::Object* renderSettings, float meters);

	////////////////////////////////////////////////////////////////////////////////
	float metersToUnits(Scene::Scene& scene, Scene::Object* renderSettings, float meters);

	////////////////////////////////////////////////////////////////////////////////
	float metersToUnits(Scene::Scene& scene, float meters);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 metersToUnits(Scene::Object* renderSettings, glm::vec3 meters);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 metersToUnits(Scene::Scene& scene, Scene::Object* renderSettings, glm::vec3 meters);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 metersToUnits(Scene::Scene& scene, glm::vec3 meters);

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolutionById(Scene::Scene& scene, Scene::Object* renderSettings, int resolutionId);

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolutionById(Scene::Scene& scene, int resolutionId);

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolution(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	glm::ivec2 getResolution(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	GLenum gbufferGlTextureFormat(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	GLenum gbufferGlTextureFormat(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelGbufferGlTextureFormat(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelGbufferGlTextureFormat(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelRadianceGlTextureFormat(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	GLenum voxelRadianceGlTextureFormat(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	std::string gbufferGlShaderFormat(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	std::string gbufferGlShaderFormat(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelGbufferGlShaderFormat(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelGbufferGlShaderFormat(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelRadianceGlShaderFormat(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	std::string voxelRadianceGlShaderFormat(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	// Convenience accessor
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMaps(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void updateShadowMaps(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGrid(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGrid(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGridRadiance(Scene::Scene& scene, Scene::Object* renderSettings);

	////////////////////////////////////////////////////////////////////////////////
	void updateVoxelGridRadiance(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	// Render callbacks
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	bool firstCallTypeCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	bool firstCallObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool multiCallTypeCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	bool multiCallObjectCondition(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void initFrameOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void uploadUniformsOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void uniformsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void uniformsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
	
	////////////////////////////////////////////////////////////////////////////////
	bool depthPrePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void depthPrePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void depthPrePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void gbufferBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool voxelBasePassTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void voxelBasePassEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void shadowMapsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool resolveMsaaTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void resolveMsaaOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool voxelLightingTypePreConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName);

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void voxelLightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void lightingBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void lightingEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void translucencyBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void translucencyEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void hdrEffectsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void hdrEffectsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void toneMapBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void toneMapEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void ldrEffectsBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void ldrEffectsEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void guiBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void guiEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void presentBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void presentEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void swapBeginOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void swapEndOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void blitFramebufferOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void swapGBuffersOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(RENDER_SETTINGS, RenderSettingsComponent, RenderSettings::RenderSettingsComponent)
DECLARE_OBJECT(RENDER_SETTINGS, COMPONENT_ID_RENDER_SETTINGS, COMPONENT_ID_EDITOR_SETTINGS)