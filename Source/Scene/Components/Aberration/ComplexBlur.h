#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "WavefrontAberration.h"

////////////////////////////////////////////////////////////////////////////////
/// COMPLEX BLUR FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace ComplexBlur
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "ComplexBlur";
	static constexpr const char* DISPLAY_NAME = "Complex Blur";
	static constexpr const char* CATEGORY = "Aberration";

	////////////////////////////////////////////////////////////////////////////////
	static constexpr int MAX_COMPONENTS = 2;
	static constexpr int MAX_TEXTURES = 3;
	static constexpr int MAX_KERNEL_RADIUS = 32;
	static constexpr int MAX_OBJECT_DISTANCES = 128;

	////////////////////////////////////////////////////////////////////////////////
	/** A complete complex blur kernel. */
	struct ComplexBlurKernelComponent
	{
		float m_a;
		float m_b;
		float m_A;
		float m_B;
	};
	using ComplexBlurKernelComponents = std::vector<ComplexBlurKernelComponent>;

	////////////////////////////////////////////////////////////////////////////////
	struct ComplexBlurKernelParameters
	{
		float m_radius;
		ComplexBlurKernelComponents m_components;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A convolution-ready complex blur kernel. */
	struct ComplexBlurConvolutionKernel
	{
		ComplexBlurConvolutionKernel()
		{}

		ComplexBlurConvolutionKernel(size_t numComponents) :
			m_horizontal(numComponents),
			m_vertical(numComponents),
			m_offsets(numComponents),
			m_scales(numComponents)
		{}

		std::vector<ComplexBlurKernelComponents> m_horizontal;
		std::vector<ComplexBlurKernelComponents> m_vertical;
		std::vector<glm::vec4> m_offsets;
		std::vector<glm::vec4> m_scales;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A complex-domain, circular blur component. */
	struct ComplexBlurComponent
	{
		// The various output modes available, mainly for debugging
		meta_enum(OutputMode, int, Convolution, BlurRadius, DilatedBlurRadius, Coverage, Near, Far, Focus);

		// Eye aberration description
		Aberration::WavefrontAberration m_aberration;

		// Output mode
		OutputMode m_outputMode;

		// Resolution at which to render
		int m_renderResolutionId;

		// Number of dilation passes to perform
		int m_dilationPasses = 4;

		// Search radius for a single dilation pass
		int m_dilationSearchRadius = 4;

		// Number of kernel components used
		int m_numComponents = 1;

		// Number of kernel weights
		int m_kernelTapsRadius = 8;

		// Screne-space size of the kernel (in microns)
		float m_blurSize = 32.0f;

		// Ellipse parameters
		float m_ellipseRotation = 0.0f;
		float m_ellipseRatio = 1.0f;
		float m_ellipseContraction = 1.0f;

		// Whether we should align the kernel to the PSF or not
		bool m_alignKernelToPsf = false;

		struct AlignKernelSettings
		{
			// Ellipse fit threshold
			float m_ellipseThreshold = 0.05f;
			float m_targetDefocus = 35.0f;
			bool m_exportPsf = false;
		} m_alignKernelSettings;

		// Whether we should fit the kernel to the PSF or not
		bool m_fitKernelToPsf = false;

		// Settings for kernel fitting
		struct FitKernelSettings
		{
			int m_maxIterations = 1000;
			float maxMinutes = 10.0f;
			int m_fitScale = 1;
			float m_diffStepSize = 1e-6f;
			float m_targetDefocus = 35.0f;
			float m_ellipseThreshold = 0.05f;
			bool m_logProgress = true;
			bool m_projectPsf = true;
			bool m_exportPsf = false;

			glm::vec4 m_initialComponents{ 1.0f, 0.0f, 1.0f, 0.0f };
			glm::vec2 m_radiusLimits{ 1.0f, 3.0f };
			glm::vec2 m_aLimits{ -5.0f, 5.0f };
			glm::vec2 m_bLimits{ -5.0f, 5.0f };
			glm::vec2 m_ALimits{ -5.0f, 5.0f };
			glm::vec2 m_BLimits{ -5.0f, 5.0f };
		} m_fitKernelSettings;

		// ---- Private members

		// Mean-squared error of the fit
		float m_fitMav = 0.0f;
		float m_fitMse = 0.0f;
		float m_fitRmse = 0.0f;
		float m_fitPsnr = 0.0f;

		// All the aberration presets available.
		Aberration::WavefrontAberrationPresets m_aberrationPresets;

		// Components of the current kernel
		std::vector<ComplexBlurKernelParameters> m_kernels;

		// Kernel weights
		ComplexBlurConvolutionKernel m_convolutionKernel;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the complex blur data. */
	struct UniformDataCommon
	{
		glm::vec2 m_resolution;
		glm::vec2 m_dilatedResolution;
		glm::vec2 m_uvScale;
		glm::vec2 m_horizontalDir;
		glm::vec2 m_verticalDir;
		GLuint m_outputMode;
		GLuint m_dilationPasses;
		GLuint m_dilationSearchRadius;
		GLuint m_dilatedTileSize;
		GLuint m_kernelTaps;
		GLfloat m_maxBlurSize;
		GLfloat m_sampleSize;
		GLfloat m_ellipseRotation;
		GLfloat m_ellipseRatio;
		GLfloat m_ellipseContraction;
		GLuint m_numObjectDistances;
		GLfloat m_objectDistancesMin;
		GLfloat m_objectDistancesMax;
		GLfloat m_objectDistancesStep;
		alignas(sizeof(glm::vec4)) glm::vec4 m_blurSizes[MAX_OBJECT_DISTANCES];
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the complex blur weights. */
	struct UniformDataWeights
	{
		alignas(sizeof(glm::vec4)) glm::vec4 m_weights[MAX_COMPONENTS];
		alignas(sizeof(glm::vec4)) glm::vec4 m_bracketsHorizontal[MAX_COMPONENTS];
		alignas(sizeof(glm::vec4)) glm::vec4 m_bracketsVertical[MAX_COMPONENTS];
		alignas(sizeof(glm::vec4)) glm::vec4 m_componentsHorizontal[MAX_COMPONENTS * (MAX_KERNEL_RADIUS * 2 + 1)];
		alignas(sizeof(glm::vec4)) glm::vec4 m_componentsVertical[MAX_COMPONENTS * (MAX_KERNEL_RADIUS * 2 + 1)];
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
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void demoSetup(Scene::Scene& scene);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(COMPLEX_BLUR, ComplexBlurComponent, ComplexBlur::ComplexBlurComponent)
DECLARE_OBJECT(COMPLEX_BLUR, COMPONENT_ID_COMPLEX_BLUR, COMPONENT_ID_EDITOR_SETTINGS)