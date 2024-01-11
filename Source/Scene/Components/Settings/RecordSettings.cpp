#include "PCH.h"
#include "RecordSettings.h"
#include "DelayedJobs.h"
#include "SimulationSettings.h"
#include "RenderSettings.h"
#include "../Rendering/Camera.h"

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "shlwapi")

namespace RecordSettings
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(RECORD_SETTINGS);
	DEFINE_OBJECT(RECORD_SETTINGS);
	REGISTER_OBJECT_UPDATE_CALLBACK(RECORD_SETTINGS, AFTER, KEYFRAMED_ANIM);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Init the windows media foundation library
		MFStartup(MF_VERSION);

		// Init the AVI library
		AVIFileInit();

		Debug::log_debug() << std::string(80, '=') << Debug::end;
		Debug::log_debug() << "List of installed video compressors: " << Debug::end;
		char buff[5] = "    ";
		for (int i = 0; i < 1024; i++)
		{
			ICINFO info = { sizeof(ICINFO) };
			if (!ICInfo(ICTYPE_VIDEO, i, &info))
				break;

			buff[0] = (info.fccHandler >> 0)  & 0xFF;
			buff[1] = (info.fccHandler >> 8)  & 0xFF;
			buff[2] = (info.fccHandler >> 16) & 0xFF;
			buff[3] = (info.fccHandler >> 24) & 0xFF;
			Debug::log_debug() << "  - " << buff << Debug::end;
		}
		Debug::log_debug() << std::string(80, '=') << Debug::end;


		DelayedJobs::postJob(scene, &object, "Restore Codec Options", true, 1, [](Scene::Scene& scene, Scene::Object& object)
		{
			restoreAviCodecConfig(scene, &object);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
		AVIFileExit();
		MFShutdown();
	}

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object)
	{
		// Video
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_F5] == 1)
		{
			if (object->component<RecordSettings::RecordSettingsComponent>().m_isRecording)
				stopRecording(scene, object);
			else
				startRecording(scene, object);
		}

		// Screenshot
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_F12] == 1)
		{
			DelayedJobs::postJob(scene, Scene::OBJECT_TYPE_RECORD_SETTINGS, "Screenshot", [](Scene::Scene& scene, Scene::Object& object)
			{
				RecordSettings::saveScreenshot(scene);
			});
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

		if (object->component<RecordSettings::RecordSettingsComponent>().m_isRecording)
		{
			addFrame(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::Combo("Record Mode", &object->component<RecordSettings::RecordSettingsComponent>().m_recordType, RecordSettingsComponent::RecordType_meta);
		ImGui::SliderInt("Video Framerate", &object->component<RecordSettings::RecordSettingsComponent>().m_videoFrameRate, 1, 60);
		if (ImGui::Button("Compress Options"))
		{
			configureAviCodec(scene, object);
		}
		ImGui::InputText("Output File Name", object->component<RecordSettings::RecordSettingsComponent>().m_outputFileName, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::Checkbox("Include GUI", &object->component<RecordSettings::RecordSettingsComponent>().m_includeGui);
		ImGui::SameLine();
		ImGui::Checkbox("Use Worker Thread", &object->component<RecordSettings::RecordSettingsComponent>().m_useWorkerThread);
		if (ImGui::Button("Export G-Buffer"))
		{
			RecordSettings::saveGbuffer(scene);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void extractSubImage(T* pixels, T* out, int width, int height, int subWidth, int subHeight, int channels)
	{
		// Early out if the target and dimensions match
		if (width == subWidth && height == subHeight && pixels == out) return;

		for (int y = 0; y < subHeight; ++y)
		for (int x = 0; x < subWidth; ++x)
		for (int c = 0; c < channels; ++c)
			out[channels * (x + y * subWidth) + c] = pixels[channels * (x + y * width) + c];
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void extractSubImage(T* pixels, int width, int height, int subWidth, int subHeight, int channels)
	{
		extractSubImage(pixels, pixels, width, height, subWidth, subHeight, channels);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isRecordingAsync(Scene::Scene& scene, Scene::Object* object)
	{
		return object->component<RecordSettings::RecordSettingsComponent>().m_isRecordingAsync;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool checkAviResult(HRESULT code)
	{
		if (code == S_OK || code == AVIERR_OK) return true;

		Debug::DebugRegion("AVI");

		const char* msg = nullptr;
		switch (code)
		{
		case AVIERR_BADFORMAT:      msg = "AVIERR_BADFORMAT: corrupt file or unrecognized format"; break;
		case AVIERR_MEMORY:         msg = "AVIERR_MEMORY: insufficient memory"; break;
		case AVIERR_FILEREAD:       msg = "AVIERR_FILEREAD: disk error while reading file"; break;
		case AVIERR_FILEOPEN:       msg = "AVIERR_FILEOPEN: disk error while opening file"; break;
		case REGDB_E_CLASSNOTREG:   msg = "REGDB_E_CLASSNOTREG: file type not recognised"; break;
		case AVIERR_READONLY:       msg = "AVIERR_READONLY: file is read-only"; break;
		case AVIERR_NOCOMPRESSOR:   msg = "AVIERR_NOCOMPRESSOR: a suitable compressor could not be found"; break;
		case AVIERR_UNSUPPORTED:    msg = "AVIERR_UNSUPPORTED: compression is not supported for this type of data"; break;
		case AVIERR_INTERNAL:       msg = "AVIERR_INTERNAL: internal error"; break;
		case AVIERR_BADFLAGS:       msg = "AVIERR_BADFLAGS"; break;
		case AVIERR_BADPARAM:       msg = "AVIERR_BADPARAM"; break;
		case AVIERR_BADSIZE:        msg = "AVIERR_BADSIZE"; break;
		case AVIERR_BADHANDLE:      msg = "AVIERR_BADHANDLE"; break;
		case AVIERR_FILEWRITE:      msg = "AVIERR_FILEWRITE: disk error while writing file"; break;
		case AVIERR_COMPRESSOR:     msg = "AVIERR_COMPRESSOR"; break;
		case AVIERR_NODATA:         msg = "AVIERR_READONLY"; break;
		case AVIERR_BUFFERTOOSMALL: msg = "AVIERR_BUFFERTOOSMALL"; break;
		case AVIERR_CANTCOMPRESS:   msg = "AVIERR_CANTCOMPRESS"; break;
		case AVIERR_USERABORT:      msg = "AVIERR_USERABORT"; break;
		case AVIERR_ERROR:          msg = "AVIERR_ERROR"; break;
		}
		if (msg) Debug::log_error() << msg << Debug::end;
		else     Debug::log_error() << "Unknown error code: " << code << Debug::end;
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getCompressOptionsFilePath(Scene::Scene& scene, Scene::Object* object)
	{
		return (EnginePaths::configFilesFolder() / "avi_compress_options.dat").string();
	}

	////////////////////////////////////////////////////////////////////////////////
	void saveAviCodecConfig(Scene::Scene& scene, Scene::Object* object)
	{
		Debug::log_debug() << "Saving codec options to file: " << getCompressOptionsFilePath(scene, object) << Debug::end;

		// Current options
		auto const& options = object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_compressOptions;

		// Try to open the config file path
		std::ofstream outputStream(getCompressOptionsFilePath(scene, object), std::ios::out | std::ios::binary);
		if (!outputStream.good()) return;

		// Extract the options properties
		outputStream << options.fccType;
		outputStream << options.fccHandler;
		outputStream << options.dwKeyFrameEvery;
		outputStream << options.dwQuality;
		outputStream << options.dwBytesPerSecond;
		outputStream << options.dwFlags;
		outputStream << options.cbFormat;
		outputStream << options.cbParms;
		outputStream << options.dwInterleaveEvery;
		if (options.lpParms != nullptr && options.cbParms > 0)
		{
			outputStream.write((char*)options.lpParms, options.cbParms);
		}
		if (options.lpFormat != nullptr && options.cbFormat > 0)
		{
			outputStream.write((char*)options.lpFormat, options.cbFormat);
		}

		Debug::log_debug() << "Codec options successfully saved. " << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void restoreAviCodecConfig(Scene::Scene& scene, Scene::Object* object)
	{
		// Resulting options
		auto& options = object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_compressOptions;

		Debug::log_debug() << "Attempting to restore previous codec options from file: " << getCompressOptionsFilePath(scene, object)  << Debug::end;

		// Try to open the config file path
		std::ifstream inputStream(getCompressOptionsFilePath(scene, object), std::ios::in | std::ios::binary);
		if (!inputStream.good())
		{
			Debug::log_debug() << "No previous settings found. " << Debug::end;
			return;
		}

		// Extract the options properties
		inputStream >> options.fccType;
		inputStream >> options.fccHandler;
		inputStream >> options.dwKeyFrameEvery;
		inputStream >> options.dwQuality;
		inputStream >> options.dwBytesPerSecond;
		inputStream >> options.dwFlags;
		inputStream >> options.cbFormat;
		inputStream >> options.cbParms;
		inputStream >> options.dwInterleaveEvery;
		if (options.cbParms > 0)
		{
			options.lpParms = new char[options.cbParms];
			inputStream.read((char*) options.lpParms, options.cbParms);
		}
		if (options.cbFormat > 0)
		{
			options.lpFormat = new char[options.cbFormat];
			inputStream.read((char*) options.lpFormat, options.cbFormat);
		}
		
		Debug::log_debug() << "Codec options successfully restored. " << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void configureAviCodec(Scene::Scene& scene, Scene::Object* object)
	{
		// Init the options from the current settings
		AVICOMPRESSOPTIONS* opts[1];
		opts[0] = &object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_compressOptions;

		// Open a dummy stream
		object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName = (EnginePaths::assetsFolder() / "~tmp.avi").string();
		openAviStream(scene, object);

		// Open the config window
		BOOL result = AVISaveOptions(GetActiveWindow(), 0, 1, &object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_streamCompressed, opts);
		checkAviResult(AVISaveOptionsFree(1, opts));

		// Close the dummy stream and delete the file
		closeAviStream(scene, object);
		DeleteFileA(object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName.c_str());

		// Store the new configuration if it changed
		if (result == TRUE) saveAviCodecConfig(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void openAviStream(Scene::Scene& scene, Scene::Object* object)
	{
		// Compute the output resolution
		Scene::Object* renderSettings = findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		glm::ivec2 resolution;
		if (object->component<RecordSettings::RecordSettingsComponent>().m_includeGui)
			resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_windowSize;
		else
			resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;

		// Set the first frame id to 0
		object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_lastFrameId = 0;

		// Convert the name to wide string
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring fileNameW = converter.from_bytes(object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName);

		// Open the AVI file
		if (checkAviResult(AVIFileOpen(&object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_file, fileNameW.c_str(), OF_WRITE | OF_CREATE, 0)) == false)
		{
			Debug::log_error() << "Error occured while opening AVI file" << Debug::end;
			object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName.clear();
		}

		// Create the AVI stream
		AVISTREAMINFO streamHeader;
		memset(&streamHeader, 0, sizeof(streamHeader));

		streamHeader.fccType = streamtypeVIDEO;
		streamHeader.fccHandler = 0;
		streamHeader.dwScale = 1;
		streamHeader.dwRate = object->component<RecordSettings::RecordSettingsComponent>().m_videoFrameRate;
		streamHeader.dwQuality = -1;
		streamHeader.dwSuggestedBufferSize = 0;
		streamHeader.dwSampleSize = 0;
		SetRect(&streamHeader.rcFrame, 0, 0, resolution.x, resolution.y);

		if (checkAviResult(AVIFileCreateStream(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_file, &object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_stream, &streamHeader)) == false)
		{
			Debug::log_error() << "Error occured while opening AVI stream" << Debug::end;
			object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName.clear();
			checkAviResult(AVIFileClose(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_file));
		}

		// Create the compressed avi stream
		AVICOMPRESSOPTIONS compressOptions = object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_compressOptions;
		compressOptions.fccType = streamtypeVIDEO;

		if (checkAviResult(AVIMakeCompressedStream(&object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_streamCompressed, object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_stream, &compressOptions, 0)) == false)
		{
			Debug::log_error() << "Error occured while opening compressed AVI stream" << Debug::end;
			object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName.clear();
			checkAviResult(AVIStreamRelease(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_stream));
			checkAviResult(AVIFileClose(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_file));
		}

		// Set the avi file format
		BITMAPINFOHEADER bi;
		memset(&bi, 0, sizeof(bi));
		bi.biSize = sizeof(bi);
		bi.biWidth = resolution.x;
		bi.biHeight = resolution.y;
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		bi.biCompression = 0;
		bi.biSizeImage = 0;
		if (checkAviResult(AVIStreamSetFormat(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_streamCompressed, 0, &bi, sizeof(bi))) == false)
		{
			Debug::log_error() << "Error occured while setting AVI stream format" << Debug::end;
			object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName.clear();
			checkAviResult(AVIStreamRelease(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_streamCompressed));
			checkAviResult(AVIStreamRelease(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_stream));
			checkAviResult(AVIFileClose(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_file));
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void closeAviStream(Scene::Scene& scene, Scene::Object* object)
	{
		// Close the avi stream
		checkAviResult(AVIStreamRelease(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_streamCompressed));

		// Close the avi stream
		checkAviResult(AVIStreamRelease(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_stream));

		// Close the file
		checkAviResult(AVIFileClose(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_file));
	}

	////////////////////////////////////////////////////////////////////////////////
	void writeFrameData(Scene::Scene& scene, Scene::Object* object, Frame frame)
	{
		checkAviResult(AVIStreamWrite(object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_streamCompressed,
			frame.m_frameId, 1, frame.m_pixels, frame.m_dataSize, AVIIF_KEYFRAME, 0, 0));
		delete[] frame.m_pixels;
	}

	////////////////////////////////////////////////////////////////////////////////
	void addFrameDataAsync(Scene::Scene& scene, Scene::Object* object, Frame frame)
	{
		std::unique_lock frameBufferLock(object->component<RecordSettings::RecordSettingsComponent>().m_frameBufferMutex);
		object->component<RecordSettings::RecordSettingsComponent>().m_frameBuffer.push(frame);
	}

	////////////////////////////////////////////////////////////////////////////////
	void addFrameDataSync(Scene::Scene& scene, Scene::Object* object, Frame frame)
	{
		writeFrameData(scene, object, frame);
	}

	////////////////////////////////////////////////////////////////////////////////
	void addFrame(Scene::Scene& scene, Scene::Object* object)
	{
		// Resolution
		Scene::Object* renderSettings = findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		glm::ivec2 resolution;
		if (object->component<RecordSettings::RecordSettingsComponent>().m_includeGui)
			resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_windowSize;
		else
			resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;

		// Construct the frame
		Frame frame;
		frame.m_frameId = object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_lastFrameId;
		frame.m_numPixels = resolution.x * resolution.y * 4;
		frame.m_dataSize = frame.m_numPixels * sizeof(unsigned char);
		frame.m_pixels = new unsigned char[frame.m_numPixels];

		// Extract the color and depth buffers
		if (object->component<RecordSettings::RecordSettingsComponent>().m_includeGui)
		{
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glReadPixels(0, 0, resolution.x, resolution.y, GL_BGRA, GL_UNSIGNED_BYTE, frame.m_pixels);
		}
		else
		{
			int gbufferId = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferRead;
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glGetTextureSubImage(scene.m_gbuffer[gbufferId].m_colorTextures[scene.m_gbuffer[gbufferId].m_readBuffer], 0, 0, 0, 0,
				resolution.x, resolution.y, 1, GL_BGRA, GL_UNSIGNED_BYTE, frame.m_dataSize, frame.m_pixels);
		}

		// Append the pixel data
		if (isRecordingAsync(scene, object))
			addFrameDataAsync(scene, object, frame);
		else
			addFrameDataSync(scene, object, frame);

		// Increment the frame counter
		++object->component<RecordSettings::RecordSettingsComponent>().m_avi.m_lastFrameId;
	}

	////////////////////////////////////////////////////////////////////////////////
	void writeThreadCallback(Scene::Scene& scene, Scene::Object* object)
	{
		Debug::log_debug() << "Worker thread started" << Debug::end;

		while (true)
		{
			// We can quit if recording has stopped and we ran out of frames
			if (!object->component<RecordSettings::RecordSettingsComponent>().m_isRecording && object->component<RecordSettings::RecordSettingsComponent>().m_frameBuffer.empty())
				break;

			// If no data is present -> wait, then start over
			if (object->component<RecordSettings::RecordSettingsComponent>().m_frameBuffer.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// Extract the first available frame and save it to disk			
			{
				std::unique_lock frameBufferLock(object->component<RecordSettings::RecordSettingsComponent>().m_frameBufferMutex);
				Frame frame = object->component<RecordSettings::RecordSettingsComponent>().m_frameBuffer.front();
				object->component<RecordSettings::RecordSettingsComponent>().m_frameBuffer.pop();
				writeFrameData(scene, object, frame);
			}
		}

		Debug::log_debug() << "Worker thread finished" << Debug::end;

		// Close the AVI stream
		closeAviStream(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void startRecording(Scene::Scene& scene, Scene::Object* object)
	{
		// Stop any ongoing records first
		if (object->component<RecordSettings::RecordSettingsComponent>().m_isRecording)
			stopRecording(scene, object);

		// Set the recording flag
		object->component<RecordSettings::RecordSettingsComponent>().m_isRecording = true;
		object->component<RecordSettings::RecordSettingsComponent>().m_isRecordingAsync = object->component<RecordSettings::RecordSettingsComponent>().m_useWorkerThread;

		// Generate output file name
		//std::string filePath = Debug::formatText(object->component<RecordSettings::RecordSettingsComponent>().m_outputFileName) + s_fileExtensions[object->component<RecordSettings::RecordSettingsComponent>().m_videoCompressor];
		std::string filePath = Debug::formatText(object->component<RecordSettings::RecordSettingsComponent>().m_outputFileName) + ".avi";
		object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName = (EnginePaths::assetsFolder() / filePath).string();

		Debug::log_info() << "Video record start, output file name: " << object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName << Debug::end;

		// Make sure that the directories exist
		Asset::makeDirectoryStructure(object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName);

		// Open the AVI stream
		openAviStream(scene, object);

		// Start the worker thread
		if (object->component<RecordSettings::RecordSettingsComponent>().m_useWorkerThread)
			std::thread(writeThreadCallback, std::ref(scene), object).detach();
	}

	////////////////////////////////////////////////////////////////////////////////
	void startRecording(Scene::Scene& scene)
	{
		startRecording(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RECORD_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void stopRecording(Scene::Scene& scene, Scene::Object* object)
	{
		if (!object->component<RecordSettings::RecordSettingsComponent>().m_isRecording) return;

		object->component<RecordSettings::RecordSettingsComponent>().m_isRecording = false;

		Debug::log_info() << "Video record end, output file name: " << object->component<RecordSettings::RecordSettingsComponent>().m_currentVideoName << Debug::end;

		// Close the AVI stream if we are not using a worker thread
		if (!isRecordingAsync(scene, object))
			closeAviStream(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void stopRecording(Scene::Scene& scene)
	{
		stopRecording(scene, Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RECORD_SETTINGS));
	}

	////////////////////////////////////////////////////////////////////////////////
	void saveScreenshot(Scene::Scene& scene)
	{
		auto renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		auto recordSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RECORD_SETTINGS);
		std::string fileName = Debug::formatText(recordSettings->component<RecordSettings::RecordSettingsComponent>().m_outputFileName);

		if (recordSettings->component<RecordSettings::RecordSettingsComponent>().m_includeGui == false)
		{
			glm::ivec2 resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
			Asset::saveGbufferColor(scene, fileName, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite, 0, resolution.x, resolution.y);
		}
		else
		{
			glm::ivec2 resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_windowSize;
			Asset::saveDefaultFramebuffer(scene, fileName, resolution.x, resolution.y);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void saveGbuffer(Scene::Scene& scene)
	{
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* recordSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RECORD_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);
		std::string fileName = Debug::formatText(recordSettings->component<RecordSettings::RecordSettingsComponent>().m_outputFileName);

		Asset::saveGbuffer(scene, fileName, 
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite, 
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers,
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution.x, 
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution.y, 
			camera->component<Camera::CameraComponent>().m_near, 
			camera->component<Camera::CameraComponent>().m_far);
	}
}