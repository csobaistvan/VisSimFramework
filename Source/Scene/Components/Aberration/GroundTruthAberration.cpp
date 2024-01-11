#include "PCH.h"
#include "GroundTruthAberration.h"
#include "ComplexBlur.h"
#include "TiledSplatBlur.h"

namespace GroundTruthAberration
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(GROUND_TRUTH_ABERRATION);
	DEFINE_OBJECT(GROUND_TRUTH_ABERRATION);
	REGISTER_OBJECT_UPDATE_CALLBACK(GROUND_TRUTH_ABERRATION, AFTER, INPUT);
	REGISTER_OBJECT_RENDER_CALLBACK(GROUND_TRUTH_ABERRATION, "Ground Truth Aberration (HDR)", OpenGL, AFTER, "Effects (HDR) [Begin]", 1, 
		&GroundTruthAberration::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, 
		&GroundTruthAberration::renderObjectPreconditionHDROpenGL, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(GROUND_TRUTH_ABERRATION, "Ground Truth Aberration (LDR)", OpenGL, AFTER, "Effects (LDR) [Begin]", 1, 
		&GroundTruthAberration::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, 
		&GroundTruthAberration::renderObjectPreconditionLDROpenGL, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	Aberration::WavefrontAberration& getAberration(Scene::Scene& scene, Scene::Object* object)
	{
		return object->component<GroundTruthAberrationComponent>().m_aberration;
	}

	////////////////////////////////////////////////////////////////////////////////
	Aberration::WavefrontAberrationPresets& getAberrationPresets(Scene::Scene& scene, Scene::Object* object)
	{
		return object->component<GroundTruthAberrationComponent>().m_aberrationPresets;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initAberrationPresets(Scene::Scene& scene, Scene::Object* object)
	{
		Aberration::initAberrationStructure(scene, getAberration(scene, object), getAberrationPresets(scene, object));
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadShaders(Scene::Scene& scene, Scene::Object* object)
	{
		Aberration::loadShaders(scene, getAberration(scene, object));
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadNeuralNetworks(Scene::Scene& scene, Scene::Object* object)
	{
		Aberration::loadNeuralNetworks(scene, getAberration(scene, object));
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Custom, initAberrationPresets, "Aberration Presets");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, loadShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::NeuralNetwork, loadNeuralNetworks, "Neural Networks");

		DelayedJobs::postJob(scene, &object, "Load Previous Results", [](Scene::Scene& scene, Scene::Object& object)
		{
			loadPreviousResults(scene, &object);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Store the shader initializer
		Scene::removeResourceInitializer(scene, object.m_name, Scene::Shader);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void uploadResultTexture(Scene::Scene& scene, Scene::Object* object, Results& result, std::string const& suffix, const void* data)
	{
		std::string textureName = object->m_name + "_" + suffix;
		Scene::createTexture(scene, textureName, GL_TEXTURE_2D, result.m_width, result.m_height, 1, GL_RGBA8, GL_RGBA, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_UNSIGNED_BYTE, data);
	}

	////////////////////////////////////////////////////////////////////////////////
	using ResultsTextures = std::unordered_map<Results::ResultAttribute, const char*>;

	////////////////////////////////////////////////////////////////////////////////
	static ResultsTextures s_allTextures =
	{
		{ Results::Original, "original" },
		{ Results::Convolution, "result" },
		{ Results::Depth, "depth" },
		{ Results::Normalization, "normalization" },
		{ Results::NumberOfSamples, "number_of_samples" },
		{ Results::IncidentAngles, "incident_angles" },
		{ Results::Defocus, "defocus" },
		{ Results::BinIncidentAngles, "bin_incident_angles" },
		{ Results::BinDefocus, "bin_defocus" },
		{ Results::BlurRadius, "radii" },
		{ Results::Reference, "reference" },
		{ Results::Ssim, "ssim" },
		{ Results::SsimJet, "ssim_jet" },
		{ Results::HdrVdp3, "hdrvdp3" },
		{ Results::HdrVdp3Jet, "hdrvdp3_jet" },
		{ Results::Difference, "diff" },
	};

	////////////////////////////////////////////////////////////////////////////////
	static ResultsTextures s_resultsTextures =
	{
		{ Results::Original, "original" },
		{ Results::Convolution, "result" },
		{ Results::Depth, "depth" },
		{ Results::Normalization, "normalization" },
		{ Results::NumberOfSamples, "number_of_samples" },
		{ Results::IncidentAngles, "incident_angles" },
		{ Results::Defocus, "defocus" },
		{ Results::BinIncidentAngles, "bin_incident_angles" },
		{ Results::BinDefocus, "bin_defocus" },
		{ Results::BlurRadius, "radii" },
	};

	////////////////////////////////////////////////////////////////////////////////
	static ResultsTextures s_metricsTextures =
	{
		{ Results::Original, "original" },
		{ Results::Convolution, "ground_truth" },
		{ Results::Reference, "reference" },
		{ Results::Ssim, "ssim" },
		{ Results::SsimJet, "ssim_jet" },
		{ Results::HdrVdp3, "hdrvdp3" },
		{ Results::HdrVdp3Jet, "hdrvdp3_jet" },
		{ Results::Difference, "diff" },
	};

	////////////////////////////////////////////////////////////////////////////////
	std::string getTextureNamePrefix(Results& result)
	{
		std::stringstream ss;
		ss << result.m_scene;
		ss << "_" << result.m_aberration;
		ss << "_" << ConvolutionSettings::Algorithm_value_to_string(result.m_algorithm);
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getTextureName(Results::ResultAttribute attrib, std::string const& prefix)
	{
		std::stringstream ss;
		ss << prefix;
		ss << "_" << s_allTextures[attrib];
		ss << ".png";
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getTextureName(Results& result, Results::ResultAttribute attrib)
	{
		return getTextureName(attrib, getTextureNamePrefix(result));
	}

	////////////////////////////////////////////////////////////////////////////////
	void uploadResultRender(Scene::Scene& scene, Scene::Object* object, Results& result)
	{
		for (auto const& texture : result.m_textures)
			if (s_allTextures.find(texture.first) != s_allTextures.end())
				uploadResultTexture(scene, object, result, s_allTextures[texture.first], texture.second.data());
	}

	////////////////////////////////////////////////////////////////////////////////
	void uploadResultTooltip(Scene::Scene& scene, Scene::Object* object, Results& result)
	{
		uploadResultTexture(scene, object, result, "tooltip_color", result.m_textures[Results::Original].data());
		uploadResultTexture(scene, object, result, "tooltip_result", result.m_textures[Results::Convolution].data());
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isDisplayResultValid(Scene::Scene& scene, Scene::Object* object)
	{
		auto const& results = object->component<GroundTruthAberrationComponent>().m_results;
		return !results.empty() && object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultRenderId < results.size();
	}

	////////////////////////////////////////////////////////////////////////////////
	Results& getDisplayResults(Scene::Scene& scene, Scene::Object* object)
	{
		return object->component<GroundTruthAberrationComponent>().m_results[object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultRenderId];
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadTexture(Scene::Scene& scene, std::filesystem::directory_entry subFolder, std::string const& fileName, Results& results, std::vector<unsigned char>& outImage)
	{
		// Try to load the image
		auto imgPath = (subFolder.path() / fileName).string();
		Debug::log_debug() << "Trying to load previous image; filename: " << imgPath << Debug::end;
		if (Asset::existsFile(imgPath) == false) return;

		auto const& imageOpt = Asset::loadImage(scene, imgPath);
		cv::Mat image = imageOpt.value();

		Debug::log_debug() << "Image dimensions: " << image.cols << ", " << image.rows << Debug::end;

		// Allocate memory for the result
		outImage.resize(image.rows * image.cols * 4);

		// Read out the pixels
		for (size_t row = 0; row < image.rows; ++row)
		for (size_t col = 0; col < image.cols; ++col)
		{
			size_t arrayId = ((row * image.cols) + col) * 4;
			cv::Vec4b pixelValue = image.at<cv::Vec4b>(row, col);

			outImage[arrayId + 0] = pixelValue[0];
			outImage[arrayId + 1] = pixelValue[1];
			outImage[arrayId + 2] = pixelValue[2];
			outImage[arrayId + 3] = pixelValue[3];
		}

		// Store the image size, if needed
		if (results.m_width < 0 || results.m_height < 0)
		{
			results.m_width = image.cols;
			results.m_height = image.rows;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void allocEmptyTexture(Scene::Scene& scene, Results& results, std::vector<unsigned char>& outImage)
	{
		// Only do this if the result is not empty
		if (outImage.size() > 0) return;

		// Alloc space in the image
		outImage.resize(results.m_width * results.m_height * 4);
		
		// Clear out the pixels
		for (size_t row = 0; row < results.m_height; ++row)
		for (size_t col = 0; col < results.m_width; ++col)
		{
			size_t arrayId = ((row * results.m_width) + col) * 4;

			outImage[arrayId + 0] = 0;
			outImage[arrayId + 1] = 0;
			outImage[arrayId + 2] = 0;
			outImage[arrayId + 3] = 255;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadResultAttributes(Scene::Scene& scene, std::filesystem::directory_entry subFolder, std::string const& fileName, Results& results)
	{
		// Try to load the image
		auto attributesPath = (subFolder.path() / fileName).string();
		if (Asset::existsFile(attributesPath) == false) return;

		std::ordered_pairs_in_blocks attributes = Asset::loadPairsInBlocks(scene, attributesPath).value();

		for (auto const& stateVar : attributes["Resolution"])
		{
			if (stateVar.first == "Width")  results.m_width = std::from_string<int>(stateVar.second);
			if (stateVar.first == "Height") results.m_height = std::from_string<int>(stateVar.second);
		}
		
		for (auto const& stateVar : attributes["Settings"])
		{
			if (stateVar.first == "TimestampDisplay")          results.m_timestampDisplay = stateVar.second;
			if (stateVar.first == "TimestampFile")             results.m_timestampFile = stateVar.second;
			if (stateVar.first == "Aberration")                results.m_aberration = stateVar.second;
			if (stateVar.first == "Scene")                     results.m_scene = stateVar.second;
			if (stateVar.first == "Camera")                    results.m_camera = stateVar.second;
			if (stateVar.first == "Focus")                     results.m_focusDistance = std::from_string<float>(stateVar.second);
			if (stateVar.first == "Aperture")                  results.m_apertureDiameter = std::from_string<float>(stateVar.second);
			if (stateVar.first == "Channels")                  results.m_channels = std::from_string<int>(stateVar.second);
			if (stateVar.first == "Off-Axis")                  results.m_simulateOffAxis = std::from_string<bool>(stateVar.second);
			if (stateVar.first == "HDR")                       results.m_isHdr = std::from_string<bool>(stateVar.second);
			if (stateVar.first == "BlendMode")                 results.m_blendMode = ConvolutionSettings::BlendMode_meta_from_name(stateVar.second).value_or(ConvolutionSettings::BlendMode_meta.members[0]).value;
			if (stateVar.first == "Algorithm")                 results.m_algorithm = ConvolutionSettings::Algorithm_meta_from_name(stateVar.second).value_or(ConvolutionSettings::Algorithm_meta.members[0]).value;
			if (stateVar.first == "NumDepthSlices")            results.m_numDepthSlices = std::from_string<int>(stateVar.second);
		}

		for (auto const& stateVar : attributes["Bins"])
		{
			if (stateVar.first == "NumBins")                 results.m_numBins = std::from_string<int>(stateVar.second);
			if (stateVar.first == "DioptresPrecision")       results.m_dioptresPrecision = std::from_string<float>(stateVar.second);
			if (stateVar.first == "IncidentAnglesPrecision") results.m_incidentAnglesPrecision = std::from_string<float>(stateVar.second);
			if (stateVar.first == "CenterDioptres")          results.m_centerDioptres = std::from_string<bool>(stateVar.second);
			if (stateVar.first == "CenterIncidentAngles")    results.m_centerIncidentAngles = std::from_string<bool>(stateVar.second);
		}

		for (auto const& stateVar : attributes["RunningTimes"])
		{
			if (stateVar.first == "PsfBins")          results.m_psfBinTime = std::from_string<float>(stateVar.second);
			if (stateVar.first == "Convolution")      results.m_convolutionTime = std::from_string<float>(stateVar.second);
			if (stateVar.first == "TotalProcessing")  results.m_totalProcessingTime = std::from_string<float>(stateVar.second);
		}

		for (auto const& stateVar : attributes["Limits"])
		{
			if (stateVar.first == "MinDepth")               results.m_minDepth = std::from_string<float>(stateVar.second);
			if (stateVar.first == "MaxDepth")               results.m_maxDepth = std::from_string<float>(stateVar.second);
			if (stateVar.first == "MinDefocus")             results.m_minDefocus = std::from_string<glm::vec4>(stateVar.second);
			if (stateVar.first == "MaxDefocus")             results.m_maxDefocus = std::from_string<glm::vec4>(stateVar.second);
			if (stateVar.first == "MinBlurRadius")          results.m_minBlurRadius = std::from_string<glm::vec4>(stateVar.second);
			if (stateVar.first == "MaxBlurRadius")          results.m_maxBlurRadius = std::from_string<glm::vec4>(stateVar.second);
			if (stateVar.first == "MinWeight")              results.m_minWeight = std::from_string<glm::vec4>(stateVar.second);
			if (stateVar.first == "MaxWeight")              results.m_maxWeight = std::from_string<glm::vec4>(stateVar.second);
			if (stateVar.first == "MinNumSamples")          results.m_minNumSamples = std::from_string<glm::ivec4>(stateVar.second);
			if (stateVar.first == "MaxNumSamples")          results.m_maxNumSamples = std::from_string<glm::ivec4>(stateVar.second);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadPreviousResults(Scene::Scene& scene, Scene::Object* object)
	{
		// Get the root filesystem path
		std::filesystem::path root = EnginePaths::generatedFilesFolder() / "GroundTruthAberration";

		// Make sure the path exists
		if (std::filesystem::exists(root) == false) return;

		// Go through the subfolders
		for (auto const& subFolder : std::filesystem::directory_iterator(root))
		{
			// Make sure its a results folder
			if (subFolder.is_directory() == false || std::filesystem::exists(subFolder.path() / "attributes.ini") == false)
				continue;

			// Make a new entry
			object->component<GroundTruthAberrationComponent>().m_results.emplace_back();
			auto& results = object->component<GroundTruthAberrationComponent>().m_results.back();

			// Read back the attributes
			loadResultAttributes(scene, subFolder, "attributes.ini", results);

			// Read back the textures
			for (auto& texture : s_allTextures)
			{
				std::string const& fileName = getTextureName(results, texture.first);
				allocEmptyTexture(scene, results, results.m_textures[texture.first]);
				loadTexture(scene, subFolder, fileName, results, results.m_textures[texture.first]);
			}
		}

		// Show the last result
		if (object->component<GroundTruthAberrationComponent>().m_results.size() > 0)
		{
			// Upload the results
			uploadResultRender(scene, object, object->component<GroundTruthAberrationComponent>().m_results.back());

			object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultRenderId = object->component<GroundTruthAberrationComponent>().m_results.size() - 1;
			object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode = Results::Convolution;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// Represents the data for a single input pixel
	struct InputPixelData
	{
		float m_color = 0.0f;
		float m_depth = 0.0f;
		float m_blurRadius = 0.0f;
		float m_defocus = 0.0f;
		glm::vec2 m_incidentAngles{ 0.0f };
		std::pair<glm::vec2, float> m_psfBinParams{ glm::vec2(0.0f), 0.0f };
		glm::vec3 m_worldSpaceCoords{ 0.0f };
		glm::vec3 m_viewSpaceCoords{ 0.0f };
		glm::vec3 m_sphericalCoords{ 0.0f };
	};

	////////////////////////////////////////////////////////////////////////////////
	// Represents the data for a single output pixel
	struct OutputPixelData
	{
		float m_result = 0.0f;
		float m_weight = 0.0f;
		int m_numSamples = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	//  A single entry in the PSF bin
	struct PsfBinEntry
	{
		float m_radius;
		float m_defocus;
		Aberration::Psf m_psf;
	};

	////////////////////////////////////////////////////////////////////////////////
	//  Represents a single sample
	struct Sample
	{
		float m_depth;
		float m_color;
		float m_weight;
	};

	////////////////////////////////////////////////////////////////////////////////
	meta_enum(ConvolutionPhase, int,
		PsfBins,
		Convolution,
		Total);

	////////////////////////////////////////////////////////////////////////////////
	// Common data
	struct CommonData
	{
		// Necessary scene objects
		Scene::Object* m_renderSettings;
		Scene::Object* m_camera;

		// Per-pixel property textures
		boost::multi_array<InputPixelData, 3> m_inputPixels;
		boost::multi_array<OutputPixelData, 3> m_outputPixels;

		// Render parameters
		glm::ivec2 m_renderResolution;
		size_t m_numPixelsRender;
		glm::mat4 m_projection;
		float m_fovy;
		glm::vec2 m_fov;

		// PSF parameters
		bool m_offAxis;
		float m_focusDistance;
		float m_apertureDiameter;
		size_t m_numChannels;
		float m_dioptresPrecision;
		float m_incidentAnglesPrecision;
		bool m_centerIncidentAngles;
		bool m_centerDioptres;
		size_t m_maxBlurRadius;
		std::vector<float> m_lambdas;

		// File names
		std::string m_resultFileName;
		std::filesystem::path m_resultsFolder;
		std::string m_resultsPrefix;
		std::filesystem::path m_psfFolderOriginal;
		std::filesystem::path m_psfFolderDownscaled;

		// PSF bins
		std::unordered_map<glm::vec2, std::unordered_map<float, std::vector<PsfBinEntry>>> m_psfBins;
		std::vector<std::pair<glm::vec2, float>> m_psfBinParams;
		std::vector<float> m_psfBinHorizontalAngles;
		std::vector<float> m_psfBinVerticalAngles;
		std::vector<float> m_psfBinDioptres;
		std::vector<float> m_psfBinDepths;

		// Aberration preset
		Aberration::WavefrontAberration m_aberration;

		// Various timers
		std::unordered_map<ConvolutionPhase, DateTime::Timer> m_timers;
	};

	////////////////////////////////////////////////////////////////////////////////
	//  Per-thread data
	struct PerThreadData
	{
		// Per-pixel PSF sample list
		std::vector<std::vector<Sample>> m_samples;
	};

	////////////////////////////////////////////////////////////////////////////////
	PsfBinEntry& psfChannel(CommonData& commonData, InputPixelData& pixelData, size_t channel)
	{
		// Make sure that the pixel is properly binned
		if (commonData.m_psfBins.find(pixelData.m_psfBinParams.first) == commonData.m_psfBins.end())
			Debug::log_error() << "Found an incident angle with no matching bin entry; bin params: " << pixelData.m_psfBinParams.first << Debug::end;

		if (commonData.m_psfBins[pixelData.m_psfBinParams.first].find(pixelData.m_psfBinParams.second) == commonData.m_psfBins[pixelData.m_psfBinParams.first].end())
			Debug::log_error() << "Found a diopter with no matching bin entry; bin params: " << pixelData.m_psfBinParams.second << Debug::end;

		// Return the corresponding PSF entry
		return commonData.m_psfBins[pixelData.m_psfBinParams.first][pixelData.m_psfBinParams.second][glm::min(commonData.m_numChannels - 1, channel)];
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string resultLabel(Results const& result)
	{
		std::stringstream ss;
		ss << result.m_scene << "---";
		ss << result.m_camera << "---";
		ss << ConvolutionSettings::Algorithm_value_to_string(result.m_algorithm) << "---";
		ss << result.m_aberration << "---";
		ss << result.m_timestampDisplay;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string resultFilename(Results const& result)
	{
		// Generate the necessary file name properties
		std::stringstream ss;
		//ss << result.m_scene << "_";
		ss << result.m_camera << "_";
		ss << ConvolutionSettings::Algorithm_value_to_string(result.m_algorithm) << "_";
		ss << result.m_aberration << "_";
		ss << result.m_timestampFile;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	void setFileLogging(Scene::Scene& scene, Scene::Object* object, bool enable)
	{
		if (object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_omitFileLogging)
		{
			// Extract the necessary scene objects
			Scene::Object* debugSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS);

			if (enable)
			{
				// Reset the old state
				debugSettings->component<DebugSettings::DebugSettingsComponent>().m_logToFile = 
					object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_fileLogState;

				// Update the logger state
				DebugSettings::updateLoggerStates(scene, debugSettings);
			}
			else
			{
				// Store the previous state
				object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_fileLogState =
					debugSettings->component<DebugSettings::DebugSettingsComponent>().m_logToFile;

				// Disable debug and trace logs to file
				debugSettings->component<DebugSettings::DebugSettingsComponent>().m_logToFile.m_debug = false;
				debugSettings->component<DebugSettings::DebugSettingsComponent>().m_logToFile.m_trace = false;

				// Update the logger state
				DebugSettings::updateLoggerStates(scene, debugSettings);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shouldLog(Scene::Scene& scene, Scene::Object* object, Threading::ThreadedExecuteEnvironment const& environment)
	{
		// Only the leading thread prints
		return environment.isLeadingThread();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool outputFilterLevel(Scene::Scene& scene, Scene::Object* object, ConvolutionSettings::PrintDetail level, const bool logFlag)
	{
		return logFlag && object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail >= level;
	}

	////////////////////////////////////////////////////////////////////////////////
	Debug::DebugOutputLevel outputLogLevel(Scene::Scene& scene, Scene::Object* object, ConvolutionSettings::PrintDetail level)
	{
		return object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail >= level ? Debug::Info : Debug::Null;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec2 roundIncidentAngles(const glm::vec2 incidentAngles, const float precision, const bool center)
	{
		return (glm::round(incidentAngles / precision) * precision) + (center ? 0.5f * precision : 0.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	float roundDioptres(const float depth, const float precision, const bool center)
	{
		return (glm::round(depth / precision) * precision) + (center ? 0.5f * precision : 0.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::pair<glm::vec2, float> constructBinParams(CommonData& commonData, const glm::vec2 incidentAngles, const float depth)
	{
		return std::make_pair
		(
			!commonData.m_offAxis ? glm::vec2(0.0f) :
			roundIncidentAngles(incidentAngles, commonData.m_incidentAnglesPrecision, commonData.m_centerIncidentAngles),
			roundDioptres(1.0f / depth, commonData.m_dioptresPrecision, commonData.m_centerDioptres)
		);
	}

	////////////////////////////////////////////////////////////////////////////////
	void prepareCommonData(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, Results& results, Aberration::WavefrontAberration aberration)
	{
		// Extract the necessary scene objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

		// Id of the current gbuffer
		const int gbufferId = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite;
		auto const& gbuffer = scene.m_gbuffer[gbufferId];

		// Resolution the effect is rendered at
		const glm::ivec2 renderResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		const size_t renderWidth = renderResolution[0], renderHeight = renderResolution[1];
		const size_t numPixelsRender = renderWidth * renderHeight;

		// Vertical FOV of the camera
		const float fovy = camera->component<Camera::CameraComponent>().m_fovy;

		// Extract the projection matrix to reproject the camera depth
		const auto projection = Camera::getProjectionMatrix(renderSettings, camera);

		// Start the total processing timer
		commonData.m_timers[ConvolutionPhase::Total].start();

		// Write out the necessary objects
		commonData.m_renderSettings = renderSettings;
		commonData.m_camera = camera;

		// Store the render parameters
		commonData.m_renderResolution = renderResolution;
		commonData.m_numPixelsRender = numPixelsRender;
		commonData.m_projection = projection;
		commonData.m_fovy = fovy;
		commonData.m_fov = Camera::getFieldOfView(renderResolution, fovy);

		// Extract the PSF generation settings
		commonData.m_offAxis = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_simulateOffAxis;
		commonData.m_focusDistance = commonData.m_camera->component<Camera::CameraComponent>().m_focusDistance;
		commonData.m_apertureDiameter = commonData.m_camera->component<Camera::CameraComponent>().m_fixedAperture;
		commonData.m_numChannels = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_numChannels;
		commonData.m_dioptresPrecision = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_dioptresPrecision;
		commonData.m_incidentAnglesPrecision = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_incidentAnglesPrecision;
		commonData.m_centerIncidentAngles = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_centerIncidentAngles;
		commonData.m_centerDioptres = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_centerDioptres;
		commonData.m_lambdas = std::vector<float>(aberration.m_psfParameters.m_lambdas.begin(), aberration.m_psfParameters.m_lambdas.begin() + commonData.m_numChannels);

		// Initialize the aberration
		commonData.m_aberration = aberration;
		commonData.m_aberration.m_psfParameters.m_logProgress = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail >= ConvolutionSettings::Detailed;
		commonData.m_aberration.m_psfParameters.m_logStats = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail >= ConvolutionSettings::Detailed;
		commonData.m_aberration.m_psfParameters.m_apertureDiameters = { commonData.m_apertureDiameter, commonData.m_apertureDiameter, 1 };
		commonData.m_aberration.m_psfParameters.m_focusDistances = { 1.0f / commonData.m_focusDistance, 1.0f / commonData.m_focusDistance, 1 };
		commonData.m_aberration.m_psfParameters.m_lambdas = commonData.m_lambdas;
		commonData.m_aberration.m_psfParameters.m_objectDistances = { FLT_MAX, FLT_MAX, 1 };
		commonData.m_aberration.m_psfParameters.m_incidentAnglesHorizontal = { FLT_MAX, FLT_MAX, 1 };
		commonData.m_aberration.m_psfParameters.m_incidentAnglesVertical = { FLT_MAX, FLT_MAX, 1 };
		const Aberration::PsfStackComputation computationFlags = 
			Aberration::PsfStackComputation_RelaxedEyeParameters | 
			Aberration::PsfStackComputation_FocusedEyeParameters |
			Aberration::PsfStackComputation_PsfUnits |
			Aberration::PsfStackComputation_PsfBesselTerms |
			Aberration::PsfStackComputation_PsfEnzCoefficients;
		Aberration::computePSFStack(scene, commonData.m_aberration, computationFlags);

		// Prepare the various per-pixel buffers
		commonData.m_inputPixels.resize(decltype(commonData.m_inputPixels)::extent_gen()[renderHeight][renderWidth][3]);
		commonData.m_outputPixels.resize(decltype(commonData.m_outputPixels)::extent_gen()[renderHeight][renderWidth][3]);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Allocate memory for the GBuffer textures
		std::unique_ptr<unsigned char[]> pixelBuffer(new unsigned char[numPixelsRender * 4]);
		std::unique_ptr<float[]> depthBuffer(new float[numPixelsRender]);

		// Extract the color and depth buffers
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTextureSubImage(gbuffer.m_colorTextures[gbuffer.m_readBuffer], 0, 0, 0, 0, renderWidth, renderHeight, 1,
			GL_RGBA, GL_UNSIGNED_BYTE, numPixelsRender * 4 * sizeof(unsigned char), pixelBuffer.get());
		glGetTextureSubImage(gbuffer.m_depthTexture, 0, 0, 0, 0, renderWidth, renderHeight, 1,
			GL_DEPTH_COMPONENT, GL_FLOAT, numPixelsRender * sizeof(float), depthBuffer.get());

		// Extract the relevant scene information
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t y, size_t x, size_t c)
			{
				// Pixel id into the buffer
				const size_t pixelId = (x + y * renderWidth);

				// Extract the scene information
				InputPixelData pixelData;
				pixelData.m_viewSpaceCoords = Camera::screenToCameraSpaceMeters(renderSettings, camera, glm::ivec2(x, y), depthBuffer[pixelId]);
				pixelData.m_worldSpaceCoords = Camera::screenToWorldSpaceMeters(renderSettings, camera, glm::ivec2(x, y), depthBuffer[pixelId]);
				pixelData.m_sphericalCoords = Camera::screenToSphericalCoordinatesDegM(renderSettings, camera, glm::ivec2(x, y), depthBuffer[pixelId]);
				pixelData.m_color = float(pixelBuffer[pixelId * 4 + c]) / 255.0f;
				pixelData.m_depth = pixelData.m_sphericalCoords.z;
				pixelData.m_incidentAngles = glm::vec2(pixelData.m_sphericalCoords.x, pixelData.m_sphericalCoords.y);
				pixelData.m_psfBinParams = constructBinParams(commonData, pixelData.m_incidentAngles, pixelData.m_depth);

				// Write out the output pixel
				commonData.m_inputPixels[y][x][c] = pixelData;

				/*
				if (environment.isLeadingThread())
				{
					Debug::log_info() 
						<< y << ", " << x << ", " << c << ": " 
						<< "cam-space: " << pixelData.m_viewSpaceCoords << ", "
						<< "spherical: " << glm::vec3(glm::degrees(pixelData.m_sphericalCoords.x), glm::degrees(pixelData.m_sphericalCoords.y), pixelData.m_sphericalCoords.z) << ", "
						<< "psf bin: " << pixelData.m_psfBinParams << ", "
						<< Debug::end;
				}
				*/
			},
			renderHeight, renderWidth, 3);

		// Generate the necessary file and folder names
		commonData.m_resultFileName = resultFilename(results);
		commonData.m_resultsFolder = EnginePaths::generatedFilesFolder() / "GroundTruthAberration" / commonData.m_resultFileName;
		commonData.m_resultsPrefix = getTextureNamePrefix(results);
		commonData.m_psfFolderOriginal = commonData.m_resultsFolder / "PSFs"s / "original";
		commonData.m_psfFolderDownscaled = commonData.m_resultsFolder / "PSFs"s / "downscaled";
	}

	////////////////////////////////////////////////////////////////////////////////
	void preparePerThreadData(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Per-thread Data");

		// Prepare the per-thread data
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t threadId)
			{
				// Extract the current thread's data
				PerThreadData& threadData = perThreadData[threadId];

				// Init the per-pixel sample lists
				threadData.m_samples.resize(3);
			},
			perThreadData.size());
	}

	////////////////////////////////////////////////////////////////////////////////
	void cleanupCommonData(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, Results& results, Aberration::WavefrontAberration aberration)
	{
		// Nothing to do
	}

	////////////////////////////////////////////////////////////////////////////////
	void cleanupPerThreadData(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		// Nothing to do
	}

	////////////////////////////////////////////////////////////////////////////////
	void initPsfBinsFromPixelDepths(Scene::Scene& scene, Scene::Object* object, CommonData& commonData)
	{
		auto width = commonData.m_renderResolution[0], height = commonData.m_renderResolution[1];

		// Collect the unique PSF bin param occurences
		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			auto const& pixelData = commonData.m_inputPixels[y][x][0];
			auto const& psfBinParams = pixelData.m_psfBinParams;
			if (commonData.m_psfBins.count(psfBinParams.first) == 0 || commonData.m_psfBins[psfBinParams.first].count(psfBinParams.second) == 0)
			{
				commonData.m_psfBinParams.push_back(psfBinParams);
				commonData.m_psfBins[psfBinParams.first][psfBinParams.second].resize(commonData.m_numChannels);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void initPsfBinsFromStack(Scene::Scene& scene, Scene::Object* object, CommonData& commonData)
	{
		if (commonData.m_offAxis)
		{
			// Generate depth slices
			auto const& horizontalAngles = Aberration::generateHorizontalAxes(scene, getAberration(scene, object));
			auto const& verticalAngles = Aberration::generateVerticalAxes(scene, getAberration(scene, object));
			auto const& defocusDioptres = Aberration::generateObjectDioptres(scene, getAberration(scene, object));
			for (size_t h = 0; h < horizontalAngles.size(); ++h)
			for (size_t v = 0; v < verticalAngles.size(); ++v)
			for (size_t d = 0; d < defocusDioptres.size(); ++d)
			{
				auto const& psfBinParams = std::make_pair(glm::vec2(horizontalAngles[h], verticalAngles[v]), defocusDioptres[d]);
				commonData.m_psfBinParams.push_back(psfBinParams);
				commonData.m_psfBins[psfBinParams.first][psfBinParams.second].resize(commonData.m_numChannels);
			}
		}
		else
		{
			// Generate depth slices
			auto const& defocusDioptres = Aberration::generateObjectDioptres(scene, getAberration(scene, object));
			for (size_t i = 0; i < defocusDioptres.size(); ++i)
			{
				auto const& psfBinParams = std::make_pair(glm::vec2(0.0f), defocusDioptres[i]);
				commonData.m_psfBinParams.push_back(psfBinParams);
				commonData.m_psfBins[psfBinParams.first][psfBinParams.second].resize(commonData.m_numChannels);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void computePsf(Scene::Scene& scene, Scene::Object* object, Threading::ThreadedExecuteEnvironment const& environment, 
		CommonData& commonData, PerThreadData& threadData, size_t binId)
	{
		// Whether we should be logging from this thread or not
		const bool log = shouldLog(scene, object, environment);

		// Extract the corresponding PSF bin
		auto const& psfBinParams = commonData.m_psfBinParams[binId];
		std::vector<PsfBinEntry>& psfs = commonData.m_psfBins[psfBinParams.first][psfBinParams.second];

		// Whether we have a new incident angle or not
		const bool newIncidentAngle = psfBinParams.first[0] != commonData.m_aberration.m_psfParameters.m_incidentAnglesHorizontal.m_min ||
			psfBinParams.first[1] != commonData.m_aberration.m_psfParameters.m_incidentAnglesVertical.m_min;

		if (outputFilterLevel(scene, object, ConvolutionSettings::Detailed, log))
		{
			Debug::log_debug() << "    < " 
				<< "Bin parameters: " << psfBinParams
				<< Debug::end;
		}

		// Compute the PSF
		commonData.m_aberration.m_psfParameters.m_incidentAnglesHorizontal = { psfBinParams.first[0], psfBinParams.first[0], 1 };
		commonData.m_aberration.m_psfParameters.m_incidentAnglesVertical = { psfBinParams.first[1], psfBinParams.first[1], 1 };
		commonData.m_aberration.m_psfParameters.m_objectDistances = { psfBinParams.second, psfBinParams.second, 1 };
		const Aberration::PsfStackComputation computationFlags = 
			(newIncidentAngle ? Aberration::PsfStackComputation_AberrationCoefficients : 0) |
			Aberration::PsfStackComputation_PsfUnits | 
			Aberration::PsfStackComputation_PsfBesselTerms | 
			Aberration::PsfStackComputation_PsfEnzCoefficients |
			Aberration::PsfStackComputation_Psfs;
		Aberration::computePSFStack(scene, commonData.m_aberration, computationFlags);

		// Store the downscaled PSF
		psfs.resize(commonData.m_numChannels);
		for (int channelId = 0; channelId < psfs.size(); ++channelId)
		{
			// Extract the resulting PSF
			auto const& psfParams = commonData.m_aberration.m_psfStack.m_psfEntryParameters[0][0][0][channelId][0][0];
			auto const& psfEntry = commonData.m_aberration.m_psfStack.m_psfs[0][0][0][channelId][0][0];

			// Compute its radius
			const float psfRadius = Aberration::blurRadiusPixels(psfEntry, commonData.m_renderResolution, commonData.m_fovy);

			if (outputFilterLevel(scene, object, ConvolutionSettings::Detailed, log))
			{
				Debug::log_debug() << "    < "
					<< "Channel[" << channelId << "]: "
					<< "PSF Radius: " << psfRadius << " (" << psfEntry.m_psf.cols() << "), "
					<< "Defocus: " << psfParams.m_focus.m_defocusParam 
					<< Debug::end;
			}

			// Actual PSF to convolve with
			psfs[channelId].m_defocus = psfParams.m_focus.m_defocusParam;
			psfs[channelId].m_radius = psfRadius;
			psfs[channelId].m_psf = Aberration::resizePsfNormalized(scene, commonData.m_aberration, psfEntry.m_psf, psfRadius);

			if (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_exportPsfs)
			{
				// Name of the generated psfs
				std::stringstream ss;
				ss << "_c" << channelId;
				ss << "_h" << (psfBinParams.first[0]);
				ss << "_v" << (psfBinParams.first[1]);
				ss << "_m" << (1.0f / psfBinParams.second);
				std::string psfName = ss.str();

				// Export the original PSF image
				{
					Aberration::Psf psf = psfEntry.m_psf / psfEntry.m_psf.maxCoeff();
					std::string filePath = (commonData.m_psfFolderOriginal / ("psf_original" + psfName + ".png")).string();
					Asset::saveImage(scene, filePath, psf);
				}

				// Export the downscaled PSF image
				{
					Aberration::Psf psf = psfs[channelId].m_psf / psfs[channelId].m_psf.maxCoeff();
					std::string filePath = (commonData.m_psfFolderDownscaled / ("psf_downscaled" + psfName + ".png")).string();
					Asset::saveImage(scene, filePath, psf);
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void computePsfBins(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		// Start the PSF bin timer
		commonData.m_timers[ConvolutionPhase::PsfBins].start();

		{
			DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "PSF Bins");

			// Init the PSF bins first
			switch (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_algorithm)
			{
			case ConvolutionSettings::PerPixel:
				initPsfBinsFromPixelDepths(scene, object, commonData);
				break;

			case ConvolutionSettings::PerPixelStack:
				initPsfBinsFromStack(scene, object, commonData);
				break;

			case ConvolutionSettings::DepthLayers:
				initPsfBinsFromStack(scene, object, commonData);
				break;
			}

			// Sort the slices by off-axis angle
			std::sort(commonData.m_psfBinParams.begin(), commonData.m_psfBinParams.end(), [](auto const& a, auto const& b)
			{
				return
					a.first[0] < b.first[0] ||
					(a.first[0] == b.first[0] && a.first[1] < b.first[1]) ||
					(a.first == b.first && a.second < b.second);
			});

			// Build the separate PSF bin param vectors too
			for (auto const& binParams : commonData.m_psfBinParams)
			{
				commonData.m_psfBinHorizontalAngles.push_back(binParams.first[0]);
				commonData.m_psfBinVerticalAngles.push_back(binParams.first[1]);
				commonData.m_psfBinDioptres.push_back(binParams.second);
				commonData.m_psfBinDepths.push_back(1.0f / binParams.second);
			}
		}

		{
			DateTime::ScopedTimer timer(Debug::Info, commonData.m_psfBinParams.size(), DateTime::Seconds, "PSFs");

			// Disable logging to file to avoid generating gigabytes of data during this operation
			if (object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail < ConvolutionSettings::Detailed)
				setFileLogging(scene, object, false);

			// Populate the bins
			Threading::threadedExecuteIndices(
				Threading::ThreadedExecuteParams(1, "Computing PSF bins", "PSF", outputLogLevel(scene, object, ConvolutionSettings::Progress)),
				[&](Threading::ThreadedExecuteEnvironment const& environment, size_t binId)
				{
					computePsf(scene, object, environment, commonData, perThreadData[Threading::currentThreadId()], binId);
				},
				commonData.m_psfBinParams.size());

			// Re-enable file logging
			if (object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail < ConvolutionSettings::Detailed)
				setFileLogging(scene, object, true);
		}

		{
			DateTime::ScopedTimer timer(Debug::Info, commonData.m_psfBinParams.size(), DateTime::Seconds, "Max PSF Size");

			commonData.m_maxBlurRadius = 0;
			for (auto const& psfsAngle: commonData.m_psfBins)
			for (auto const& psfsDepth: psfsAngle.second)
			for (auto const& psf: psfsDepth.second)
			{
				commonData.m_maxBlurRadius = glm::max(commonData.m_maxBlurRadius, size_t(glm::ceil(psf.m_radius) + 1));
			}
		}

		// Stop the PSF bin timer
		commonData.m_timers[ConvolutionPhase::PsfBins].stop();
	}

	////////////////////////////////////////////////////////////////////////////////
	void computePerPixelProperties(Scene::Scene& scene, Scene::Object* object, Threading::ThreadedExecuteEnvironment const& environment, 
		CommonData& commonData, PerThreadData& perThreadData, int img_row, int img_col)
	{
		// Whether we should be logging from this thread or not
		const bool log = shouldLog(scene, object, environment);

		// Pixel id into the buffer
		auto width = commonData.m_renderResolution[0], height = commonData.m_renderResolution[1];

		// Perform the summation for each channel
		for (int channelId = 0; channelId < 3; ++channelId)
		{
			// Extract the pixel's data entry
			auto& pixelData = commonData.m_inputPixels[img_row][img_col][channelId];

			// Extract the PSF
			auto const& centerPixelPsfEntry = psfChannel(commonData, pixelData, channelId);

			// Store the center pixel properties (defocus, blur radius, etc.)
			pixelData.m_defocus = centerPixelPsfEntry.m_defocus;
			pixelData.m_blurRadius = centerPixelPsfEntry.m_radius;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void computePixelProperties(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		// Only do this for the per-pixel algorithm
		if (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_algorithm != ConvolutionSettings::PerPixel)
			return;

		DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Per-Pixel Properties");

		// Compute the per-pixel properties
		Threading::threadedExecuteIndices(
			Threading::ThreadedExecuteParams(Threading::numThreads(), "Processing pixels", "pixel", outputLogLevel(scene, object, ConvolutionSettings::Progress)),
			[&](Threading::ThreadedExecuteEnvironment const& environment, int img_row, int img_col)
			{
				computePerPixelProperties(scene, object, environment, commonData, perThreadData[Threading::currentThreadId()], img_row, img_col);
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0]);
	}
	
	////////////////////////////////////////////////////////////////////////////////
	void gatherPixel(Scene::Scene& scene, Scene::Object* object, Threading::ThreadedExecuteEnvironment const& environment, 
		CommonData& commonData, PerThreadData& threadData, int img_row, int img_col)
	{
		// Whether we should be logging from this thread or not
		const bool log = shouldLog(scene, object, environment);

		// Image dimensions
		auto width = commonData.m_renderResolution[0], height = commonData.m_renderResolution[1];

		// How many pixels to gather
		int gatherRadius = commonData.m_maxBlurRadius;
		int gatherDiameter = gatherRadius * 2 + 1;

		// Reference to the 'main' sample array
		for (int channelId = 0; channelId < 3; ++channelId)
		{
			threadData.m_samples[channelId].clear();	
			threadData.m_samples[channelId].reserve(gatherDiameter * gatherDiameter);
		}

		// Splat the PSF to the result buffer
		for (int sampleRows = 0; sampleRows < gatherDiameter; sampleRows++)
		for (int sampleCols = 0; sampleCols < gatherDiameter; sampleCols++)
		{
			// Pixel indices
			const int targetPixelRows = img_row - gatherRadius + sampleRows;
			const int targetPixelCols = img_col - gatherRadius + sampleCols;
			const int relativeSampleRow = img_row - targetPixelRows;
			const int relativeSampleCol = img_col - targetPixelCols;

			// Make sure we are inside the image
			if (targetPixelCols < 0 || targetPixelCols >= width || targetPixelRows < 0 || targetPixelRows >= height)
				continue;

			// Process each layer and each channel
			for (int sampleChannel = 0; sampleChannel < 3; ++sampleChannel)
			{
				// Extract the pixel's data entry
				auto& targetPixelData = commonData.m_inputPixels[targetPixelRows][targetPixelCols][sampleChannel];

				// Extract the PSF
				auto const& targetPixelPsf = psfChannel(commonData, targetPixelData, sampleChannel).m_psf;
				const int psfRadius = targetPixelPsf.rows() / 2;

				// Make sure the PSF overlaps the center pixel
				if (glm::abs(relativeSampleRow) > psfRadius || glm::abs(relativeSampleCol) > psfRadius)
					continue;

				// Store a new sample
				Sample sample;
				sample.m_color = targetPixelData.m_color;
				sample.m_depth = targetPixelData.m_depth;
				sample.m_weight = targetPixelPsf(size_t(psfRadius + relativeSampleRow), size_t(psfRadius + relativeSampleCol));
				threadData.m_samples[sampleChannel].push_back(sample);
			}
		}

		// Perform the summation for each channel
		for (int channelId = 0; channelId < 3; ++channelId)
		{
			// Sample array for the current channel
			auto& sampleArray = threadData.m_samples[channelId];

			// Sort the array
			if (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_blendMode == ConvolutionSettings::BackToFront)
			{
				std::sort(sampleArray.begin(), sampleArray.end(), [](Sample const& a, Sample const& b)
				{
					// Back to front and we use positive camera space distance (in meters),
					// so sample 'a' comes before if it's depth is greater than the depth of 'b'
					return a.m_depth > b.m_depth;
				});
			}
			else if (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_blendMode == ConvolutionSettings::FrontToBack)
			{
				std::sort(sampleArray.begin(), sampleArray.end(), [](Sample const& a, Sample const& b)
				{
					// Front to back and we use positive camera space distance (in meters),
					// so sample 'a' comes before if it's depth is less than the depth of 'b'
					return a.m_depth < b.m_depth;
				});
			}

			// Compute the convolution
			auto& outPixelData = commonData.m_outputPixels[img_row][img_col][channelId];
			for (auto const& sample : sampleArray)
			{
				// Color and weight for the sample
				const float sampleColor = sample.m_color;
				const float sampleWeight = sample.m_weight;

				switch (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_blendMode)
				{
				// Summed blending
				case ConvolutionSettings::Sum:
					outPixelData.m_result += sampleWeight * sampleColor;
					outPixelData.m_weight += sampleWeight;
					break;

				// Front-to-back blending
				case ConvolutionSettings::FrontToBack:
					outPixelData.m_result += (1.0f - outPixelData.m_weight) * sampleWeight * sampleColor;
					outPixelData.m_weight = sampleWeight + (1.0f - sampleWeight) * outPixelData.m_weight;
					break;

				// Back-to-front blending
				case ConvolutionSettings::BackToFront:
					outPixelData.m_result = sampleWeight * sampleColor + (1.0f - sampleWeight) * outPixelData.m_result;
					outPixelData.m_weight = sampleWeight + (1.0f - sampleWeight) * outPixelData.m_weight;
					break;
				}

				// Add to the number of samples
				++outPixelData.m_numSamples;

				// Check for saturation
				if (outPixelData.m_weight >= 1.0f) break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void convolutionBarsky(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		#ifdef HAS_Matlab

		// PSF bins and MATLAB data factory
		const size_t numDepthSlices = commonData.m_psfBinDepths.size();
		const size_t numChannels = commonData.m_numChannels;
		matlab::data::ArrayFactory factory;

		// Convert the PSF depths to a matlab array
		matlab::data::TypedArray<double> psfDepths = Matlab::vectorToDataArray<double, float>(commonData.m_psfBinDepths);

		// Convert the PSFs to matlab images
		matlab::data::CellArray psfs = factory.createCellArray({ (size_t)numDepthSlices, (size_t)numChannels });
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t psfId, size_t channelId)
			{
				auto const& psf = commonData.m_psfBins[glm::vec2(0.0f)][commonData.m_psfBinDioptres[psfId]][channelId].m_psf;
				psfs[psfId][channelId] = Matlab::eigenToDataArray(psf);
			},
			numDepthSlices, numChannels);

		// Convert the scene images to Matlab images
		matlab::data::TypedArray<unsigned char> colorImage = factory.createArray<unsigned char>({ size_t(commonData.m_renderResolution[1]), size_t(commonData.m_renderResolution[0]), size_t(3) });
		matlab::data::TypedArray<float> depthImage = factory.createArray<float>({ size_t(commonData.m_renderResolution[1]), size_t(commonData.m_renderResolution[0]), size_t(1) });

		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				auto& pixelData = commonData.m_inputPixels[rowId][colId][channel];
				colorImage[rowId][colId][channel] = unsigned char(pixelData.m_color * 255.0f);
				depthImage[rowId][colId][0] = pixelData.m_depth;
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0], 3);

		// Perform the convolution
		auto convolutionResult = Matlab::g_matlab->feval(
			"ground_truth_barsky",
			{
				colorImage,
				depthImage,
				psfDepths,
				psfs
			},
			Matlab::logged_buffer(Debug::Info),
			Matlab::logged_buffer(Debug::Error));

		// Store the results
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				auto& pixelData = commonData.m_outputPixels[rowId][colId][channel];

				pixelData.m_result = convolutionResult[rowId][colId][channel];
				pixelData.m_weight = 1.0f;
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0], 3);

		#endif
	}

	////////////////////////////////////////////////////////////////////////////////
	void convolutionCelaya(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		#ifdef HAS_Matlab

		// The various angles
		auto horizontalAngles = Aberration::generateHorizontalAxes(scene, getAberration(scene, object));
		auto verticalAngles = Aberration::generateVerticalAxes(scene, getAberration(scene, object));
		auto defocusDioptres = Aberration::generateObjectDioptres(scene, getAberration(scene, object));
		auto defocusMeters = Aberration::generateObjectDistances(scene, getAberration(scene, object));

		// PSF bins and MATLAB data factory
		const size_t numHorSlices = horizontalAngles.size();
		const size_t numVertSlices = verticalAngles.size();
		const size_t numDepthSlices = defocusMeters.size();

		const size_t numChannels = commonData.m_numChannels;
		matlab::data::ArrayFactory factory;

		// Convert the PSFs to matlab images
		matlab::data::CellArray psfs = factory.createCellArray({ (size_t)numDepthSlices, (size_t)numChannels, (size_t)numHorSlices, (size_t) numVertSlices });
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t psfId, size_t channelId, size_t horizontalId, size_t verticalId)
			{
				auto const& psfBinParams = std::make_pair(glm::vec2(horizontalAngles[horizontalId], verticalAngles[verticalId]), defocusDioptres[psfId]);
				auto const& psf = commonData.m_psfBins[psfBinParams.first][psfBinParams.second][channelId].m_psf;
				psfs[psfId][channelId][horizontalId][verticalId] = Matlab::eigenToDataArray(psf);
			},
			numDepthSlices, numChannels, numHorSlices, numVertSlices);

		// Convert the PSF axes and depths to a matlab arrays
		std::sort(horizontalAngles.begin(), horizontalAngles.end());
		std::sort(verticalAngles.begin(), verticalAngles.end());
		std::sort(defocusMeters.begin(), defocusMeters.end());

		matlab::data::TypedArray<double> psfHorAngles = Matlab::vectorToDataArray<double, float>(horizontalAngles);
		matlab::data::TypedArray<double> psfVertAngles = Matlab::vectorToDataArray<double, float>(verticalAngles);
		matlab::data::TypedArray<double> psfDepths = Matlab::vectorToDataArray<double, float>(defocusMeters);

		// Convert the scene images to Matlab images
		matlab::data::TypedArray<unsigned char> colorImage = factory.createArray<unsigned char>({ size_t(commonData.m_renderResolution[1]), size_t(commonData.m_renderResolution[0]), size_t(3) });
		matlab::data::TypedArray<float> coordsImage = factory.createArray<float>({ size_t(commonData.m_renderResolution[1]), size_t(commonData.m_renderResolution[0]), size_t(3) });

		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				auto& pixelData = commonData.m_inputPixels[rowId][colId][channel];
				colorImage[rowId][colId][channel] = unsigned char(pixelData.m_color * 255.0f);
				coordsImage[rowId][colId][0] = pixelData.m_incidentAngles.x;
				coordsImage[rowId][colId][1] = pixelData.m_incidentAngles.y;
				coordsImage[rowId][colId][2] = pixelData.m_depth;
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0], 3);

		// Perform the convolution
		auto convolutionResult = Matlab::g_matlab->feval(
			"ground_truth_celaya",
			{
				colorImage,
				coordsImage,
				psfDepths,
				psfHorAngles,
				psfVertAngles,
				psfs
			},
			Matlab::logged_buffer(Debug::Info),
			Matlab::logged_buffer(Debug::Error));

		// Store the results
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				auto& pixelData = commonData.m_outputPixels[rowId][colId][channel];

				pixelData.m_result = convolutionResult[rowId][colId][channel];
				pixelData.m_weight = 1.0f;
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0], 3);

		#endif
	}

	////////////////////////////////////////////////////////////////////////////////
	void convolutionGonzalez(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		#ifdef HAS_Matlab

		// The various angles
		auto horizontalAngles = Aberration::generateHorizontalAxes(scene, getAberration(scene, object));
		auto verticalAngles = Aberration::generateVerticalAxes(scene, getAberration(scene, object));
		auto defocusDioptres = Aberration::generateObjectDioptres(scene, getAberration(scene, object));
		auto defocusMeters = Aberration::generateObjectDistances(scene, getAberration(scene, object));

		// PSF bins and MATLAB data factory
		const size_t numHorSlices = horizontalAngles.size();
		const size_t numVertSlices = verticalAngles.size();
		const size_t numDepthSlices = defocusMeters.size();

		const size_t numChannels = commonData.m_numChannels;
		matlab::data::ArrayFactory factory;

		// Convert the PSFs to matlab images
		matlab::data::CellArray psfs = factory.createCellArray({ (size_t)numDepthSlices, (size_t)numChannels, (size_t)numHorSlices, (size_t)numVertSlices });
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t psfId, size_t channelId, size_t horizontalId, size_t verticalId)
			{
				auto const& psfBinParams = std::make_pair(glm::vec2(horizontalAngles[horizontalId], verticalAngles[verticalId]), defocusDioptres[psfId]);
				auto const& psf = commonData.m_psfBins[psfBinParams.first][psfBinParams.second][channelId].m_psf;
				psfs[psfId][channelId][horizontalId][verticalId] = Matlab::eigenToDataArray(psf);
			},
			numDepthSlices, numChannels, numHorSlices, numVertSlices);

		// Convert the PSF axes and depths to a matlab arrays
		std::sort(horizontalAngles.begin(), horizontalAngles.end());
		std::sort(verticalAngles.begin(), verticalAngles.end());
		std::sort(defocusMeters.begin(), defocusMeters.end());

		matlab::data::TypedArray<double> psfHorAngles = Matlab::vectorToDataArray<double, float>(horizontalAngles);
		matlab::data::TypedArray<double> psfVertAngles = Matlab::vectorToDataArray<double, float>(verticalAngles);
		matlab::data::TypedArray<double> psfDepths = Matlab::vectorToDataArray<double, float>(defocusMeters);

		// Convert the scene images to Matlab images
		matlab::data::TypedArray<unsigned char> colorImage = factory.createArray<unsigned char>({ size_t(commonData.m_renderResolution[1]), size_t(commonData.m_renderResolution[0]), size_t(3) });
		matlab::data::TypedArray<float> coordsImage = factory.createArray<float>({ size_t(commonData.m_renderResolution[1]), size_t(commonData.m_renderResolution[0]), size_t(3) });

		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				auto& pixelData = commonData.m_inputPixels[rowId][colId][channel];
				colorImage[rowId][colId][channel] = unsigned char(pixelData.m_color * 255.0f);
				coordsImage[rowId][colId][0] = pixelData.m_incidentAngles.x;
				coordsImage[rowId][colId][1] = pixelData.m_incidentAngles.y;
				coordsImage[rowId][colId][2] = pixelData.m_depth;
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0], 3);

		// Perform the convolution
		auto convolutionResult = Matlab::g_matlab->feval(
			"ground_truth_gonzalez",
			{
				colorImage,
				coordsImage,
				psfDepths,
				psfHorAngles,
				psfVertAngles,
				psfs
			},
			Matlab::logged_buffer(Debug::Info),
			Matlab::logged_buffer(Debug::Error));

		// Store the results
		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				auto& pixelData = commonData.m_outputPixels[rowId][colId][channel];

				pixelData.m_result = convolutionResult[rowId][colId][channel];
				pixelData.m_weight = 1.0f;
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0], 3);

		#endif
	}

	////////////////////////////////////////////////////////////////////////////////
	void convolutionPerPixel(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		// Perform the actual convolution
		Threading::threadedExecuteIndices(
			Threading::ThreadedExecuteParams(Threading::numThreads(), "Convolving pixels", "pixel", outputLogLevel(scene, object, ConvolutionSettings::Progress)),
			[&](Threading::ThreadedExecuteEnvironment const& environment, int img_row, int img_col)
			{
				// Extract the thread data corresponding to this thread
				PerThreadData& threadData = perThreadData[Threading::currentThreadId()];

				// Convolve with the PSF by gathering
				gatherPixel(scene, object, environment, commonData, perThreadData[Threading::currentThreadId()], img_row, img_col);
			},
			commonData.m_renderResolution[1], commonData.m_renderResolution[0]);
	}

	////////////////////////////////////////////////////////////////////////////////
	void convolutionPerPixelStack(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		convolutionCelaya(scene, object, commonData, perThreadData);
	}

	////////////////////////////////////////////////////////////////////////////////
	void convolutionDepthLayers(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		if (commonData.m_offAxis)
			convolutionGonzalez(scene, object, commonData, perThreadData);
		else
			convolutionBarsky(scene, object, commonData, perThreadData);
	}

	////////////////////////////////////////////////////////////////////////////////
	void doConvolution(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData)
	{
		DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Convolution");

		// Start the convolution timer
		commonData.m_timers[ConvolutionPhase::Convolution].start();

		switch (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_algorithm)
		{
		case ConvolutionSettings::PerPixel:
			convolutionPerPixel(scene, object, commonData, perThreadData);
			break;

		case ConvolutionSettings::PerPixelStack:
			convolutionPerPixelStack(scene, object, commonData, perThreadData);
			break;

		case ConvolutionSettings::DepthLayers:
			convolutionDepthLayers(scene, object, commonData, perThreadData);
			break;
		}

		// Stop the convolution timer
		commonData.m_timers[ConvolutionPhase::Convolution].stop();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getCurrentScene(Scene::Scene& scene)
	{
		return std::string_replace_first(SimulationSettings::getActiveGroup(scene, "Scene"), "Scene_", "");
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getCurrentCamera(Scene::Scene& scene)
	{
		return RenderSettings::getMainCamera(scene)->m_name;
	}

	////////////////////////////////////////////////////////////////////////////////
	Results& initResults(Scene::Scene& scene, Scene::Object* object, Aberration::WavefrontAberration const& aberration)
	{
		// Update the output
		object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode = Results::Convolution;
		object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultRenderId = object->component<GroundTruthAberrationComponent>().m_results.size();

		// Create a new results object
		auto& results = object->component<GroundTruthAberrationComponent>().m_results.emplace_back();

		// Extract the necessary scene objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

		// Resolution the effect is rendered at
		glm::ivec2 renderResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;

		// Timestamps
		results.m_timestampDisplay = DateTime::getDateStringUtf8(DateTime::dateFormatDisplay());
		results.m_timestampFile = DateTime::getDateStringUtf8(DateTime::dateFormatFilename());

		// Input settings
		results.m_scene = getCurrentScene(scene);
		results.m_camera = camera->m_name;
		results.m_aberration = aberration.m_name;
		results.m_width = renderResolution[0];
		results.m_height = renderResolution[1];

		// Simulation settings
		results.m_channels = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_numChannels;
		results.m_dioptresPrecision = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_dioptresPrecision;
		results.m_incidentAnglesPrecision = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_incidentAnglesPrecision;
		results.m_centerIncidentAngles = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_centerIncidentAngles;
		results.m_centerDioptres = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_centerDioptres;
		results.m_blendMode = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_blendMode;
		results.m_algorithm = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_algorithm;
		results.m_numDepthSlices = aberration.m_psfParameters.m_objectDistances.m_numSteps;
		results.m_simulateOffAxis = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_simulateOffAxis;
		results.m_isHdr = object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_inputDynamicRange == ConvolutionSettings::HDR;

		// Camera properties
		results.m_focusDistance = camera->component<Camera::CameraComponent>().m_focusDistance;
		results.m_apertureDiameter = camera->component<Camera::CameraComponent>().m_fixedAperture;

		Debug::log_info() << "Initiating aberration simulation..." << Debug::end;
		Debug::log_info() << "  - Scene: " << results.m_scene << Debug::end;
		Debug::log_info() << "  - Camera: " << results.m_camera << Debug::end;
		Debug::log_info() << "  - Aberration: " << results.m_aberration << Debug::end;
		Debug::log_info() << "  - Algorithm: " << ConvolutionSettings::Algorithm_value_to_string(results.m_algorithm) << Debug::end;
		Debug::log_info() << "  - Channels: " << results.m_channels << Debug::end;
		Debug::log_info() << "  - Off-axis: " << results.m_simulateOffAxis << Debug::end;
		Debug::log_info() << "  - Bin Precision (Dioptres): " << results.m_dioptresPrecision << Debug::end;
		Debug::log_info() << "  - Bin Precision (Incident Angles): " << results.m_incidentAnglesPrecision << Debug::end;
		Debug::log_info() << "  - Pupil Diameter: " << results.m_apertureDiameter << Debug::end;
		Debug::log_info() << "  - Dynamic Range: " << ConvolutionSettings::InputDynamicRange_value_to_string(results.m_isHdr ? ConvolutionSettings::HDR : ConvolutionSettings::LDR) << Debug::end;

		// Return the results
		return results;
	}

	////////////////////////////////////////////////////////////////////////////////
	void produceResult(Scene::Scene& scene, Scene::Object* object, CommonData& commonData, std::vector<PerThreadData>& perThreadData, Results& results)
	{
		// Image dimensions
		auto width = commonData.m_renderResolution[0], height = commonData.m_renderResolution[1];

		{
			DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Final Results");

			for (int row = 0; row < height; ++row)
			for (int col = 0; col < width; ++col)
			for (int channel = 0; channel < 3; ++channel)
			{
				auto const& inputPixelData = commonData.m_inputPixels[row][col][channel];
				auto const& outputPixelData = commonData.m_outputPixels[row][col][channel];

				results.m_minDepth = glm::min(results.m_minDepth, inputPixelData.m_depth);
				results.m_maxDepth = glm::max(results.m_maxDepth, inputPixelData.m_depth);

				results.m_minBlurRadius[channel] = glm::min(results.m_minBlurRadius[channel], inputPixelData.m_blurRadius);
				results.m_minBlurRadius[3] = glm::min(results.m_minBlurRadius[3], inputPixelData.m_blurRadius);
				results.m_maxBlurRadius[channel] = glm::max(results.m_maxBlurRadius[channel], inputPixelData.m_blurRadius);
				results.m_maxBlurRadius[3] = glm::max(results.m_maxBlurRadius[3], inputPixelData.m_blurRadius);

				results.m_minDefocus[channel] = glm::min(results.m_minDefocus[channel], inputPixelData.m_defocus);
				results.m_minDefocus[3] = glm::min(results.m_minDefocus[3], inputPixelData.m_defocus);
				results.m_maxDefocus[channel] = glm::max(results.m_maxDefocus[channel], inputPixelData.m_defocus);
				results.m_maxDefocus[3] = glm::max(results.m_maxDefocus[3], inputPixelData.m_defocus);

				results.m_minNumSamples[channel] = glm::min(results.m_minNumSamples[channel], outputPixelData.m_numSamples);
				results.m_minNumSamples[3] = glm::min(results.m_minNumSamples[3], outputPixelData.m_numSamples);
				results.m_maxNumSamples[channel] = glm::max(results.m_maxNumSamples[channel], outputPixelData.m_numSamples);
				results.m_maxNumSamples[3] = glm::max(results.m_maxNumSamples[3], outputPixelData.m_numSamples);

				results.m_minWeight[channel] = glm::min(results.m_minWeight[channel], outputPixelData.m_weight);
				results.m_minWeight[3] = glm::min(results.m_minWeight[3], outputPixelData.m_weight);
				results.m_maxWeight[channel] = glm::max(results.m_maxWeight[channel], outputPixelData.m_weight);
				results.m_maxWeight[3] = glm::max(results.m_maxWeight[3], outputPixelData.m_weight);
			}

			// Alloc space in the output vectors
			for (auto& texture : s_allTextures)
				allocEmptyTexture(scene, results, results.m_textures[texture.first]);

			// Write data into the arrays
			Threading::threadedExecuteIndices(Threading::numThreads(),
				[&](Threading::ThreadedExecuteEnvironment const& environment, size_t imgRow, size_t imgCol, size_t channel)
				{
					size_t arrayId = (imgRow * width + imgCol) * 4 + channel;

					auto const& inputPixelData = commonData.m_inputPixels[imgRow][imgCol][channel];
					auto const& outputPixelData = commonData.m_outputPixels[imgRow][imgCol][channel];

					#define FTOUI(X) (unsigned char((X) * 255.0f))

					// Store the generated images
					results.m_textures[Results::Original][arrayId] = FTOUI(inputPixelData.m_color);
					results.m_textures[Results::Convolution][arrayId] = FTOUI(outputPixelData.m_result / outputPixelData.m_weight);

					// Write out the per-pixel properties
					const glm::vec4 angles = glm::abs(glm::vec4(inputPixelData.m_incidentAngles / glm::degrees(commonData.m_fov * 0.5f), 0.0f, 0.0f));
					const glm::vec4 binAngles = glm::abs(glm::vec4(inputPixelData.m_psfBinParams.first / glm::degrees(commonData.m_fov * 0.5f), 0.0f, 0.0f));
					results.m_textures[Results::BlurRadius][arrayId] = FTOUI(inputPixelData.m_blurRadius / results.m_maxBlurRadius[3]);
					results.m_textures[Results::Normalization][arrayId] = FTOUI(outputPixelData.m_weight / results.m_maxWeight[3]);
					results.m_textures[Results::NumberOfSamples][arrayId] = FTOUI(outputPixelData.m_numSamples / float(results.m_maxNumSamples[3]));
					results.m_textures[Results::IncidentAngles][arrayId] = FTOUI(angles[channel]);
					results.m_textures[Results::Defocus][arrayId] = FTOUI(inputPixelData.m_defocus / results.m_maxDefocus[3]);
					results.m_textures[Results::BinIncidentAngles][arrayId] = FTOUI(binAngles[channel]);
					results.m_textures[Results::BinDefocus][arrayId] = FTOUI(inputPixelData.m_psfBinParams.second / results.m_maxDefocus[3]);
					results.m_textures[Results::Depth][arrayId] = FTOUI(inputPixelData.m_depth / results.m_maxDepth);
				},
				height, width, 3);

			// Store the number of bins
			results.m_numBins = std::accumulate(commonData.m_psfBins.begin(), commonData.m_psfBins.end(), size_t(0), [](const size_t a, auto const& b)
				{ return a + b.second.size(); });

			// Stop the total processing time timer
			commonData.m_timers[ConvolutionPhase::Total].stop();

			// Write out the elapsed times
			results.m_psfBinTime = commonData.m_timers[ConvolutionPhase::PsfBins].getElapsedTime();
			results.m_convolutionTime = commonData.m_timers[ConvolutionPhase::Convolution].getElapsedTime();
			results.m_totalProcessingTime = commonData.m_timers[ConvolutionPhase::Total].getElapsedTime();
		}

		{
			DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Final Results");

			// Upload the results
			uploadResultRender(scene, object, results);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void saveResults(Scene::Scene& scene, Scene::Object* object, Results& results, ResultsTextures const& savedTextures,
		std::filesystem::path const& outFolder, std::string const& attribsPrefix, std::string const& imagePrefix)
	{
		{
			DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Saving Images");

			// Export the generated textures
			for (auto& texture : s_allTextures)
			{
				std::string const& fileName = getTextureName(texture.first, imagePrefix);
				std::string const& filePath = (outFolder / fileName).string();
				Asset::saveImage(scene, filePath, results.m_width, results.m_height, 4, results.m_textures[texture.first].data(), true);
			}
		}

		{
			DateTime::ScopedTimer timer(Debug::Info, 1, DateTime::Seconds, "Saving Metadata");

			// Write out the result attributes
			std::ordered_pairs_in_blocks attributes;
			attributes["Resolution"].push_back({ "Width", std::to_string(results.m_width) });
			attributes["Resolution"].push_back({ "Height", std::to_string(results.m_height) });

			attributes["Settings"].push_back({ "TimestampDisplay", results.m_timestampDisplay });
			attributes["Settings"].push_back({ "TimestampFile", results.m_timestampFile });
			attributes["Settings"].push_back({ "Scene", results.m_scene });
			attributes["Settings"].push_back({ "Camera", results.m_camera });
			attributes["Settings"].push_back({ "Aberration", results.m_aberration });
			attributes["Settings"].push_back({ "Channels", std::to_string(results.m_channels) });
			attributes["Settings"].push_back({ "Off-Axis", std::to_string(results.m_simulateOffAxis) });
			attributes["Settings"].push_back({ "HDR", std::to_string(results.m_isHdr) });
			attributes["Settings"].push_back({ "Focus", std::to_string(results.m_focusDistance) });
			attributes["Settings"].push_back({ "Aperture", std::to_string(results.m_apertureDiameter) });
			attributes["Settings"].push_back({ "BlendMode", std::to_string(ConvolutionSettings::BlendMode_value_to_string(results.m_blendMode)) });
			attributes["Settings"].push_back({ "Algorithm", std::to_string(ConvolutionSettings::Algorithm_value_to_string(results.m_algorithm)) });

			attributes["Bins"].push_back({ "NumBins", std::to_string(results.m_numBins) });
			attributes["Bins"].push_back({ "DioptresPrecision", std::to_string(results.m_dioptresPrecision) });
			attributes["Bins"].push_back({ "IncidentAnglesPrecision", std::to_string(results.m_incidentAnglesPrecision) });
			attributes["Bins"].push_back({ "CenterDioptres", std::to_string(results.m_centerDioptres) });
			attributes["Bins"].push_back({ "CenterIncidentAngles", std::to_string(results.m_centerIncidentAngles) });

			attributes["Limits"].push_back({ "MinDepth", std::to_string(results.m_minDepth) });
			attributes["Limits"].push_back({ "MaxDepth", std::to_string(results.m_maxDepth) });
			attributes["Limits"].push_back({ "MinDefocus", std::to_string(results.m_minDefocus) });
			attributes["Limits"].push_back({ "MaxDefocus", std::to_string(results.m_maxDefocus) });
			attributes["Limits"].push_back({ "MinBlurRadius", std::to_string(results.m_minBlurRadius) });
			attributes["Limits"].push_back({ "MaxBlurRadius", std::to_string(results.m_maxBlurRadius) });
			attributes["Limits"].push_back({ "MinNumSamples", std::to_string(results.m_minNumSamples) });
			attributes["Limits"].push_back({ "MaxNumSamples", std::to_string(results.m_maxNumSamples) });
			attributes["Limits"].push_back({ "MinWeight", std::to_string(results.m_minWeight) });
			attributes["Limits"].push_back({ "MaxWeight", std::to_string(results.m_maxWeight) });

			attributes["Metrics"].push_back({ "MSSIM", std::to_string(results.m_mssim) });
			attributes["Metrics"].push_back({ "MMSSIM", std::to_string(results.m_mmssim) });
			attributes["Metrics"].push_back({ "HDRVDP3", std::to_string(results.m_hdrvdp) });
			attributes["Metrics"].push_back({ "PSNR", std::to_string(results.m_psnr) });
			attributes["Metrics"].push_back({ "SNR", std::to_string(results.m_snr) });
			attributes["Metrics"].push_back({ "RMSE", std::to_string(results.m_rmse) });
			attributes["Metrics"].push_back({ "MSE", std::to_string(results.m_mse) });
			attributes["Metrics"].push_back({ "MPSNR", std::to_string(results.m_mpsnr) });
			attributes["Metrics"].push_back({ "MSNR", std::to_string(results.m_msnr) });
			attributes["Metrics"].push_back({ "MRMSE", std::to_string(results.m_mrmse) });
			attributes["Metrics"].push_back({ "MMSE", std::to_string(results.m_mmse) });

			attributes["RunningTimes"].push_back({ "PsfBins", std::to_string(results.m_psfBinTime) });
			attributes["RunningTimes"].push_back({ "Convolution", std::to_string(results.m_convolutionTime) });
			attributes["RunningTimes"].push_back({ "TotalProcessing", std::to_string(results.m_totalProcessingTime) });

			Debug::log_debug() << "Attribs file prefix: " << attribsPrefix << Debug::end;

			std::string const& attribsFileName = (attribsPrefix.empty() ? ""s : attribsPrefix + "_"s) + "attributes.ini";
			Asset::savePairsInBlocks(scene, (outFolder / attribsFileName).string(), attributes);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void computeMetrics(Scene::Scene& scene, Scene::Object* object, bool isResultGT, int resultId, std::string const& outFolder, std::string const& outFilePrefix, 
		bool computePsnr, bool computeSsim, bool computeHdrvdp)
	{
		Debug::log_info() << "Computing metrics..." << Debug::end;

		// Extract the metric evaluation parameters
		auto& metricSettings = object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings;

		// Extract the result that we are using
		auto& results = getDisplayResults(scene, object);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Extract the necessary scene objects
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Id of the current gbuffer
		int gbufferId = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite;

		// Resolution the effect is rendered at
		glm::ivec2 renderResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		auto width = renderResolution[0], height = renderResolution[1];
		size_t numPixelsRender = width * height;

		// Reference to the necessary texture data
		auto& original = results.m_textures[Results::Original];
		auto& reference = results.m_textures[Results::Reference];
		auto& convolution = results.m_textures[Results::Convolution];
		auto& ssimPc = results.m_textures[Results::Ssim];
		auto& ssimJet = results.m_textures[Results::SsimJet];
		auto& hdrvdp = results.m_textures[Results::HdrVdp3];
		auto& hdrvdpJet = results.m_textures[Results::HdrVdp3Jet];
		auto& diff = results.m_textures[Results::Difference];


		// Extract the color and depth buffers
		if (isResultGT)
		{
			auto& referenceResults = object->component<GroundTruthAberrationComponent>().m_results[resultId];
			auto& referencOutput = referenceResults.m_textures[Results::Convolution];

			std::memcpy(reference.data(), referencOutput.data(), numPixelsRender * 4 * sizeof(unsigned char));
		}
		else
		{
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glGetTextureSubImage(scene.m_gbuffer[gbufferId].m_colorTextures[scene.m_gbuffer[gbufferId].m_readBuffer], 0, 0, 0, 0, width, height, 1,
				GL_RGBA, GL_UNSIGNED_BYTE, numPixelsRender * 4 * sizeof(unsigned char), reference.data());
		}

		#ifdef HAS_Matlab

		// Convert the scene images to Matlab images
		matlab::data::ArrayFactory factory;
		matlab::data::TypedArray<unsigned char> referenceImage = factory.createArray<unsigned char>({ size_t(results.m_height), size_t(results.m_width), size_t(3) });
		matlab::data::TypedArray<unsigned char> resultImage = factory.createArray<unsigned char>({ size_t(results.m_height), size_t(results.m_width), size_t(3) });

		Threading::threadedExecuteIndices(Threading::numThreads(),
			[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channel)
			{
				const size_t pixelId = (rowId * results.m_width + colId) * 4 + channel;
				referenceImage[rowId][colId][channel] = reference[pixelId];
				resultImage[rowId][colId][channel] = convolution[pixelId];
			},
			results.m_height, results.m_width, 3);

		if (computeSsim)
		{
			Profiler::ScopedCpuPerfCounter(scene, "SSIM");

			Debug::log_info() << "  - SSIM..." << Debug::end;

			// Compute the ssim
			auto ssimResult = Matlab::g_matlab->feval(
				"compute_ssim",
				4,
				{
					resultImage,
					referenceImage
				},
				Matlab::logged_buffer(Debug::Info),
				Matlab::logged_buffer(Debug::Error));

			// Store the mean ssim
			auto const& mssims = ssimResult[1];
			results.m_mssim = glm::vec3(float(mssims[0]), float(mssims[1]), float(mssims[2]));
			results.m_mmssim = (results.m_mssim[0] + results.m_mssim[1] + results.m_mssim[2]) / 3.0f;

			// Also store the ssim map
			auto const& ssimMapPc = ssimResult[2];
			auto const& ssimMapJet = ssimResult[3];

			Threading::threadedExecuteIndices(Threading::numThreads(),
				[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId)
				{
					size_t arrayId = (rowId * results.m_width + colId) * 4;

					for (size_t c = 0; c < 3; ++c)
					{
						float ssimPcPixel = ssimMapPc[rowId][colId][c];
						float ssimJetPixel = ssimMapJet[rowId][colId][c];
						ssimPc[arrayId + c] = unsigned char(ssimPcPixel * 255.0f);
						ssimJet[arrayId + c] = unsigned char(ssimJetPixel * 255.0f);
					}
				},
				results.m_height, results.m_width);
		}

		if (computeHdrvdp)
		{
			Profiler::ScopedCpuPerfCounter(scene, "HDRVDP3");

			Debug::log_info() << "  - HDR-VDP3..." << Debug::end;

			// Compute the hdrvdp
			auto hdrvdpResult = Matlab::g_matlab->feval(
				"compute_hdrvdp",
				3,
				{
					resultImage,
					referenceImage,
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpPeakLuminance),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpContrastRatio),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpGamma),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpAmbientLight),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpDisplaySize),
					Matlab::eigenToDataArray(Eigen::Map<Eigen::RowVectorXi>(glm::value_ptr(metricSettings.m_hdrvdpDisplayResolution), 1, 2).cast<double>()),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpViewDistance),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpSurround),
					Matlab::scalarToDataArray<double>(metricSettings.m_hdrvdpSensitivityCorrection),
				},
				Matlab::logged_buffer(Debug::Info),
				Matlab::logged_buffer(Debug::Error));

			// Store the mean ssim
			results.m_hdrvdp = hdrvdpResult[0][0];

			// Also store the ssim map
			auto const& hdrvdpMapPc = hdrvdpResult[1];
			auto const& hdrvdpMapJet = hdrvdpResult[2];

			Threading::threadedExecuteIndices(Threading::numThreads(),
				[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId)
				{
					size_t arrayId = (rowId * results.m_width + colId) * 4;

					for (size_t c = 0; c < 3; ++c)
					{
						float hdrvdpPcPixel = hdrvdpMapPc[rowId][colId];
						float hdrvdpJetPixel = hdrvdpMapJet[rowId][colId][c];
						hdrvdp[arrayId + c] = unsigned char(hdrvdpPcPixel * 255.0f);
						hdrvdpJet[arrayId + c] = unsigned char(hdrvdpJetPixel * 255.0f);
					}
				},
				results.m_height, results.m_width);
		}

		if (computePsnr)
		{
			Profiler::ScopedCpuPerfCounter(scene, "PSNR");

			Debug::log_info() << "  - PSNR..." << Debug::end;

			// Compute the hdrvdp
			auto psnrResult = Matlab::g_matlab->feval(
				"compute_psnr",
				2,
				{
					resultImage,
					referenceImage
				},
				Matlab::logged_buffer(Debug::Info),
				Matlab::logged_buffer(Debug::Error));

			Debug::log_debug() << "Result dimensions: " << Debug::end;
			for (auto const& result : psnrResult)
			{
				auto const& dimensions = result.getDimensions();
				for (size_t i = 0; i < dimensions.size(); ++i)
				{
					if (i > 0) Debug::log_debug() << " x ";
					Debug::log_debug() << dimensions[i];
				}
				Debug::log_debug() << Debug::end;
			}

			// Store the PSNR
			auto const& psnr = psnrResult[0];
			results.m_psnr = glm::vec3(float(psnr[0]), float(psnr[1]), float(psnr[2]));
			results.m_mpsnr = (results.m_psnr[0] + results.m_psnr[1] + results.m_psnr[2]) / 3.0f;

			// Store the SNR
			auto const& snr = psnrResult[1];
			results.m_snr = glm::vec3(float(snr[0]), float(snr[1]), float(snr[2]));
			results.m_msnr = (results.m_snr[0] + results.m_snr[1] + results.m_snr[2]) / 3.0f;
		}
		#endif

		{
			Profiler::ScopedCpuPerfCounter(scene, "MSE");

			Debug::log_info() << "  - MSE..." << Debug::end;

			Threading::threadedExecuteIndices(Threading::numThreads(),
				[&](Threading::ThreadedExecuteEnvironment const& environment, size_t rowId, size_t colId, size_t channelId)
				{
					size_t arrayId = (rowId * results.m_width + colId) * 4 + channelId;

					float difference = glm::abs(float(convolution[arrayId]) - float(reference[arrayId])) / 255.0f;

					diff[arrayId] = unsigned char(difference * 255.0f);
				},
				results.m_height, results.m_width, 3);

			// Compute the MSE
			results.m_mse = glm::vec3(0.0f);
			for (size_t rowId = 0; rowId < results.m_height; ++rowId)
			for (size_t colId = 0; colId < results.m_width; ++colId)
			for (size_t channelId = 0; channelId < 3; ++channelId)
			{
				size_t arrayId = (rowId * results.m_width + colId) * 4 + channelId;
				results.m_mse[channelId] += glm::pow(diff[arrayId], 2.0f);

			}
			results.m_mse = results.m_mse / float(results.m_width * results.m_height);

			// Compute the RMSE
			results.m_rmse = glm::sqrt(results.m_mse);

			// Compute the per-channel MSE and RMSE
			results.m_mmse = (results.m_mse[0] + results.m_mse[1] + results.m_mse[2]) / 3.0f;
			results.m_mrmse = (results.m_rmse[0] + results.m_rmse[1] + results.m_rmse[2]) / 3.0f;
		}

		Debug::log_info() << "Metrics successfully computed" << Debug::end;

		Debug::log_info() << "Uploading results..." << Debug::end;

		// Upload the results
		uploadResultRender(scene, object, results);

		// Write the result
		if (object->component<GroundTruthAberrationComponent>().m_metricSettings.m_exportMetrics)
		{
			std::filesystem::path resultsFolder = (EnginePaths::generatedFilesFolder() / "GroundTruthAberration" / outFolder);
			saveResults(scene, object, results, s_metricsTextures, resultsFolder, outFilePrefix, outFilePrefix);
		}

		Debug::log_info() << "Results uploaded." << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initiateComputeMetrics(Scene::Scene& scene, Scene::Object* object, std::string const& aberrationType, 
		std::string const& outFolder, std::string const& fileNamePrefix, int resultId, bool psnr, bool ssim, bool hdrvdp, bool display)
	{
		const bool isResultGT = aberrationType == "Aberration_GroundTruth";

		SimulationSettings::enableGroup(scene, aberrationType, true);
		DelayedJobs::postJob(scene, object, "Compute Metrics", true, 3, 
			[isResultGT, resultId, outFolder, fileNamePrefix, psnr, ssim, hdrvdp](Scene::Scene& scene, Scene::Object& object)
		{
			GroundTruthAberration::computeMetrics(scene, &object, isResultGT, resultId, outFolder, fileNamePrefix, psnr, ssim, hdrvdp);
			SimulationSettings::enableGroup(scene, "Aberration_GroundTruth", true);
		});

		if (display) object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode = Results::HdrVdp3Jet;
	}

	////////////////////////////////////////////////////////////////////////////////
	void simulateAberration(Scene::Scene& scene, Scene::Object* object, Aberration::WavefrontAberration const& aberration)
	{
		// Result of the convolution
		auto& results = initResults(scene, object, aberration);

		// Common and per-thread data
		CommonData commonData;
		std::vector<PerThreadData> perThreadData(Threading::numThreads());

		// Common, global payload
		prepareCommonData(scene, object, commonData, results, aberration);

		// Per-thread data
		preparePerThreadData(scene, object, commonData, perThreadData);

		// Compute the psfs for each bin
		computePsfBins(scene, object, commonData, perThreadData);

		// Compute the per-pixel properties for each pixel
		computePixelProperties(scene, object, commonData, perThreadData);

		// Perform convolution
		doConvolution(scene, object, commonData, perThreadData);

		// Derive some properties of the result and prepare the textures
		produceResult(scene, object, commonData, perThreadData, results);

		// Cleanup the per-thread data
		cleanupPerThreadData(scene, object, commonData, perThreadData);

		// Cleanup the common data
		cleanupCommonData(scene, object, commonData, results, aberration);

		// Save the results as well
		saveResults(scene, object, results, s_resultsTextures, commonData.m_resultsFolder, "", commonData.m_resultsPrefix);
	}

	////////////////////////////////////////////////////////////////////////////////
	void simulateAberrations(Scene::Scene& scene, Scene::Object* object, bool batch)
	{
		Aberration::WavefrontAberration aberration = getAberration(scene, object);

		// No batching: process the configured aberration
		if (!batch)
		{
			simulateAberration(scene, object, aberration);
		}

		// Batching: go through each preset and process them
		else
		{
			for (auto const& preset : getAberrationPresets(scene, object))
			{
				if (EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_BatchConvolve_" + preset.first))
				{
					// Write out the preset's proeprties
					aberration.m_name = preset.second.m_name;
					aberration.m_aberrationParameters = preset.second.m_aberrationParameters;

					// Simulate aberration with the modified aberration
					simulateAberration(scene, object, aberration);
				}
			}
		}

		// Mark the result for display
		object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode = Results::Convolution;
		InputSettings::enableInput(scene, false);

		// Play a notification sound
		Sound::playSystemSound("Notification.Default");

		// Suspend the system
		if (object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_sleepWhenDone)
		{
			System::sleep();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void setToneMapperState(Scene::Scene& scene, bool enable)
	{
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		std::string const& fnName = "Tone Map";
		auto& disabledFns = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_disabledFunctions;
		if (enable)
		{
			disabledFns.insert(fnName);
		}
		else
		{
			if (auto it = disabledFns.find(fnName); it != disabledFns.end())
				disabledFns.erase(it);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void initiateSimulateAberration(Scene::Scene& scene, Scene::Object* object, bool batch)
	{
		// Disable tone mapping for HDR inputs
		if (object->component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_inputDynamicRange == ConvolutionSettings::HDR)
			setToneMapperState(scene, false);

		object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode = Results::Scene;
		DelayedJobs::postJob(scene, object, "Aberration Simulation", false, 3, [=](Scene::Scene& scene, Scene::Object& object)
		{
			GroundTruthAberration::simulateAberrations(scene, &object, batch);

			// Re-enable tone mapping for HDR inputs
			if (object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_inputDynamicRange == ConvolutionSettings::HDR)
				setToneMapperState(scene, true);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode != Results::Scene &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Make sure the selected output is valid
		if (!isDisplayResultValid(scene, object)) return false;

		// Filter
		return !getDisplayResults(scene, object).m_isHdr &&
			renderObjectPreconditionOpenGL(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Make sure the selected output is valid
		if (!isDisplayResultValid(scene, object)) return false;

		// Filter
		return getDisplayResults(scene, object).m_isHdr &&
			renderObjectPreconditionOpenGL(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Bind the new buffer
		RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings);
		RenderSettings::setupViewportOpenGL(scene, simulationSettings, renderSettings);
		Scene::bindShader(scene, "Misc", "blit_texture");

		// Bind the scene texture
		glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
		glBindTexture(GL_TEXTURE_2D, scene.m_textures[object->m_name + "_" + s_allTextures[object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode]].m_texture);

		// Render the fullscreen quad
		RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsOutFolderBase(Scene::Scene& scene, Scene::Object* object)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream outFolder;
		outFolder << "Metrics" << "/";
		//outFolder << results.m_scene << "_";
		outFolder << results.m_camera << "_";
		outFolder << ConvolutionSettings::Algorithm_value_to_string(results.m_algorithm) << "_";
		outFolder << results.m_aberration << "_";
		return outFolder.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsOutFolderComplexPhasor(Scene::Scene& scene, Scene::Object* object, Scene::Object* complexBlur)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream outFolder;
		outFolder << getMetricsOutFolderBase(scene, object);
		outFolder << "ComplexPhasor" << "_";
		outFolder << complexBlur->component<ComplexBlur::ComplexBlurComponent>().m_numComponents << "_";
		outFolder << results.m_timestampFile;
		return outFolder.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsOutFolderTiledSplat(Scene::Scene& scene, Scene::Object* object, Scene::Object* tiledSplatBlur)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream outFolder;
		outFolder << getMetricsOutFolderBase(scene, object);
		outFolder << "TiledSplatBlur" << "_";
		outFolder << TiledSplatBlur::TiledSplatBlurComponent::PsfAxisMethod_value_to_string(
			tiledSplatBlur->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod) << "_";
		outFolder << TiledSplatBlur::TiledSplatBlurComponent::PsfTextureDepthLayout_value_to_string(
			tiledSplatBlur->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout) << "_";
		outFolder << results.m_timestampFile;
		return outFolder.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsOutFolderGroundTruth(Scene::Scene& scene, Scene::Object* object, Results& resultsTarget)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream outFolder;
		outFolder << getMetricsOutFolderBase(scene, object);
		outFolder << ConvolutionSettings::Algorithm_value_to_string(resultsTarget.m_algorithm) << "_";
		outFolder << results.m_timestampFile;
		return outFolder.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsFileNamePrefixComplexPhasor(Scene::Scene& scene, Scene::Object* object, Scene::Object* complexBlur)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream filenamePrefix;
		filenamePrefix << results.m_scene;
		filenamePrefix << "_" << results.m_aberration;
		filenamePrefix << "_" << "ComplexPhasor";
		return filenamePrefix.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsFileNamePrefixTiledSplat(Scene::Scene& scene, Scene::Object* object, Scene::Object* tiledSplatBlur)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream filenamePrefix;
		filenamePrefix << results.m_scene;
		filenamePrefix << "_" << results.m_aberration;
		filenamePrefix << "_" << "TiledSplat";
		return filenamePrefix.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getMetricsFileNamePrefixGroundTruth(Scene::Scene& scene, Scene::Object* object, Results& resultsTarget)
	{
		auto& results = getDisplayResults(scene, object);

		std::stringstream filenamePrefix;
		filenamePrefix << results.m_scene;
		filenamePrefix << "_" << results.m_aberration;
		filenamePrefix << "_" << ConvolutionSettings::Algorithm_value_to_string(resultsTarget.m_algorithm);
		return filenamePrefix.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiMetricsPopup(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		auto& results = getDisplayResults(scene, object);
		float maxWidth = 0;

		// Complex lurs
		auto const& complexBlurs = Scene::filterObjects(scene, Scene::OBJECT_TYPE_COMPLEX_BLUR, true, true);
		const size_t numComplexBlurs = complexBlurs.size();
		for (size_t i = 0; i < complexBlurs.size(); ++i)
		{
			auto complexBlur = complexBlurs[i];
			maxWidth = glm::max(maxWidth, ImGui::CalcTextSize(complexBlur->m_name.c_str()).x);
			const int buttonId = i;

			// Filter out invalid results
			if (results.m_aberration != complexBlur->component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_name) continue;
			if (results.m_channels != 1) continue;
			if (results.m_scene != getCurrentScene(scene)) continue;
			if (results.m_camera != getCurrentCamera(scene)) continue;

			if (EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") == -1)
			{
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName") = getMetricsOutFolderComplexPhasor(scene, object, complexBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix") = getMetricsFileNamePrefixComplexPhasor(scene, object, complexBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType") = "Aberration_ComplexBlur";

				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") = buttonId;
			}

			if (ImGui::RadioButton(complexBlur->m_name.c_str(), &EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId"), buttonId))
			{
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName") = getMetricsOutFolderComplexPhasor(scene, object, complexBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix") = getMetricsFileNamePrefixComplexPhasor(scene, object, complexBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType") = "Aberration_ComplexBlur";
			}
		}

		// Tiled splat blurs
		auto const& tiledSplatBlurs = Scene::filterObjects(scene, Scene::OBJECT_TYPE_TILED_SPLAT_BLUR, true, true);
		const size_t numTiledSplatBlurs = tiledSplatBlurs.size();
		for (size_t i = 0; i < tiledSplatBlurs.size(); ++i)
		{
			auto tiledSplatBlur = tiledSplatBlurs[i];
			maxWidth = glm::max(maxWidth, ImGui::CalcTextSize(tiledSplatBlur->m_name.c_str()).x);
			const int buttonId = numComplexBlurs + i;

			// Filter out invalid results
			if (results.m_aberration != tiledSplatBlur->component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_name) continue;
			if (results.m_scene != getCurrentScene(scene)) continue;
			if (results.m_camera != getCurrentCamera(scene)) continue;

			if (EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") == -1)
			{
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName") = getMetricsOutFolderTiledSplat(scene, object, tiledSplatBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix") = getMetricsFileNamePrefixTiledSplat(scene, object, tiledSplatBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType") = "Aberration_TiledSplat";

				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") = buttonId;
			}

			if (ImGui::RadioButton(tiledSplatBlur->m_name.c_str(), &EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId"), buttonId))
			{
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName") = getMetricsOutFolderTiledSplat(scene, object, tiledSplatBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix") = getMetricsFileNamePrefixTiledSplat(scene, object, tiledSplatBlur);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType") = "Aberration_TiledSplat";
			}
		}

		// Previous results
		for (size_t i = 0; i < object->component<GroundTruthAberrationComponent>().m_results.size(); ++i)
		{
			auto& resultsTarget = object->component<GroundTruthAberrationComponent>().m_results[i];
			std::string label = resultLabel(resultsTarget);
			maxWidth = glm::max(maxWidth, ImGui::CalcTextSize(label.c_str()).x);
			const int buttonId = numTiledSplatBlurs + numComplexBlurs + i;
			
			// Filter out ourselves and invalid other results
			if (i == object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultRenderId) continue;
			if (results.m_aberration != resultsTarget.m_aberration) continue;
			if (results.m_scene != resultsTarget.m_scene) continue;
			if (results.m_camera != resultsTarget.m_camera) continue;
			if (results.m_simulateOffAxis != resultsTarget.m_simulateOffAxis) continue;
			if (results.m_channels != resultsTarget.m_channels) continue;

			if (EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") == -1)
			{
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName") = getMetricsOutFolderGroundTruth(scene, object, resultsTarget);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix") = getMetricsFileNamePrefixGroundTruth(scene, object, resultsTarget);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType") = "Aberration_GroundTruth";
				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ResultId") = i;

				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") = buttonId;
			}

			if (ImGui::RadioButton(label.c_str(), &EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId"), buttonId))
			{
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName") = getMetricsOutFolderGroundTruth(scene, object, resultsTarget);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix") = getMetricsFileNamePrefixGroundTruth(scene, object, resultsTarget);
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType") = "Aberration_GroundTruth";
				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ResultId") = i;
			}
		}

		ImGui::Separator();

		// Widget editors
		ImGui::PushItemWidth(maxWidth);
		ImGui::InputText("Folder", EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName"));
		ImGui::InputText("Prefix", EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix"));
		ImGui::Checkbox("PSNR", &EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_Metrics_PSNR"));
		ImGui::SameLine();
		ImGui::Checkbox("SSIM", &EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_Metrics_SSIM"));
		ImGui::SameLine();
		ImGui::Checkbox("HDR-VDP", &EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_Metrics_HdrVdp"));
		ImGui::PopItemWidth();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		// Initia metric computations
		if (ImGui::ButtonEx("Ok", "|########|"))
		{
			initiateComputeMetrics(scene, object,
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_SourceType"),
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FolderName"),
				EditorSettings::editorProperty<std::string>(scene, object, "GroundTruthAberration_Metrics_FileNamePrefix"),
				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ResultId"),
				&EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_Metrics_PSNR"),
				&EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_Metrics_SSIM"),
				&EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_Metrics_HdrVdp"),
				true);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::ButtonEx("Cancel", "|########|"))
		{
			ImGui::CloseCurrentPopup();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiResultImages(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, Results& result, const int resultId, const int imageSize)
	{
		std::string label = resultLabel(result);
		if (ImGui::RadioButton(label.c_str(), &object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultRenderId, resultId))
		{
			uploadResultRender(scene, object, result);
			object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode = Results::Convolution;
		}
		if (ImGui::IsItemHovered())
		{
			if (resultId != object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultTooltipId)
			{
				uploadResultTooltip(scene, object, result);
				object->component<GroundTruthAberrationComponent>().m_outputSettings.m_resultTooltipId = resultId;
			}
			const int imageSize = 512;
			const float aspect = float(result.m_width) / float(result.m_height);

			ImGui::BeginTooltip();
			// Original image
			ImGui::TextDisabled("Original Image");
			ImGui::Image(&(scene.m_textures[object->m_name + "_tooltip_color"].m_texture), ImVec2(imageSize * aspect, imageSize),
				ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			// Resulting image
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			ImGui::TextDisabled("Resulting Image");
			ImGui::Image(&(scene.m_textures[object->m_name + "_tooltip_result"].m_texture), ImVec2(imageSize * aspect, imageSize),
				ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			ImGui::EndTooltip();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiResultProperties(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, Results& result)
	{
		#define LABEL(L) { ImGui::TableNextColumn(); ImGui::TextDisabled(L); ImGui::TableNextColumn(); }
		#define ROW_STRING(NAME, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text((VALUE).c_str());
		#define ROW_STRING_VIEW(NAME, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); { std::string str = std::to_string(VALUE); ImGui::Text(str.c_str()); }
		#define ROW_BOOL(NAME, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text("%s", (VALUE) ? "true" : "false");
		#define ROW_NUM(NAME, FORMAT, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text(FORMAT, (VALUE));
		#define ROW_NUM2(NAME, FORMAT, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text("(" FORMAT ", " FORMAT ")", (VALUE)[0], (VALUE)[1]);
		#define ROW_NUM3(NAME, FORMAT, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text("(" FORMAT ", " FORMAT ", " FORMAT ")", (VALUE)[0], (VALUE)[1], (VALUE)[2]);
		#define ROW_NUM4(NAME, FORMAT, VALUE) ImGui::TableNextColumn(); ImGui::Text(NAME); ImGui::TableNextColumn(); ImGui::Text("(" FORMAT ", " FORMAT ", " FORMAT ", " FORMAT ")", (VALUE)[0], (VALUE)[1], (VALUE)[2], (VALUE)[3]);

		if (ImGui::BeginTable("GroundTruthResult", 2))
		{
			// Common Properties
			LABEL("Common Properties");
			ROW_STRING("Date", result.m_timestampDisplay);
			ROW_STRING("Scene", result.m_scene);
			ROW_STRING("Camera", result.m_camera);
			ROW_STRING("Aberration", result.m_aberration);
			ROW_BOOL("HDR", result.m_isHdr);
			ROW_NUM("Width", "%d", result.m_width);
			ROW_NUM("Height", "%d", result.m_height);
			ROW_NUM("Channels", "%d", result.m_channels);
			ROW_BOOL("Off-axis", result.m_simulateOffAxis);
			ROW_NUM("Focus Distance", "%f", result.m_focusDistance);
			ROW_NUM("Aperture Diameter", "%f", result.m_apertureDiameter);
			ROW_STRING_VIEW("Blend Mode", ConvolutionSettings::BlendMode_value_to_string(result.m_blendMode));
			ROW_STRING_VIEW("Algorithm", ConvolutionSettings::Algorithm_value_to_string(result.m_algorithm));

			// Diopter Binning
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			LABEL("Diopter Binning");
			ROW_NUM("Dioptres Precision", "%d", result.m_dioptresPrecision);
			ROW_NUM("Incident Angles Precision", "%f", result.m_incidentAnglesPrecision);
			ROW_BOOL("Centered Dioptres", result.m_centerIncidentAngles);
			ROW_BOOL("Centered Incident Angles", result.m_centerDioptres);
			ROW_NUM("Number of Bins", "%d", result.m_numBins);

			// Processing Times
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			LABEL("Processing Times");
			ROW_NUM("PSF Bins", "%f", result.m_psfBinTime);
			ROW_NUM("Convolution", "%f", result.m_convolutionTime);
			ROW_NUM("Total", "%f", result.m_totalProcessingTime);

			// Minimum and Maximum Values
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			LABEL("Minimum and Maximum Values");
			ROW_NUM("Min Depth", "%f", result.m_minDepth);
			ROW_NUM("Max Depth", "%f", result.m_maxDepth);
			ROW_NUM3("Min Defocus", "%f", result.m_minDefocus);
			ROW_NUM3("Max Defocus", "%f", result.m_maxDefocus);
			ROW_NUM3("Min Blur Radius", "%f", result.m_minBlurRadius);
			ROW_NUM3("Max Blur Radius", "%f", result.m_maxBlurRadius);
			ROW_NUM3("Min Num Samples", "%d", result.m_minNumSamples);
			ROW_NUM3("Max Num Samples", "%d", result.m_maxNumSamples);

			// Similarity Metrics
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			LABEL("Similarity Metrics");
			ROW_NUM("HDR-VDP Probablity of Detection", "%f", result.m_hdrvdp);
			ROW_NUM("Average MSSIM", "%f", result.m_mmssim);
			ROW_NUM("Average PSNR", "%f", result.m_mpsnr);
			ROW_NUM("Average SNR", "%f", result.m_msnr);
			ROW_NUM("Average RMSE", "%f", result.m_mrmse);
			ROW_NUM("Average MSE", "%f", result.m_mmse);
			ROW_NUM3("MSSIM", "%f", result.m_mssim);
			ROW_NUM3("PSNR", "%f", result.m_psnr);
			ROW_NUM3("SNR", "%f", result.m_snr);
			ROW_NUM3("RMSE", "%f", result.m_rmse);
			ROW_NUM3("MSE", "%f", result.m_mse);

			ImGui::EndTable();
		}

	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGuiResultsTab(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		// Results entries
		for (size_t i = 0; i < object->component<GroundTruthAberrationComponent>().m_results.size(); ++i)
		{
			generateGuiResultImages(scene, guiSettings, object, object->component<GroundTruthAberrationComponent>().m_results[i], i, 512);
		}

		// Properties below the list
		if (object->component<GroundTruthAberrationComponent>().m_results.size() > 0)
		{
			generateGuiResultProperties(scene, guiSettings, object, getDisplayResults(scene, object));
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		// Make the tab bar
		if (ImGui::BeginTabBar(object->m_name.c_str()) == false)
			return;

		// Restore the selected tab id
		std::string activeTab;
		if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, object, "MainTabBar_SelectedTab#Synced"); activeTabSynced.has_value())
			activeTab = activeTabSynced.value();

		// Aberration settings
		if (ImGui::BeginTabItem("Aberration", activeTab.c_str()))
		{
			Aberration::generateGui(scene, guiSettings, object, getAberration(scene, object), getAberrationPresets(scene, object));

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// Convolution tab
		if (ImGui::BeginTabItem("Convolution", activeTab.c_str()))
		{
			ImGui::Combo("Output Mode", &object->component<GroundTruthAberrationComponent>().m_outputSettings.m_outputMode, Results::ResultAttribute_meta);
			ImGui::Separator();

			ImGui::Combo("Input Dynamic Range", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_inputDynamicRange, ConvolutionSettings::InputDynamicRange_meta);
			ImGui::Combo("Algorithm", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_algorithm, ConvolutionSettings::Algorithm_meta);
			ImGui::Combo("Blend Mode", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_blendMode, ConvolutionSettings::BlendMode_meta);
			
			ImGui::Separator();

			ImGui::SliderInt("Number of Channels", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_numChannels, 1, 3);
			ImGui::SliderFloat("Diopter Bin Precision", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_dioptresPrecision, 0.0f, 0.1f);
			ImGui::SameLine();
			ImGui::Checkbox("Center Dioptres", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_centerDioptres);
			ImGui::SliderFloat("Incident Angles Bin Precision", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_incidentAnglesPrecision, 0.0f, 5.0f);
			ImGui::SameLine();
			ImGui::Checkbox("Center Angles", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_centerIncidentAngles);

			ImGui::Checkbox("Simulate Off-Axis", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_simulateOffAxis);
			ImGui::SameLine();
			ImGui::Checkbox("Export PSFs", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_exportPsfs);

			ImGui::Separator();

			ImGui::Combo("Log Detail", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail, ConvolutionSettings::PrintDetail_meta);
			ImGui::Checkbox("Omit File Logging", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_omitFileLogging);
			ImGui::SameLine();
			ImGui::Checkbox("Sleep On Finish", &object->component<GroundTruthAberrationComponent>().m_convolutionSettings.m_sleepWhenDone);

			if (ImGui::Button("Convolve"))
			{
				for (auto const& preset : getAberrationPresets(scene, object))
				{
					//EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_BatchConvolve_" + preset.first) = true;
				}
				ImGui::OpenPopup("GroundTruthAberration_BatchConvolve");
			}

			// Batch convolve popup
			if (ImGui::BeginPopup("GroundTruthAberration_BatchConvolve"))
			{
				for (auto const& preset : getAberrationPresets(scene, object))
				{
					ImGui::Checkbox(preset.first.c_str(), &EditorSettings::editorProperty<bool>(scene, object, "GroundTruthAberration_BatchConvolve_" + preset.first));
				}

				// Initia metric computations
				if (ImGui::ButtonEx("Ok", "|########|"))
				{
					initiateSimulateAberration(scene, object, true);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::ButtonEx("Cancel", "|########|"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// Metrics tab
		if (ImGui::BeginTabItem("Metrics", activeTab.c_str()))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::Text("HDR-VDP");
			ImGui::PopStyleColor();

			ImGui::SliderFloat("Peak-Luminance", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpPeakLuminance, 0.0f, 1000.0f);
			ImGui::SliderFloat("Contrast Ratio", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpContrastRatio, 0.0f, 1000.0f);
			ImGui::SliderFloat("Gamma", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpGamma, 0.0f, 4.0f);
			ImGui::SliderFloat("Ambient Light", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpAmbientLight, 0.0f, 1000.0f);
			ImGui::SliderFloat("Display Size", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpDisplaySize, 0.0f, 100.0f);
			ImGui::DragInt2("Display Resolution", glm::value_ptr(object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpDisplayResolution));
			ImGui::SliderFloat("View Distance", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpViewDistance, 0.0f, 10.0f);
			ImGui::SliderFloat("Surround", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpSurround, 0.0f, 1000.0f);
			ImGui::SliderFloat("Sensitivity Correction", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpSensitivityCorrection, -2.0f, 2.0f);

			ImGui::Separator();

			ImGui::Checkbox("Export Metrics", &object->component<GroundTruthAberrationComponent>().m_metricSettings.m_exportMetrics);

			if (ImGui::Button("Compute Metrics"))
			{
				Scene::Object* tiledSplatBlur = Scene::filterObjects(scene, Scene::OBJECT_TYPE_TILED_SPLAT_BLUR, true, true)[0];
				EditorSettings::editorProperty<int>(scene, object, "GroundTruthAberration_Metrics_ButtonId") = -1;
				ImGui::OpenPopup("GroundTruthAberration_Metrics");
			}

			// Metrics popup
			if (ImGui::BeginPopup("GroundTruthAberration_Metrics"))
			{
				generateGuiMetricsPopup(scene, guiSettings, object);
				ImGui::EndPopup();
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (object->component<GroundTruthAberrationComponent>().m_results.empty() == false && ImGui::BeginTabItem("Result", activeTab.c_str()))
		{
			generateGuiResultsTab(scene, guiSettings, object);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// End the tab bar
		ImGui::EndTabBar();
	}

	////////////////////////////////////////////////////////////////////////////////
	void demoSetup(Scene::Scene& scene)
	{
		// @CONSOLE_VAR(Scene, Object Groups, -object_group, Aberration_GroundTruth)
		SimulationSettings::createGroup(scene, "Aberration", "Aberration_GroundTruth");

		// Add the debug visualization object.
		auto& groundTruthAberration = createObject(scene, Scene::OBJECT_TYPE_GROUND_TRUTH_ABERRATION, Scene::extendDefaultObjectInitializerBefore([](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, "Aberration_GroundTruth");

			// Aberration parameters
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_aberration.m_name = Config::AttribValue("aberration").get<std::string>();

			// Convolution settings
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_inputDynamicRange = GroundTruthAberration::ConvolutionSettings::LDR;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_algorithm = GroundTruthAberration::ConvolutionSettings::PerPixel;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_blendMode = GroundTruthAberration::ConvolutionSettings::FrontToBack;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_printDetail = GroundTruthAberration::ConvolutionSettings::Progress;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_numChannels = 3;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_simulateOffAxis = true;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_dioptresPrecision = 0.005f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_incidentAnglesPrecision = 0.20f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_centerIncidentAngles = false;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_centerDioptres = false;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_omitFileLogging = true;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_convolutionSettings.m_sleepWhenDone = false;

			// Metric settings
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpPeakLuminance = 400.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpContrastRatio = 1000.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpGamma = 2.2f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpAmbientLight = 100.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpDisplaySize = 28.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpDisplayResolution = glm::ivec2(3840, 2160);
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpViewDistance = 1.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpSurround = 200.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_hdrvdpSensitivityCorrection = 0.0f;
			object.component<GroundTruthAberration::GroundTruthAberrationComponent>().m_metricSettings.m_exportMetrics = true;
		}));
	}
}