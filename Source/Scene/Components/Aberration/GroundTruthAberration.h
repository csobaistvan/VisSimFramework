#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "WavefrontAberration.h"

////////////////////////////////////////////////////////////////////////////////
/// GROUND TRUTH ABERRATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace GroundTruthAberration
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "GroundTruthAberration";
	static constexpr const char* DISPLAY_NAME = "Ground Truth Aberration";
	static constexpr const char* CATEGORY = "Aberration";

	////////////////////////////////////////////////////////////////////////////////
	struct ConvolutionSettings
	{
		// Input dynamic range
		meta_enum(InputDynamicRange, int, HDR, LDR);

		// What blend mode should be used
		meta_enum(Algorithm, int, PerPixel, PerPixelStack, DepthLayers);

		// What blend mode should be used
		meta_enum(BlendMode, int, Sum, FrontToBack, BackToFront);

		// What should be printed out
		meta_enum(PrintDetail, int, Nothing, Progress, Detailed);

		// Number of channels to render
		int m_numChannels = 3;

		// What precision to use for incident angles
		float m_incidentAnglesPrecision = 0.1;

		// What precision to use for object distance binning
		float m_dioptresPrecision = 1e-2f;

		// Whether incident angles should be centered on the region or not
		bool m_centerIncidentAngles = false;

		// Whether dioptres should be centered on the region or not
		bool m_centerDioptres = false;

		// Whether we should simulate off axis aberrations as well or not
		bool m_simulateOffAxis = false;

		// Whether we should export the psfs or not
		bool m_exportPsfs = false;

		// Whether we should be writing to log files during convolution or not
		bool m_omitFileLogging = true;

		// Type of input to use
		InputDynamicRange m_inputDynamicRange;

		// Which GT algorithm to run
		Algorithm m_algorithm = PerPixel;

		// The blend mode to use
		BlendMode m_blendMode = Sum;

		// What to print out
		PrintDetail m_printDetail = Progress;

		// Put computer to sleep mode when done
		bool m_sleepWhenDone = false;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct MetricSettings
	{
		// HDR-VDP parameters
		float m_hdrvdpPeakLuminance = 400.0f;
		float m_hdrvdpContrastRatio = 1000.0f;
		float m_hdrvdpGamma = 2.2f;
		float m_hdrvdpAmbientLight = 100.0f;
		float m_hdrvdpDisplaySize = 28.0f;
		glm::ivec2 m_hdrvdpDisplayResolution{ 3840, 2160 };
		float m_hdrvdpViewDistance = 1.0f;
		float m_hdrvdpSurround = 13.0f;
		float m_hdrvdpSensitivityCorrection = -0.3f;

		// Whether we should export result of the metric computations or not
		bool m_exportMetrics = false;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Structure holding all the simulation results
	struct Results
	{
		// The various output modes available, mainly for debugging
		meta_enum(ResultAttribute, int,
			Scene,
			Original,
			Convolution,
			Depth,
			BlurRadius,
			Normalization,
			NumberOfSamples,
			IncidentAngles,
			Defocus,
			BinIncidentAngles,
			BinDefocus,
			Reference,
			Difference,
			Ssim,
			SsimJet,
			HdrVdp3,
			HdrVdp3Jet);

		std::string m_timestampDisplay;
		std::string m_timestampFile;
		std::string m_aberration;
		std::string m_scene;
		std::string m_camera;

		int m_width = -1;
		int m_height = -1;
		int m_channels = -1;
		float m_incidentAnglesPrecision = 0.0f;
		float m_dioptresPrecision = -1;
		bool m_centerIncidentAngles = false;
		bool m_centerDioptres = false;
		int m_numBins = -1;
		float m_apertureDiameter = -1.0f;
		float m_focusDistance = -1.0f;
		bool m_simulateOffAxis = false;
		bool m_isHdr = false;

		ConvolutionSettings::Algorithm m_algorithm = ConvolutionSettings::PerPixel;
		ConvolutionSettings::BlendMode m_blendMode = ConvolutionSettings::Sum;
		int m_numDepthSlices;

		float m_minDepth = FLT_MAX;
		float m_maxDepth = -FLT_MAX;
		glm::vec4 m_minWeight = glm::vec4(FLT_MAX);
		glm::vec4 m_maxWeight = glm::vec4(-FLT_MAX);
		glm::ivec4 m_minNumSamples = glm::ivec4(INT_MAX);
		glm::ivec4 m_maxNumSamples = glm::ivec4(-INT_MAX);
		glm::vec4 m_minDefocus = glm::vec4(FLT_MAX);
		glm::vec4 m_maxDefocus = glm::vec4(-FLT_MAX);
		glm::vec4 m_minBlurRadius = glm::vec4(FLT_MAX);
		glm::vec4 m_maxBlurRadius = glm::vec4(-FLT_MAX);

		// HDR-VDP probability of detection
		float m_hdrvdp;

		// Per-channel and average mean structural similarity
		glm::vec3 m_mssim;
		float m_mmssim;

		// Per-channel and average peak signal-to-noise and signal-to-noise ratio
		glm::vec3 m_psnr;
		glm::vec3 m_snr;
		float m_mpsnr;
		float m_msnr;

		// Per-channel and average mean-squared error and root-mean-squared error
		glm::vec3 m_mse;
		glm::vec3 m_rmse;
		float m_mmse;
		float m_mrmse;

		// Processing time
		float m_psfBinTime = 0.0f;
		float m_convolutionTime = 0.0f;
		float m_totalProcessingTime = 0.0f;

		// Resulting images
		std::unordered_map<ResultAttribute, std::vector<unsigned char>> m_textures;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct OutputSettings
	{
		// Which result to show
		Results::ResultAttribute m_outputMode = Results::Scene;

		// Which result to use for rendering
		int m_resultRenderId = 0;

		// Which result to use for previewing
		int m_resultTooltipId = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Component for splatting a dense set of PSFs. */
	struct GroundTruthAberrationComponent
	{
		// Convolution settings
		ConvolutionSettings m_convolutionSettings;

		// Metric settings
		MetricSettings m_metricSettings;

		// Output settings
		OutputSettings m_outputSettings;

		// The aberration that we are simulating
		Aberration::WavefrontAberration m_aberration;

		// ---- Private members

		// File logging state before the process was started
		Debug::LogChannels m_fileLogState;

		// All the aberration presets available.
		Aberration::WavefrontAberrationPresets m_aberrationPresets;

		// List of previously generated results
		std::vector<Results> m_results;
	};

	////////////////////////////////////////////////////////////////////////////////
	void loadPreviousResults(Scene::Scene& scene, Scene::Object* object);

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
DECLARE_COMPONENT(GROUND_TRUTH_ABERRATION, GroundTruthAberrationComponent, GroundTruthAberration::GroundTruthAberrationComponent)
DECLARE_OBJECT(GROUND_TRUTH_ABERRATION, COMPONENT_ID_GROUND_TRUTH_ABERRATION, COMPONENT_ID_EDITOR_SETTINGS)