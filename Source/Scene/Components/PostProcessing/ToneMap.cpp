#include "PCH.h"
#include "ToneMap.h"

namespace ToneMap
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(TONEMAP);
	DEFINE_OBJECT(TONEMAP);
	REGISTER_OBJECT_RENDER_CALLBACK(TONEMAP, "Tone Map", OpenGL, AFTER, "Tone Map [Begin]", 1, &ToneMap::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	GLenum getLuminanceTextureFormat(Scene::Scene& scene, Scene::Object* object)
	{
		return GL_RG16F;
	}

	////////////////////////////////////////////////////////////////////////////////
	GLenum getLuminanceTextureLayout(Scene::Scene& scene, Scene::Object* object)
	{
		return GL_RG;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initFramebuffers(Scene::Scene& scene, Scene::Object* object)
	{
		// Max supported resolution
		const glm::ivec2 resolution = GPU::maxResolution();

		GLenum format = getLuminanceTextureFormat(scene, object);
		GLenum layout = getLuminanceTextureLayout(scene, object);

		Scene::createTexture(scene, "ToneMap_Luminance_0", GL_TEXTURE_2D, resolution[0], resolution[1], 1, format, layout, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		Scene::createTexture(scene, "ToneMap_Luminance_1", GL_TEXTURE_2D, resolution[0], resolution[1], 1, format, layout, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			ToneMap::TonemapComponent::ExposureMethod_meta,
			ToneMap::TonemapComponent::KeyMethod_meta,
			ToneMap::TonemapComponent::ToneMapOperator_meta,
			ToneMap::TonemapComponent::AdaptationMethod_meta
		);

		// Luminance generation program
		Asset::loadShader(scene, "PostProcessing/ToneMap", "luminance", "ToneMap/luminance", shaderParameters);

		// Luminance mipmap generation program
		Asset::loadShader(scene, "PostProcessing/ToneMap", "generate_luminance_mipmap", "ToneMap/generate_luminance_mipmap", shaderParameters);

		// Tone mapping program
		Asset::loadShader(scene, "PostProcessing/ToneMap", "tonemap", "ToneMap/tonemap", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		Scene::createGPUBuffer(scene, "ToneMap", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, initFramebuffers, "FrameBuffers");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, initGPUBuffers, "Generic GPU Buffers");
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurveClamp(Scene::Scene& scene, Scene::Object* object, float x)
	{
		return glm::clamp(x, 0.0f, 1.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurveReinhard(Scene::Scene& scene, Scene::Object* object, float x)
	{
		const float w = object->component<ToneMap::TonemapComponent>().m_linearWhite;
		return (x * (1.0f + x / (w * w))) / (1.0f + x);
	}

	////////////////////////////////////////////////////////////////////////////////
	float curveFilmic(Scene::Scene& scene, Scene::Object* object, float x)
	{
		const float A = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_shoulderStrength;
		const float B = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearStrength;
		const float C = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearAngle;
		const float D = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeStrength;
		const float E = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeNumerator;
		const float F = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeDenominator;

		return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	float evalCurveFilmic(Scene::Scene& scene, Scene::Object* object, float x)
	{
		return curveFilmic(scene, object, x) / curveFilmic(scene, object, object->component<ToneMap::TonemapComponent>().m_linearWhite);
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurveAces(Scene::Scene& scene, Scene::Object* object, float x)
	{
		const float a = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_a;
		const float b = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_b;
		const float c = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_c;
		const float d = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_d;
		const float e = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_e;
		return glm::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurveLottes(Scene::Scene& scene, Scene::Object* object, float x)
	{
		const float a = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_a;
		const float d = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_d;
		const float midIn = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_midIn;
		const float midOut = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_midOut;
		const float hdrMax = object->component<ToneMap::TonemapComponent>().m_linearWhite;

		const float b =
			(-pow(midIn, a) + pow(hdrMax, a) * midOut) /
			((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
		const float c =
			(pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
			((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

		return glm::pow(x, a) / (glm::pow(x, a * d) * b + c);
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurveUchimura(Scene::Scene& scene, Scene::Object* object, float x)
	{
		return 0.0f;
		/*
		const float P = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_maxBrightness;
		const float a = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_contrast;
		const float m = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_linearStart;
		const float l = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_linearLength;
		const float c = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_black;
		const float b = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_pedestal;

		const float l0 = ((P - m) * l) / a;
		const float L0 = m - m / a;
		const float L1 = m + (1.0 - m) / a;
		const float S0 = m + l0;
		const float S1 = m + a * l0;
		const float C2 = (a * P) / (P - S1);
		const float CP = -C2 / P;

		const float w0 = (1.0 - glm::smoothstep(0.0f, m, x));
		const float w2 = (glm::step(m + l0, x));
		const float w1 = (1.0 - w0 - w2);

		const float T = (m * pow(x / m, c) + b);
		const float S = (P - (P - S1) * exp(CP * (x - S0)));
		const float L = (m + a * (x - m));

		return T * w0 + L * w1 + S * w2;
		*/
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurve(Scene::Scene& scene, Scene::Object* object, TonemapComponent::ToneMapOperator op, float x)
	{
		// Reinhard operator
		if (op == ToneMap::TonemapComponent::Clamp)
			return evalCurveClamp(scene, object, x);

		// Reinhard operator
		if (op == ToneMap::TonemapComponent::Reinhard)
			return evalCurveReinhard(scene, object, x);

		// Filmic operator
		if (op == ToneMap::TonemapComponent::Filmic)
			return evalCurveFilmic(scene, object, x);

		// Aces operator
		if (op == ToneMap::TonemapComponent::Aces)
			return evalCurveAces(scene, object, x);

		// Lottes operator
		if (op == ToneMap::TonemapComponent::Lottes)
			return evalCurveLottes(scene, object, x);

		// Lottes operator
		if (op == ToneMap::TonemapComponent::Uchimura)
			return evalCurveUchimura(scene, object, x);

		// Unknown operator
		return 0.0f;
	}

	////////////////////////////////////////////////////////////////////////////////
	float evalCurve(Scene::Scene& scene, Scene::Object* object, float x)
	{
		return evalCurve(scene, object, object->component<ToneMap::TonemapComponent>().m_operator, x);
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace ToneMapCurveChart
	{
		// Total number of samples in the chart
		static const size_t NUM_SAMPLES = 200;

		// Min and max X-coords
		static const float MIN_X = 0.0f;
		static const float MAX_X = 1.0f;

		////////////////////////////////////////////////////////////////////////////////
		struct ChartGeneratorPayload
		{
			// Reference to the scene
			Scene::Scene* m_scene;

			// Necessary scene information
			Scene::Object* m_object;

			// Sample offset
			float m_sampleOffset;

			// Width of one sample
			float m_sampleWidth;
		};

		////////////////////////////////////////////////////////////////////////////////
		float computeSamplePosition(ChartGeneratorPayload const& payload, int index)
		{
			return payload.m_sampleOffset + index * payload.m_sampleWidth;
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGenerator(TonemapComponent::ToneMapOperator op, void* pPayload, int index)
		{
			// Extract the payload
			ChartGeneratorPayload const& payload = *(const ChartGeneratorPayload*)pPayload;

			// Get the sample position
			const float x = computeSamplePosition(payload, index);

			// Return the sampled track
			return ImPlotPoint(x, evalCurve(*payload.m_scene, payload.m_object, op, x));
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGeneratorClamp(void* pPayload, int index)
		{
			return chartDataGenerator(TonemapComponent::Clamp, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGeneratorReinhard(void* pPayload, int index)
		{
			return chartDataGenerator(TonemapComponent::Reinhard, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGeneratorFilmic(void* pPayload, int index)
		{
			return chartDataGenerator(TonemapComponent::Filmic, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGeneratorAces(void* pPayload, int index)
		{
			return chartDataGenerator(TonemapComponent::Aces, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGeneratorLottes(void* pPayload, int index)
		{
			return chartDataGenerator(TonemapComponent::Lottes, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint chartDataGeneratorUchimura(void* pPayload, int index)
		{
			return chartDataGenerator(TonemapComponent::Uchimura, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		using ImPlotGenerator = ImPlotPoint(*)(void* data, int idx);
		static std::unordered_map<TonemapComponent::ToneMapOperator, ImPlotGenerator> s_generators
		{
			{ TonemapComponent::Clamp, &chartDataGeneratorClamp },
			{ TonemapComponent::Reinhard, &chartDataGeneratorReinhard },
			{ TonemapComponent::Filmic,&chartDataGeneratorFilmic },
			{ TonemapComponent::Aces, &chartDataGeneratorAces },
			{ TonemapComponent::Lottes, &chartDataGeneratorLottes },
			{ TonemapComponent::Uchimura, &chartDataGeneratorUchimura },
		};

		////////////////////////////////////////////////////////////////////////////////
		void generateChart(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
		{
			// Chart generator payload
			ChartGeneratorPayload payload;
			payload.m_scene = &scene;
			payload.m_object = object;
			payload.m_sampleOffset = 0.0f;
			payload.m_sampleWidth = (MAX_X - MIN_X) / (NUM_SAMPLES - 1);

			// Axis flags
			const ImPlotFlags plotFlags = ImPlotFlags_AntiAliased | ImPlotFlags_NoMousePos;
			const ImPlotAxisFlags xFlags = ImPlotAxisFlags_AutoFit;
			const ImPlotAxisFlags yFlags = ImPlotAxisFlags_AutoFit;

			// Plot the curves
			if (ImPlot::BeginPlot("Operators", "Luminance", "Response", ImVec2(-1, 0), plotFlags, xFlags, yFlags))
			{
				// Generate the plots
				for (auto const& generator: s_generators)
				{
					// Highlight the currently active operator
					if (object->component<TonemapComponent>().m_operator == generator.first)
					{
						const float defaultWeight = ImPlot::GetStyle().LineWeight;
						ImPlot::SetNextLineStyle(ImVec4(0, 0, 0, -1), 2.0f * defaultWeight);
					}

					// Plot the operator
					std::string opName = std::string(TonemapComponent::ToneMapOperator_value_to_string(generator.first));
					ImPlot::PlotLineG(opName.c_str(), generator.second, &payload, NUM_SAMPLES);
				}

				ImPlot::EndPlot();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	int numMipLevels(Scene::Scene& scene, Scene::Object* renderSettings)
	{
		return GPU::numMipLevels(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution);
	}
	
	////////////////////////////////////////////////////////////////////////////////
	int numMipLevels(Scene::Scene& scene)
	{
		return numMipLevels(scene, Scene::filterObjects(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS)[0]);
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		if (ImGui::BeginTabBar(object->m_name.c_str()) == false)
			return;

		// Restore the selected tab id
		std::string activeTab;
		if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, object, "MainTabBar_SelectedTab#Synced"); activeTabSynced.has_value())
			activeTab = activeTabSynced.value();

		if (ImGui::BeginTabItem("Exposure", activeTab.c_str()))
		{
			ImGui::Combo("Exposure Method", &object->component<ToneMap::TonemapComponent>().m_exposureMethod, ToneMap::TonemapComponent::ExposureMethod_meta);
			if (object->component<ToneMap::TonemapComponent>().m_exposureMethod == ToneMap::TonemapComponent::FixedExposure)
			{
				ImGui::SliderFloat("Exposure", &object->component<ToneMap::TonemapComponent>().m_fixedExposure, -20.0f, 20.0f);
			}

			if (object->component<ToneMap::TonemapComponent>().m_exposureMethod == ToneMap::TonemapComponent::AutoExposure)
			{
				ImGui::SliderFloat("Exposure Bias", &object->component<ToneMap::TonemapComponent>().m_exposureBias, -20.0f, 20.0f);
				ImGui::SliderFloat("Luminance Threshold (Shadows)", &object->component<ToneMap::TonemapComponent>().m_luminanceThresholdShadows, 0.0f, 1.0f, "%.6f");
				ImGui::SliderFloat("Luminance Threshold (Highlights)", &object->component<ToneMap::TonemapComponent>().m_luminanceThresholdHighlights, 0.0f, 1.0f, "%.6f");
				ImGui::SliderFloat("Exposure Bias (Shadows)", &object->component<ToneMap::TonemapComponent>().m_exposureBiasShadows, -20.0f, 20.0f);
				ImGui::SliderFloat("Exposure Bias (Highlights)", &object->component<ToneMap::TonemapComponent>().m_exposureBiasHighlights, -20.0f, 20.0f);

				ImGui::Combo("Key Method", &object->component<ToneMap::TonemapComponent>().m_keyMethod, ToneMap::TonemapComponent::KeyMethod_meta);
				if (object->component<ToneMap::TonemapComponent>().m_keyMethod == ToneMap::TonemapComponent::FixedKey)
				{
					ImGui::SliderFloat("Key", &object->component<ToneMap::TonemapComponent>().m_fixedKey, 0.0f, 2.0f);
				}

				ImGui::Combo("Adaptation Method", &object->component<ToneMap::TonemapComponent>().m_adaptationMethod, ToneMap::TonemapComponent::AdaptationMethod_meta);
				ImGui::SliderFloat("Adaptation Rate", &object->component<ToneMap::TonemapComponent>().m_adaptationRate, 0.0f, 16.0f);
				ImGui::SliderFloat("Minimum Avg. Luminance", &object->component<ToneMap::TonemapComponent>().m_minAvgLuminance, 0.0f, 0.5f);
				ImGui::SliderFloat("Local Luminance Mip Offset", &object->component<ToneMap::TonemapComponent>().m_localLuminanceMipOffset, 0.0f, numMipLevels(scene));
				ImGui::SliderFloat("Max Local Luminance Contribution", &object->component<ToneMap::TonemapComponent>().m_maxLocalLuminanceContribution, 0.0f, 1.0f);
				
				if (ImGui::Button("Clear Luminance Map"))
				{
					ToneMap::clearLuminanceMaps(scene, object);
				}
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Operator", activeTab.c_str()))
		{
			ImGui::Combo("Tone Map Operator", &object->component<ToneMap::TonemapComponent>().m_operator, ToneMap::TonemapComponent::ToneMapOperator_meta);
			ImGui::SliderFloat("Linear White", &object->component<ToneMap::TonemapComponent>().m_linearWhite, 0.0f, 16.0f);
			if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Reinhard)
			{ }
			else if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Filmic)
			{
				ImGui::SliderFloat("Shoulder Strength", &object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_shoulderStrength, 0.0f, 1.0f);
				ImGui::SliderFloat("Linear Strength", &object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearStrength, 0.0f, 1.0f);
				ImGui::SliderFloat("Linear Angle", &object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearAngle, 0.0f, 1.0f);
				ImGui::SliderFloat("Toe Strength", &object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeStrength, 0.0f, 1.0f);
				ImGui::SliderFloat("Toe Numerator", &object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeNumerator, 0.0f, 1.0f);
				ImGui::SliderFloat("Toe Denominator", &object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeDenominator, 0.0f, 1.0f);
			}
			if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Aces)
			{
				ImGui::SliderFloat("A", &object->component<ToneMap::TonemapComponent>().m_acesSettings.m_a, 0.0f, 4.0f);
				ImGui::SliderFloat("B", &object->component<ToneMap::TonemapComponent>().m_acesSettings.m_b, 0.0f, 1.0f);
				ImGui::SliderFloat("C", &object->component<ToneMap::TonemapComponent>().m_acesSettings.m_c, 0.0f, 4.0f);
				ImGui::SliderFloat("D", &object->component<ToneMap::TonemapComponent>().m_acesSettings.m_d, 0.0f, 1.0f);
				ImGui::SliderFloat("E", &object->component<ToneMap::TonemapComponent>().m_acesSettings.m_e, 0.0f, 1.0f);
			}
			if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Lottes)
			{
				ImGui::SliderFloat("A", &object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_a, 0.0f, 4.0f);
				ImGui::SliderFloat("D", &object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_d, 0.0f, 4.0f);
				ImGui::SliderFloat("Mid In", &object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_midIn, 0.0f, 1.0f);
				ImGui::SliderFloat("Mid Out", &object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_midOut, 0.0f, 1.0f);
			}
			if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Uchimura)
			{
				ImGui::SliderFloat("Max Brightness", &object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_maxBrightness, 0.0f, 4.0f);
				ImGui::SliderFloat("Contrast", &object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_contrast, 0.0f, 4.0f);
				ImGui::SliderFloat("Linear Start", &object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_linearStart, 0.0f, 1.0f);
				ImGui::SliderFloat("Linear Length", &object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_linearLength, 0.0f, 1.0f);
				ImGui::SliderFloat("Black", &object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_black, 0.0f, 1.0f);
				ImGui::SliderFloat("Pedestal", &object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_pedestal, 0.0f, 1.0f);
			}
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ToneMapCurveChart::generateChart(scene, guiSettings, object);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Color Table", activeTab.c_str()))
		{
			if (ImGui::InputTextPreset("Color Table", object->component<ToneMap::TonemapComponent>().m_colorTable, scene.m_lutNames, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				Asset::load3DTexture(scene, object->component<ToneMap::TonemapComponent>().m_colorTable, object->component<ToneMap::TonemapComponent>().m_colorTable);
				Asset::loadTexture(scene, object->component<ToneMap::TonemapComponent>().m_colorTable + "_Preview", object->component<ToneMap::TonemapComponent>().m_colorTable);
			}
			std::string const& previewName = object->component<ToneMap::TonemapComponent>().m_colorTable + "_Preview";
			if (scene.m_textures.find(previewName) != scene.m_textures.end())
			{
				float imgHeight = 50;
				float imgWidth = imgHeight * 16.0f;
				ImGui::Image(&scene.m_textures[previewName].m_texture, ImVec2(imgWidth, imgHeight), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(0, 0, 0, 0));
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// End the tab bar
		ImGui::EndTabBar();
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Resolution to render at
		glm::ivec2 renderResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;

		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Upload the tone map parameters
		ToneMap::UniformData toneMapData;
		toneMapData.m_exposureMethod = object->component<ToneMap::TonemapComponent>().m_exposureMethod;
		toneMapData.m_fixedExposure = object->component<ToneMap::TonemapComponent>().m_fixedExposure;
		toneMapData.m_luminanceThresholdShadows = object->component<ToneMap::TonemapComponent>().m_luminanceThresholdShadows;
		toneMapData.m_luminanceThresholdHighlights = object->component<ToneMap::TonemapComponent>().m_luminanceThresholdHighlights;
		toneMapData.m_exposureBiasShadows = object->component<ToneMap::TonemapComponent>().m_exposureBiasShadows;
		toneMapData.m_exposureBiasHighlights = object->component<ToneMap::TonemapComponent>().m_exposureBiasHighlights;
		toneMapData.m_keyMethod = object->component<ToneMap::TonemapComponent>().m_keyMethod;
		toneMapData.m_fixedKey = object->component<ToneMap::TonemapComponent>().m_fixedKey;
		toneMapData.m_adaptationMethod = object->component<ToneMap::TonemapComponent>().m_adaptationMethod;
		toneMapData.m_adaptationRate = object->component<ToneMap::TonemapComponent>().m_adaptationRate;
		toneMapData.m_minAvgLuminance = object->component<ToneMap::TonemapComponent>().m_minAvgLuminance;
		toneMapData.m_numMipLevels = numMipLevels(scene, renderSettings);
		toneMapData.m_localMipLevel = glm::max(toneMapData.m_numMipLevels - object->component<ToneMap::TonemapComponent>().m_localLuminanceMipOffset, 0.0f);
		toneMapData.m_maxLocalContribution = object->component<ToneMap::TonemapComponent>().m_maxLocalLuminanceContribution;
		toneMapData.m_operator = object->component<ToneMap::TonemapComponent>().m_operator;
		toneMapData.m_exposureBias = object->component<ToneMap::TonemapComponent>().m_exposureBias;
		toneMapData.m_linearWhite = object->component<ToneMap::TonemapComponent>().m_linearWhite;
		toneMapData.m_hasColorTable = (object->component<ToneMap::TonemapComponent>().m_colorTable.empty()) ? 0.0f : 1.0f;
		if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Reinhard)
		{
			int paramId = 0;
		}
		else if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Filmic)
		{
			int paramId = 0;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_shoulderStrength;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearStrength;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_linearAngle;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeStrength;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeNumerator;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_filmicSettings.m_toeDenominator;
		}
		if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Aces)
		{
			int paramId = 0;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_a;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_b;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_c;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_d;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_acesSettings.m_e;
		}
		if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Lottes)
		{
			int paramId = 0;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_a;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_d;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_midIn;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_lottesSettings.m_midOut;
		}
		if (object->component<ToneMap::TonemapComponent>().m_operator == ToneMap::TonemapComponent::Uchimura)
		{
			int paramId = 0;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_maxBrightness;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_contrast;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_linearStart;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_linearLength;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_black;
			toneMapData.m_operatorParams[paramId++].x = object->component<ToneMap::TonemapComponent>().m_uchimuraSettings.m_pedestal;
		}
		
		uploadBufferData(scene, "ToneMap", toneMapData);

		// Name of the previous and current luminance textures
		const std::string prevLuminanceName = "ToneMap_Luminance_" + std::to_string(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferRead);
		const std::string currLuminanceName = "ToneMap_Luminance_" + std::to_string(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferWrite);

		// Generate the luminance map
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Luminance Map");

			// Bind the scene textures
			RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings);

			// Bind the previous luminance texture
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[prevLuminanceName].m_texture);

			// Compute the scene luminance
			glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures[currLuminanceName].m_framebuffer);
			RenderSettings::setupViewportOpenGL(scene, simulationSettings, renderSettings);
			Scene::bindShader(scene, "ToneMap", "luminance");

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Generate the mipmap chain
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Luminance Mipmap");

			// Bind the adapted luminance texture
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[currLuminanceName].m_texture);

			// AVG luminance: simple mipmap chain
			if (object->component<ToneMap::TonemapComponent>().m_luminanceComponents == TonemapComponent::AvgLuminance)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			// AVG & MAX luminance: manual mipmap generation
			if (object->component<ToneMap::TonemapComponent>().m_luminanceComponents == TonemapComponent::AvgMaxLuminance)
			{
				// sync barrier
				glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

				// Bind the shader
				Scene::bindShader(scene, "ToneMap", "generate_luminance_mipmap");

				// Go through the mip levels
				glm::ivec2 mipDimension = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution / 2;
				for (unsigned mipLevel = 1; mipLevel <= toneMapData.m_numMipLevels; ++mipLevel, mipDimension /= 2)
				{
					// Bind the target image
					glBindImageTexture(0, scene.m_textures[currLuminanceName].m_texture, mipLevel, GL_TRUE, 0, GL_WRITE_ONLY, getLuminanceTextureFormat(scene, object));

					// Set the necessary uniuforms
					glUniform2iv(0, 1, glm::value_ptr(mipDimension));
					glUniform1ui(1, mipLevel);

					// Dispatch the shader
					const unsigned workGroups = unsigned(glm::ceil(glm::max(mipDimension.x, mipDimension.y) / 8.0f));
					glDispatchCompute(workGroups, workGroups, workGroups);

					// sync barrier
					glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
				}
			}
		}

		// Apply the tonemap operator
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Operator Apply");

			// Bind the new buffer
			RenderSettings::bindGbufferLayersOpenGL(scene, simulationSettings, renderSettings);
			RenderSettings::setupViewportArrayOpenGL(scene, simulationSettings, renderSettings);
			Scene::bindShader(scene, "ToneMap", "tonemap");

			// Bind the adapted luminance texture
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures[currLuminanceName].m_texture);

			// Bind the lut texture
			if (object->component<ToneMap::TonemapComponent>().m_colorTable.empty() == false)
			{
				glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_2_ENUM);
				glBindTexture(GL_TEXTURE_3D, scene.m_textures[object->component<ToneMap::TonemapComponent>().m_colorTable].m_texture);
			}

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
	}

	////////////////////////////////////////////////////////////////////////////////
	void clearLuminanceMaps(Scene::Scene& scene, Scene::Object* object)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures["ToneMap_Luminance_0"].m_framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, scene.m_textures["ToneMap_Luminance_1"].m_framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}