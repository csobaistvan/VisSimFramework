#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
/// TONE MAP FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace ToneMap
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "Tonemap";
	static constexpr const char* DISPLAY_NAME = "Tonemap";
	static constexpr const char* CATEGORY = "Post Processing";

	////////////////////////////////////////////////////////////////////////////////
	/** Reinhard tone map settings. */
	struct ReinhardToneMapSettings
	{};

	////////////////////////////////////////////////////////////////////////////////
	/** Filmic tone map settings. */
	struct FilmicToneMapSettings
	{
		float m_shoulderStrength = 0.15f;
		float m_linearStrength = 0.05f;
		float m_linearAngle = 0.1f;
		float m_toeStrength = 0.2f;
		float m_toeNumerator = 0.02f;
		float m_toeDenominator = 0.3f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Aces tone map settings. */
	struct AcesToneMapSettings
	{
		float m_a = 2.51f;
		float m_b = 0.03f;
		float m_c = 2.43f;
		float m_d = 0.59f;
		float m_e = 0.14f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Lottes tone map settings. */
	struct LottesToneMapSettings
	{
		float m_a = 1.6f;
		float m_d = 0.977f;
		float m_midIn = 0.18f;
		float m_midOut = 0.267f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uchimura tone map settings. */
	struct UchimuraToneMapSettings
	{
		float m_maxBrightness = 1.0f;
		float m_contrast = 1.0f;
		float m_linearStart = 0.22f;
		float m_linearLength = 0.4f;
		float m_black = 1.33f;
		float m_pedestal = 0.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A tone mapper component. */
	struct TonemapComponent
	{
		// Various operator types
		meta_enum(ToneMapOperator, int, Clamp, Reinhard, Filmic, Aces, Lottes, Uchimura);

		// Luminance adaptation method
		meta_enum(ExposureMethod, int, AutoExposure, FixedExposure);

		// Luminance adaptation method
		meta_enum(KeyMethod, int, AutoKey, FixedKey);

		// Luminance adaptation method
		meta_enum(AdaptationMethod, int, Lerp, Exponential);

		// What components to store in the luminance texture
		meta_enum(LuminanceComponents, int, AvgLuminance, AvgMaxLuminance);

		// Luminance update method
		AdaptationMethod m_adaptationMethod = Lerp;

		// Exposure method
		ExposureMethod m_exposureMethod = AutoExposure;

		// Which luminance components to store
		LuminanceComponents m_luminanceComponents = AvgLuminance;

		// The fixed key exposure
		float m_fixedExposure = 10.0f;

		// Key method
		KeyMethod m_keyMethod = AutoKey;

		// The fixed key value
		float m_fixedKey = 0.115f;

		// Minimum value for the average luminance
		float m_minAvgLuminance = 0.001f;

		// Adaptation rate
		float m_adaptationRate = 2.0f;

		// Mip level to use for the average luminance lookup
		float m_localLuminanceMipOffset = 0.0f;

		// Max contribution of the local luminance
		float m_maxLocalLuminanceContribution = 0.3f;

		// Tone mapper operator
		ToneMapOperator m_operator = Aces;

		// Exposure bias
		float m_exposureBias = 0.0f;

		// Shadows-highlights parameters
		float m_luminanceThresholdShadows = 0.05f;
		float m_luminanceThresholdHighlights = 0.95f;
		float m_exposureBiasShadows = 0.0f;
		float m_exposureBiasHighlights = 0.0f;

		// Linear white
		float m_linearWhite = 11.2f;

		// Settings for the various tonemappers
		ReinhardToneMapSettings m_reinhardSettings;
		FilmicToneMapSettings m_filmicSettings;
		AcesToneMapSettings m_acesSettings;
		LottesToneMapSettings m_lottesSettings;
		UchimuraToneMapSettings m_uchimuraSettings;

		// Lut texture name
		std::string m_colorTable;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the tone map data. */
	struct UniformData
	{
		GLuint m_exposureMethod;
		GLfloat m_fixedExposure;
		GLfloat m_exposureBias;
		GLfloat m_luminanceThresholdShadows;
		GLfloat m_luminanceThresholdHighlights;
		GLfloat m_exposureBiasShadows;
		GLfloat m_exposureBiasHighlights;
		GLuint m_keyMethod;
		GLfloat m_fixedKey;
		GLuint m_adaptationMethod;
		GLfloat m_adaptationRate;
		GLfloat m_minAvgLuminance;
		GLfloat m_numMipLevels;
		GLfloat m_localMipLevel;
		GLfloat m_maxLocalContribution;
		GLuint m_operator;
		GLfloat m_linearWhite;
		GLfloat m_hasColorTable;
		alignas(sizeof(glm::vec4)) glm::vec4 m_operatorParams[8];
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void clearLuminanceMaps(Scene::Scene& scene, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(TONEMAP, TonemapComponent, ToneMap::TonemapComponent)
DECLARE_OBJECT(TONEMAP, COMPONENT_ID_TONEMAP, COMPONENT_ID_EDITOR_SETTINGS)