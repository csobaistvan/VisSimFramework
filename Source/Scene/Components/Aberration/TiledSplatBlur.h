#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "WavefrontAberration.h"

////////////////////////////////////////////////////////////////////////////////
/// TILED SPLATTING BLUR FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace TiledSplatBlur
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "TiledSplatBlur";
	static constexpr const char* DISPLAY_NAME = "Tiled Splat Blur";
	static constexpr const char* CATEGORY = "Aberration";

	////////////////////////////////////////////////////////////////////////////////
	/** A compute-based tile splat depth of field component. */
	struct TiledSplatBlurComponent
	{
		// Input dynamic range
		meta_enum(InputDynamicRange, int, HDR, LDR);

		// The various output modes available, mainly for debugging
		meta_enum(OutputMode, int, Convolution, MergedColor, MergedDepth, FragmentSize, PsfId, LerpFactor, BlurRadius, BlurRadiusFract, BlurRadiusCont, TileBufferSize, Alpha, IncidentAngle);

		// The various overlay modes avaiable, entirely for debugging purposes
		meta_enum(OverlayMode, int, None, PsfBorder, BlurRadiusBorder, TileBorder, ObjectDepthBorder);

		// The various accumulation methods
		meta_enum(AccumulationMethod, int, Sum, BackToFront, FrontToBack);

		// Type of shader to use
		meta_enum(CoefficientLerpMethod, int, Lerp, Trigonometric);

		// The various PSF texture formats
		meta_enum(PsfTextureFormat, int, F11, F16, F32);

		// PSF texture layouts
		meta_enum(PsfTextureDepthLayout, int, RadiusBased, DiopterBased);
		meta_enum(PsfTextureAngleLayout, int, LayerBased, PsfBased);

		// PSF axis handling method
		meta_enum(PsfAxisMethod, int, OnAxis, OffAxis);

		// The various weight scaling methods (for fragment size)
		meta_enum(WeightScaleMethod, int, One, Linear, AreaCircle, AreaSquare);

		// The various weight rescaling methods
		meta_enum(WeightRescaleMethod, int, LinearRescale, AlphaBlend);

		// Type of shader to use
		meta_enum(ShaderType, int, Debug, Release, Auto);

		// Eye aberration description
		Aberration::WavefrontAberration m_aberration;

		// Whether the aberration cache should be cleared or not after computation
		bool m_clearAberrationCache = false;

		// Type of input to use
		InputDynamicRange m_inputDynamicRange;

		// Output mode
		OutputMode m_outputMode;

		// Overlay mode
		OverlayMode m_overlayMode;

		// Accumulation method.
		AccumulationMethod m_accumulationMethod;

		// Resolution at which to render
		int m_renderResolutionId;

		// Size of a tile, in pixels
		int m_tileSize;

		// Max CoC radius
		int m_maxCoC;

		// Number of wavelengths to consider
		int m_numWavelengths;

		/** Various group sizes*/
		struct GroupSizes
		{
			// Batch size for the PSF interpolation step.
			int m_interpolation;

			// Batch size for the fragment merge step.
			int m_merge;

			// Batch size for the tile splatting step.
			int m_splat;

			// Batch size for the sorting step.
			int m_sort;

			// Batch size for the sorting step.
			int m_convolution;
		} m_groupSizes;

		// Number of fragment merge steps to take
		int m_fragmentMergeSteps;

		// Max sort elements
		int m_maxSortElements;

		// Fragment merge parameters
		struct MergePreset
		{
			int m_blockSize;
			float m_colorSimilarityThreshold;
			float m_colorContrastThreshold;
			float m_depthSimilarityThreshold;
			float m_minBlurRadiusThreshold;
		};

		// Thresholds for fragment merging
		std::vector<MergePreset> m_mergePresets;

		// Alpha threshold for early accumulation stop.
		float m_alphaThreshold;

		// Whether sorting should be applied to the tile buffer or not.
		bool m_sortTileBuffer;

		// Whether the result is to be normalized or not
		bool m_normalizeResult;

		// The PSF texture format
		PsfTextureFormat m_psfTextureFormat;

		// The PSF texture layout
		PsfTextureDepthLayout m_psfTextureDepthLayout;
		PsfTextureAngleLayout m_psfTextureAngleLayout;

		// PSF axis management
		PsfAxisMethod m_psfAxisMethod;

		// Fragment size weight scaling method
		WeightScaleMethod m_weightScaleMethod;

		// Fragment rescale method
		WeightRescaleMethod m_weightRescaleMethod;

		// What type of shader to use
		ShaderType m_shaderType;

		// Coefficient lerping method
		CoefficientLerpMethod m_coeffLerpMethod;

		// Reduction terms for the PSF layers
		glm::vec3 m_psfLayersS{ 0.0f };
		glm::vec3 m_psfLayersP{ 0.0f };
		bool m_clearPsfTexture = false;

		// Scale factors for the constant and scaled sort depth offsets
		float m_sortDepthOffset;
		float m_sortDepthScale;

		// Depth offset used for debugging, to simulate movement.
		float m_depthOffset;

		// ---- Private members

		// All the aberration presets available.
		Aberration::WavefrontAberrationPresets m_aberrationPresets;

		// List of derived PSF parameters
		struct DerivedPsfParameters
		{
			size_t m_minBlurRadius;
			size_t m_maxBlurRadius;
			size_t m_numPsfWeights;
			float m_blurRadiusDeg;
		};
		std::vector<DerivedPsfParameters> m_derivedPsfParameters;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Tiled splat blur common parameters. */
	struct UniformDataCommon
	{
		// Common, pre-computed values
		glm::uvec2 m_numTiles;
		glm::ivec2 m_renderResolution;
		glm::ivec2 m_paddedResolution;
		glm::vec2 m_cameraFov;
		GLuint m_fragmentBufferSubentries;
		GLuint m_tileBufferCenterSubentries;
		GLuint m_tileBufferTotalSubentries;
		GLuint m_numSortIterations;

		// Run modes
		GLuint m_psfAxisMethod;
		GLuint m_psfTextureFormat;
		GLuint m_psfTextureDepthLayout;
		GLuint m_psfTextureAngleLayout;
		GLuint m_weightScaleMethod;
		GLuint m_weightRescaleMethod;
		GLuint m_outputMode;
		GLuint m_overlayMode;
		GLuint m_accumulationMethod;

		// PSF texture settings
		alignas(sizeof(glm::vec4)) glm::vec4 m_psfLayersS;
		alignas(sizeof(glm::vec4)) glm::vec4 m_psfLayersP;

		// Merge settings
		GLuint m_numMergeSteps;
		GLuint m_mergedFragmentSize;

		// Sort settings
		GLfloat m_sortOffsetConstant;
		GLfloat m_sortOffsetScale;

		// Convolution settings
		GLuint m_renderChannels;
		GLuint m_renderLayers;
		GLfloat m_depthOffset;
		GLfloat m_alphaThreshold;
		GLfloat m_normalizeResult;

		// PSF properties
		GLuint m_minBlurRadiusCurrent;
		GLuint m_maxBlurRadiusCurrent;
		GLuint m_minBlurRadiusGlobal;
		GLuint m_maxBlurRadiusGlobal;
		GLuint m_numDefocuses;
		GLuint m_numHorizontalAngles;
		GLuint m_numVerticalAngles;
		GLuint m_numChannels;
		GLuint m_numApertures;
		GLuint m_numFocuses;
		GLfloat m_objectDistancesMin;
		GLfloat m_objectDistancesMax;
		GLfloat m_objectDistancesStep;
		GLfloat m_apertureMin;
		GLfloat m_apertureMax;
		GLfloat m_apertureStep;
		GLfloat m_focusDistanceMin;
		GLfloat m_focusDistanceMax;
		GLfloat m_focusDistanceStep;
		GLfloat m_incidentAnglesHorMin;
		GLfloat m_incidentAnglesHorMax;
		GLfloat m_incidentAnglesHorStep;
		GLfloat m_incidentAnglesVertMin;
		GLfloat m_incidentAnglesVertMax;
		GLfloat m_incidentAnglesVertStep;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Tiled splat blur PSF interpolation parameters. */
	struct UniformDataPsfInterpolation
	{
		GLuint m_radius;
		GLuint m_diameter;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Tiled splat blur fragment merge parameters. */
	struct UniformDataFragmentMerge
	{
		GLuint m_blockSize;
		GLfloat m_colorSimilarityThreshold;
		GLfloat m_colorContrastThreshold;
		GLfloat m_depthSimilarityThreshold;
		GLfloat m_minBlurRadiusThreshold;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Tiled splat blur sort parameters. */
	struct UniformDataSort
	{
		GLuint m_groupId;
		GLuint m_groupSize;
		GLuint m_compareDistance;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Tiled splat blur psf parameters. */
	struct UniformDataPsfParam
	{
		GLuint m_minBlurRadius;
		GLuint m_maxBlurRadius;
		GLuint m_weightStartId;
		GLfloat m_blurRadiusDeg;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void demoSetup(Scene::Scene& scene);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(TILED_SPLAT_BLUR, TiledSplatBlurComponent, TiledSplatBlur::TiledSplatBlurComponent)
DECLARE_OBJECT(TILED_SPLAT_BLUR, COMPONENT_ID_TILED_SPLAT_BLUR, COMPONENT_ID_EDITOR_SETTINGS)