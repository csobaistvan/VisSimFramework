#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
/// ABERRATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace Aberration
{
	////////////////////////////////////////////////////////////////////////////////
	/** What precision to use throughout the computation process. */
	using ScalarZernikeCoeffs = float;
	using ScalarComputation = long double;
	using ScalarFinal = float;
	using ComplexZernikeCoeffs = std::complex<ScalarZernikeCoeffs>;
	using ComplexComputation = std::complex<ScalarComputation>;
	using ComplexFinal = std::complex<ScalarFinal>;

	namespace EigenTypes
	{
		// Scalar row vectors
		using ScalarRowVectorComputation = Eigen::Matrix<ScalarComputation, 1, Eigen::Dynamic>;

		// Complex row vectors
		using ComplexRowVectorComputation = Eigen::Matrix<ComplexComputation, 1, Eigen::Dynamic>;

		// Scalar matrices
		using ScalarMatrixComputation = Eigen::Matrix<ScalarComputation, Eigen::Dynamic, Eigen::Dynamic>;
		using ScalarMatrixFinal = Eigen::Matrix<ScalarFinal, Eigen::Dynamic, Eigen::Dynamic>;

		// Complex matrices
		using ComplexMatrixComputation = Eigen::Matrix<ComplexComputation, Eigen::Dynamic, Eigen::Dynamic>;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Number of zernike coefficients in total for a given maximum degree. */
	static constexpr size_t numZernikeCoefficients(int maxDegree, bool positiveOnly = false)
	{
		return (positiveOnly ? (maxDegree / 2 + 1) : (maxDegree + 1)) +
			(maxDegree == 0 ? 0 : numZernikeCoefficients(maxDegree - 1, positiveOnly));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** An object holding all the zernike coefficient values. */
	template<typename ScalarType, typename CoeffType>
	struct TZernikeCoefficients : public std::vector<CoeffType>
	{
		static const int MAX_DEGREES = 6;
		static const int NUM_COEFFS = numZernikeCoefficients(MAX_DEGREES);

		TZernikeCoefficients(int maxDegree = MAX_DEGREES) :
			std::vector<CoeffType>(numZernikeCoefficients(maxDegree) + 1, CoeffType(ScalarType(0)))
		{}
		TZernikeCoefficients(std::initializer_list<CoeffType> coeffs) :
			std::vector<CoeffType>(NUM_COEFFS + 1, CoeffType(ScalarType(0)))
		{
			std::copy(coeffs.begin(), coeffs.end(), this->begin() + 1);
		}
		TZernikeCoefficients(std::vector<CoeffType> const& coeffs) :
			std::vector<CoeffType>(NUM_COEFFS + 1, CoeffType(ScalarType(0)))
		{
			std::copy(coeffs.begin(), coeffs.end(), this->begin() + 1);
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Alpha and beta Zernike coefficients */
	using ZernikeCoefficientsAlpha = TZernikeCoefficients<ScalarZernikeCoeffs, ScalarZernikeCoeffs>;
	using ZernikeCoefficientsBeta = TZernikeCoefficients<ScalarZernikeCoeffs, ComplexZernikeCoeffs>;

	////////////////////////////////////////////////////////////////////////////////
	/** Spectacle lens parameters. */
	struct SpectacleLens
	{
		float m_sphere;
		float m_cylinder;
		float m_axis;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Used to represent a PSF image. */
	using Psf = EigenTypes::ScalarMatrixFinal;
	using PsfGpu = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

	////////////////////////////////////////////////////////////////////////////////
	/** A list of indices identifying a single PSF in the stack. The indices are organized as follows:
	*     [0] Object depth;
	*     [1] Horizontal axis;
	*     [2] Vertical axis;
	*     [3] Wavelength;
	*     [4] Aperture diameter;
	*     [5] Focus distance;
	*/
	using PsfIndex = std::array<size_t, 6>;

	////////////////////////////////////////////////////////////////////////////////
	/** A list of indices identifying a single PSF coefficient set in the stack. The indices are organized as follows:
	*     [0] Horizontal axis;
	*     [1] Vertical axis;
	*     [2] Wavelength;
	*     [3] Aperture diameter;
	*     [4] Focus distance;
	*/
	using PsfCoeffsIndex = std::array<size_t, 5>;

	////////////////////////////////////////////////////////////////////////////////
	/** Parameters describing the aberration itself. */
	struct AberrationParameters
	{
		// How the aberration is described
		meta_enum(AberrationType, int, Spectacle, Preset);

		// Diameter of the aperture (in millimeters).
		float m_apertureDiameter = 5.0f;

		// Wavelength for which the aberration was measured.
		float m_lambda = 587.56f;

		// The method used for describing the aberration.
		AberrationType m_type = Preset;

		// Spectacle lens prescription. Converted to Zernike coefficients (internally).
		SpectacleLens m_spectacleLens = { 0.0f, 0.0f, glm::radians(90.0f) };

		// Zernike coefficients.
		ZernikeCoefficientsAlpha m_coefficients;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Eye reconstruction parameters. */
	struct EyeReconstructionParameters
	{
		// Reconstruction optimizer
		meta_enum(Optimizer, int, PatternSearch);

		// Reconstruction solver
		meta_enum(Solver, int, GpsNp1, Gps2N, GssNp1, Gss2N, MadsNp1, Mads2N, GpsNp1Mads2N, Gps2NMads2N, GssNp1Mads2N, Gss2NMads2N, MadsNp1Gps2N, Mads2NGps2N);

		// Number of rays
		int m_numRays = 500;

		// Time limit
		float m_timeLimit = 24 * 60.0f;

		// Anatomical weights
		float m_anatomicalWeightBoundary = 0.1f;
		float m_anatomicalWeightAverage = 0.1f;

		// Functional weights
		float m_functionalWeightSpecified = 200.0f;
		float m_functionalWeightUnspecified = 200.0f;

		// Optimizer
		Optimizer m_optimizer = PatternSearch;

		// Solver
		Solver m_solver = Gps2N;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** PSF stack evaluation parameters. */
	struct PSFStackParameters
	{
		// Computation backend to use
		meta_enum(ComputationBackend, int, CPU, GPU);

		// Eye estimation methods
		meta_enum(EyeEstimationMethod, int, Matlab, NeuralNetworks);

		// Pupil sampling methods for alpha to beta conversion
		meta_enum(PupilSampling, int, LinearSampling, SquareRootSampling, CosineSampling);

		// Interpolation methods for the kernel resampling
		meta_enum(InterpolationType, int, Nearest, Bilinear, Cubic, Lanczos, Area);

		// Which aberration coefficient to use
		meta_enum(CoefficientVariation, int, AlphaOpd, AlphaPhaseCumulative, AlphaPhaseResidual, Beta);

		/** Represents a parameter range. */
		struct ParameterRange
		{
			float m_min;
			float m_max;
			int m_numSteps;
			float m_step;
		};

		// Computation-related settings
		ComputationBackend m_backend = GPU;

		// List of all the parameter ranges for which to generate PSF's
		ParameterRange m_objectDistances{ 0.125f, 10.125f, 41 }; // Object distance dioptres
		ParameterRange m_incidentAnglesHorizontal{ 0.0f, 0.0f, 1 }; // Horizontal incident angles
		ParameterRange m_incidentAnglesVertical{ 0.0f, 0.0f, 1 }; // Vertical incident angles
		ParameterRange m_apertureDiameters{ 5.0f, 5.0f, 1 }; // Aperture diameters
		ParameterRange m_focusDistances{ 0.125f, 0.125f, 1 }; // Focus distance dioptres
		std::vector<float> m_lambdas{ 612.0f, 549.0f, 464.0f }; // Wavelengths; default to sRGB primary (https://clarkvision.com/articles/color-spaces/)

		// Manual parameter support
		bool m_manualDefocus = false; // Whether we want to supply manual defocuses or not
		bool m_manualCoefficients = false; // Whether we want to manually supply our alpha coefficients or not
		ZernikeCoefficientsAlpha m_desiredCoefficients; // The desired manual coefficients
		float m_desiredDefocus = 0.0f; // The desired manual defocus
		float m_desiredPupilRetinaDistance = 0.0f; // The desired manual pupil-retina distance

		// Eye estimation parameters
		EyeEstimationMethod m_eyeEstimationMethod = NeuralNetworks; // Which method to use for eye estimation
		bool m_forceOnAxisNetwork = false; // Whether we should always use the on-axis network or not

		// Alpha to beta conversion parameters
		CoefficientVariation m_alphaToBetaCoefficient = AlphaPhaseCumulative; // Source alpha coefficients to generate betas
		PupilSampling m_alphaToBetaLSampling = LinearSampling; // Sampling strategy for the L parameter (angle/theta)
		PupilSampling m_alphaToBetaKSampling = CosineSampling; // Sampling strategy for the K parameter (radius/rho)
		int m_alphaToBetaL = 350; // Number of samples for L (angle/theta)
		int m_alphaToBetaK = 6; // Number of samples for K (radius/rho)
		int m_betaDegrees = 30; // Highest degree of the generated complex beta coefficients
		float m_betaThreshold = 0.0f; // Minimum value of beta to consider; can speed up PSF calculation

		// Approximation sampling parameters
		float m_approximationSampleSize = 1.0f / 5.0f; // Sample size for the radius parameter
		float m_approximationTermsMultiplier = 80.0f; // Multiplier of sampling units for calculating the highest order of 'k'
		int m_approximationTermsMin = 100; // Minimum number of approximation terms to use
		int m_approximationTermsMax = 1100; // Maximum number of approximation terms to use
		int m_maxApproximationSamples = 3800; // Maximum number of approximation samples allowed
		int m_besselBatchSize = 50; // Batch size for the Bessel coefficient computations
		bool m_precomputeVnmLSum = true; // Whether we can pre-compute the sum of the inner Vnm terms (over L)

		// PSF sampling parameters
		float m_minSamplingUnits = 2.0f; // Lower threshold for the sampling units
		float m_maxSamplingUnits = 50.0f; // Upper threshold for the sampling units
		float m_psfSampleSizeMultiplier = 0.5f; // Size of one psf sample per sampling units
		float m_psfSampleCountMultiplier = 25.0f; // How many samples per one sampling unit
		int m_psfSamplesMin = 300; // Minimum number of samples
		int m_psfSamplesMax = 300; // Maximum number of samples

		// PSF downscaling parameters
		InterpolationType m_interpolationType = Area; // Interpolation algorithm
		float m_cropThresholdSum = 1.0f; // Threshold for cropping the PSF sum; 1.0 means no cropping
		float m_cropThresholdCoeff = 0.0f; // Threshold for cropping the PSF coeffs; 0.0 means no cropping

		// Logging, debugging, etc.
		bool m_omitVnmCalculation = false; // Whether we should omit the computation of the actual Vnm; for timing purposes only
		bool m_omitPsfCalculation = false; // Whether we should omit the computation of the actual PSF; for timing purposes only
		bool m_logProgress = false; // Whether we want to log our progress or not
		bool m_logStats = false; // Whether we want to log statistics or not
		bool m_logDebug = false; // Whether we want to log debug stats or not
		bool m_truncateStatsTiming = true; // Whether we should truncate displayed timings or not
		bool m_shortenStatsTiming = false; // Whether we should shorten displayed timings or not
		bool m_collectDebugInfo = false; // Whether we should also collect debug information

		// ---- Private members

		struct EvaluatedRanges
		{
			// The original ranges
			ParameterRange m_objectDistancesRange; // Object distance dioptres
			ParameterRange m_incidentAnglesHorizontalRange; // Horizontal incident angles
			ParameterRange m_incidentAnglesVerticalRange; // Vertical incident angles
			ParameterRange m_apertureDiametersRange; // Aperture diameters
			ParameterRange m_focusDistancesRange; // Focus distance dioptres

			// Evaluated parameters
			std::vector<float> m_objectDistances;
			std::vector<float> m_objectDioptres;
			std::vector<float> m_incidentAnglesHorizontal;
			std::vector<float> m_incidentAnglesVertical;
			std::vector<float> m_lambdas;
			std::vector<float> m_apertureDiameters;
			std::vector<float> m_focusDioptres;
			std::vector<float> m_focusDistances;
		} m_evaluatedParameters;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** PSF stack preview parameters. */
	struct PSFStackPreviewParameters
	{
		// PSF indices
		int m_psfId = 0; // Id of the display PSF
		int m_horizontalId = 0; // Id of the horizontal angle
		int m_verticalId = 0; // Id of the horizontal angle
		int m_lambdaId = 0; // Id of the display lambda
		int m_apertureId = 0; // Id of the display aperure size
		int m_focusId = 0; // Id of the display aperure size

		// Render parameters
		int m_resolutionId = 0; // Render resolution ID
		glm::ivec2 m_resolution; // Render resolution
		float m_fovy = glm::radians(60.0f); // Vertical fov
		
		struct PsfTexture
		{
			PsfTexture(std::string const& source="") :
				m_fullRes("_" + source + "_FullRes"),
				m_lowRes("_" + source + "_LowRes"),
				m_projected("_" + source + "_Projected")
			{}

			// Display radius
			float m_radius = 16.0f;

			// ---- Private members

			// ID of when the texture was last uploaded
			int m_lastUploadFrameId = -1;

			// Texture suffixes
			std::string m_fullRes;   // Full-scale PSF
			std::string m_lowRes;    // PSF downscaled to the desired size
			std::string m_projected; // PSF downscaled to the projected size (via the camera)
		};

		// Preview options for the main stack
		struct PsfStack
		{
			// Should we display everything at the same scale
			bool m_sameScale = false;

			// Display texture settings
			PsfTexture m_texture{ "PsfStack" };
		} m_psfStack;

		// Preview options for the interactive PSF visualizer
		struct Psf
		{
			// The various Enz interpolation methods
			meta_enum(ValueMode, int, Defocus, Depth);

			// What type of value we have
			ValueMode m_valueMode = Depth;

			// Display depth/defocus
			float m_value = 1.0f;

			// Diopter precision
			int m_dioptresPrecision = 6;

			// Other PSF parameters
			float m_horizontalAngle = 0.0f;
			float m_verticalAngle = 0.0f;
			float m_focusDistance = 8.0f;
			float m_aperture = 5.0f;
			float m_lambda = 550.0f;

			// Display limits
			glm::vec2 m_valueLimits{ 0.1f, 1.0f };

			// Do we want to use our own Zernike coefficients or not
			bool m_useManualCoefficients = true;

			// Manual Zernike coefficients
			ZernikeCoefficientsAlpha m_manualCoefficients;

			// Manual focal length related parameters
			float m_manualPupilRetinaDistance = 17.0f;

			// Display texture settings
			PsfTexture m_texture{ "Psf" };
		} m_psf;

		struct Coefficient
		{
			// The various data visualization methods
			meta_enum(VisualizationMethod, int, Alpha, AlphaPhaseCumulative, AlphaPhaseResidual, BetaReal, BetaImag, BetaMagnitude, BetaPhase);

			// The actual visualization method used
			VisualizationMethod m_visualizationMethod = BetaMagnitude;
		} m_coefficient;

		// Focus
		struct Focus
		{
			// Which axis to display
			meta_enum(DisplayAxis, int, ObjectDistance, HorizontalAngle, VerticalAngle, ApertureDiameter, FocusDistance);

			// The various data visualization methods
			meta_enum(VisualizationMethod, int, DefocusParam, ImageShiftAberration, ImageShiftObjectDistance, ImageShift, BlurRadius);

			// The actual visualization method used
			VisualizationMethod m_visualizationMethod = BlurRadius;

			// X axis to display
			DisplayAxis m_displayAxis = ObjectDistance;
		} m_focus;
	};

	////////////////////////////////////////////////////////////////////////////////
	namespace PsfStackElements
	{
		////////////////////////////////////////////////////////////////////////////////
		struct AberrationCoefficientVariants
		{
			ZernikeCoefficientsAlpha m_alpha; // Real-valued OPD (alpha)
			ZernikeCoefficientsAlpha m_alphaTrue; // True real-valued OPD (alpha) (only if available)
			ZernikeCoefficientsAlpha m_alphaPhaseCumulative; // Real-valued cumulative phase (alpha)
			ZernikeCoefficientsAlpha m_alphaPhaseResidual; // Real-valued residual phase (alpha)
			ZernikeCoefficientsBeta m_beta; // Complex aberration (beta)
			std::unordered_map<std::string, float> m_eyeParameters; // List of eye parameters that generated these coefficients
		};

		////////////////////////////////////////////////////////////////////////////////
		/** Aberration coefficients in the form:
		*     [0] Horizontal angle;
		*     [1] Vertical angle;
		*     [2] Lambda;
		*     [3] Aperture diameter;
		*     [4] Focus distance;
		*/
		using AberrationCoefficients = boost::multi_array<AberrationCoefficientVariants, 5>;

		////////////////////////////////////////////////////////////////////////////////
		/** Eye structure parameters for a single entry of the PSF stack. */
		struct RelaxedEyeParameters
		{
			TensorFlow::DataSample m_eyeParameters; // The eye parameter Tensor
		};

		////////////////////////////////////////////////////////////////////////////////
		/** Eye structure parameters for a single entry of the PSF stack. */
		struct FocusedEyeParams
		{
			float m_pupilRetinaDistance; // The distance between the pupil and the retina
			TensorFlow::DataSample m_eyeParameters; // The eye parameter Tensor
			struct DebugInformation
			{
				float m_trueLensD; // True focused lens diameter
				float m_trueAqueousT; // True aqueous thickness
				float m_trueFocusDistance; // The true focus distance of the eye
			} m_debugInformation;
		};

		////////////////////////////////////////////////////////////////////////////////
		/** Focused eye parameters, organized as follows:
		*     [0] Aperture diameter;
		*     [1] Focus distance;
		*/
		using FocusedEyeParameters = boost::multi_array<FocusedEyeParams, 2>;

		////////////////////////////////////////////////////////////////////////////////
		/** A list of non-zero coeffs. */
		using Coefficients = std::vector<size_t>;

		////////////////////////////////////////////////////////////////////////////////
		/** PSF stack entry sampling parameters. */
		struct EnzEntrySamplingParameters
		{
			int m_maxDegree; // Max Beta degree
			int m_maxCoefficients; // Max number of coefficients
			int m_maxOrder; // Max approximation terms
			int m_maxTermsPerOrder; // Maximum terms for each order of 'k' for the w_kl terms
			int m_maxTermOrder; // Maximum order for the sub-terms
			int m_maxSamples; // Max approximation samples
			float m_maxExtent; // Max extent, in microns
			EigenTypes::ScalarRowVectorComputation m_radius; // Radial coordinates
			EigenTypes::ScalarRowVectorComputation m_defocusParams; // Defocuses

			// ---- Private members

			bool m_isWklDirty = false; // Whether the Wkl-related params changed or not
			bool m_isCylindricalDirty = false; // Whether the Cylindrical Bessel params changed or not
		};

		////////////////////////////////////////////////////////////////////////////////
		/** PSF stack entry sampling parameters. */
		struct PsfEntryParams
		{
			struct Units
			{
				float m_focusDistanceM; // Focus distance, in meters
				float m_objectDistanceM; // Object distance, in meters
				float m_horizontalAngle; // Horizontal incident angle
				float m_verticalAngle; // Vertical incident angle
				float m_apertureDiameterMM; // Aperture diameter, in millimeters
				float m_apertureDiameterMuM; // Aperture diameter, in microns
				float m_apertureRadiusMuM; // Aperture radius, in microns
				float m_pupilRetinaDistanceM; // Distance from the pupil to the retina in meters
				float m_pupilRetinaDistanceMuM; // Distance from the pupil to the retina in microns
				float m_s0; // Numerical aperture
				float m_u0; // Geometrical aperture
				float m_refractiveIndex; // Refractive index inside the optical system
				float m_lambdaMuM; // Wavelength of light, in microns
				float m_diffractionUnit; // Diffraction unit / field unit
				float m_axialDiffractionUnit; // Axial diffraction unit
				float m_focalShiftToDefocus; // Focal shift to defocus conversion multiplier
			} m_units;

			struct Sampling
			{
				float m_samplingUnits; // Sampling units
				float m_samplingMuM; // Pupil sampling, in microns
				int m_samples; // PSF size, in pixels (total)
				float m_halfExtentMuM; // Pupil end coordinates (in microns)
				float m_halfExtent; // Pupil end coordinates (in axial diffraction units)
			} m_sampling;

			struct EnzSampling
			{
				float m_sampling; // Pupil sampling
				int m_terms; // Approximation terms
				int m_samples; // Approximation samples
				float m_extent; // Extent, in microns
			} m_enzSampling;

			struct Focus
			{
				float m_imageShiftAberrationMuM; // Image shift coming from the aberrations, in microns
				float m_imageShiftObjectDepthMuM; // Image shift due to the object depth difference, in microns
				float m_imageShiftMuM; // Total image shift in depth, in micrometers 
				double m_defocusParam; // Corresponding defocus parameter
				double m_defocusUnits; // Defocus units (defocus / (pi / 2))
			} m_focus;

			struct ZernikeCoefficientsVariations
			{
				ZernikeCoefficientsAlpha m_alpha; // Real-valued OPD (alpha)
				ZernikeCoefficientsAlpha m_alphaPhaseCumulative; // Real-valued cumulative phase (alpha)
				ZernikeCoefficientsAlpha m_alphaPhaseResidual; // Real-valued residual phase (alpha)
				ZernikeCoefficientsBeta m_beta; // Complex aberration (beta)
			} m_coefficients;

			struct DebugInformation
			{
				std::unordered_map<std::string, float> m_eyeParameters; // List of eye parameters corresponding to the input
				ZernikeCoefficientsAlpha m_alphaTrue; // Aberration coefficients generated by the reconstructed model
				float m_focusDistance; // True focus distance of this eye
			} m_debug;
		};

		////////////////////////////////////////////////////////////////////////////////
		/** The various entry parameters for each parameter combination. They are organized as follows:
		*     [0] Object depth;
		*     [1] Horizontal axis;
		*     [2] Vertical axis;
		*     [3] Wavelength;
		*     [4] Aperture diameter;
		*     [5] Focus distance;
		*/
		using PsfEntryParameters = boost::multi_array<PsfEntryParams, 6>;

		////////////////////////////////////////////////////////////////////////////////
		/** The ENZ Vnm coefficients (wkl). They are organized as follows:
		*     [0] n;
		*     [1] m;
		*     [2] k;
		*     [3] l;
		*/
		using EnzCoefficients = boost::multi_array<double, 4>;

		////////////////////////////////////////////////////////////////////////////////
		/** Accel structure for the spherical Bessel values used in the ENZ terms.
		*   They are organized as follows:
		*     [0] k;
		*/
		using EnzSphericalBessels = std::vector<EigenTypes::ScalarRowVectorComputation>;

		////////////////////////////////////////////////////////////////////////////////
		/** Accel structure for the cylindrical Bessel values used in the ENZ terms.
		*   They are organized as follows:
		*     [0] k;
		*/
		using EnzCylindricalBessels = std::vector<EigenTypes::ScalarRowVectorComputation>;

		////////////////////////////////////////////////////////////////////////////////
		/** Accel structure for the cylindrical Bessel values used in the ENZ terms.
		*   They are organized as follows:
		*     [0] n;
		*     [1] m;
		*     [2] k;
		*/
		using EnzVnmInnerTerms = boost::multi_array<EigenTypes::ScalarRowVectorComputation, 3>;

		////////////////////////////////////////////////////////////////////////////////
		/** Structure holding a PSF's ENZ corresponding to a certain wavelength and object distance. */
		struct EnzTermsEntry
		{
			glm::ivec2 m_degrees; // Degrees of the corresponding coefficient
			EigenTypes::ComplexRowVectorComputation m_vnms; // The Extended Nijboer-Zernike Vnm-s
		};

		////////////////////////////////////////////////////////////////////////////////
		/** The ENZ terms, for each zernike coefficient. They are organized as follows:
		*     [0] Defocus parameter;
		*     [1] Zernike coefficients
		*/
		using EnzTerms = boost::multi_array<EnzTermsEntry, 2>;

		////////////////////////////////////////////////////////////////////////////////
		/** Structure holding a PSF corresponding to a certain wavelength and object distance. */
		struct PsfEntry
		{
			int m_kernelSizePx; // Size of the kernel, in pixels
			float m_blurRadiusMuM; // Size of the PSF on the retina (in micrometers).
			float m_blurRadiusDeg; // Size of the PSF on the retina (in degrees).
			float m_blurSizeMuM; // Size of the PSF on the retina (in micrometers).
			float m_blurSizeDeg; // Size of the PSF on the retina (in degrees).
			Psf m_psf; // The PSF itself.
		};

		////////////////////////////////////////////////////////////////////////////////
		/** A set of PSFs. Follows the same organization as the entry parameters. They are organized as follows:
		*     [0] Object depth;
		*     [1] Horizontal axis;
		*     [2] Vertical axis;
		*     [3] Wavelength;
		*     [4] Aperture diameter;
		*     [5] Focus distance;
		*/
		using PsfEntries = boost::multi_array<PsfEntry, 6>;

		////////////////////////////////////////////////////////////////////////////////
		/** Structure holding common debug information */
		struct DebugInformationCommon
		{
			std::unordered_map<std::string, float> m_eyeParameters; // List of eye parameters corresponding to the input
			ZernikeCoefficientsAlpha m_alphaTrueNN; // Aberration coefficients generated by the reconstructed model (computed via neural network)
			ZernikeCoefficientsAlpha m_alphaTrueRT; // Aberration coefficients generated by the reconstructed model (computed via ray-tracing)
			PSFStackParameters::ComputationBackend m_backend = PSFStackParameters::ComputationBackend::CPU; // Backend with which the PSFs were last computed
			int m_lastComputedFrameId = -1; // The frame in which the stack was computed.
		};
	}
	
	////////////////////////////////////////////////////////////////////////////////
	/** A "PSF stack" - a collection of PSFs with all the necessary data. */
	struct PSFStack
	{
		/** The various aberration coefficients. */
		PsfStackElements::AberrationCoefficients m_aberrationCoefficients;

		/** Parameters of the relaxed eye. */
		PsfStackElements::RelaxedEyeParameters m_relaxedEyeParameters;

		/** Parameters of the focused eye states. */
		PsfStackElements::FocusedEyeParameters m_focusedEyeParameters;

		/** ENZ entry sampling parameters. */
		PsfStackElements::EnzEntrySamplingParameters m_enzEntrySamplingParameters;

		/** The ENZ Vnm coefficients (wkl). */
		PsfStackElements::EnzCoefficients m_enzWklCoefficients;

		/** Accel structure for the spherical Bessel values used in the ENZ terms. */
		PsfStackElements::EnzSphericalBessels m_enzSphericalBessel;

		/** Accel structure for the cylindrical Bessel values used in the ENZ terms. */
		PsfStackElements::EnzCylindricalBessels m_enzCylindricalBessel;

		/** Cache structure for the inner Vnm terms (sum of wkl * jk over l). */
		PsfStackElements::EnzVnmInnerTerms m_enzVnmInnerTerms;

		/** The various PSF parameters for each entry. */
		PsfStackElements::PsfEntryParameters m_psfEntryParameters;

		/** The actual PSFs. */
		PsfStackElements::PsfEntries m_psfs;

		/** Common debug information. */
		PsfStackElements::DebugInformationCommon m_debugInformationCommon;

		/** Computation timer-set. */
		DateTime::TimerSet m_timers;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Structure describing a set of wavefront aberrations. */
	struct WavefrontAberration
	{
		std::string m_name = "Unknown Aberration"; // Name of the aberration
		std::string m_shortName = "NA"; // Name of the aberration
		float m_refractiveIndex = 1.337f; // Refractive index of the optical system
		AberrationParameters m_aberrationParameters; // Measurement parameters.
		EyeReconstructionParameters m_reconstructionParameters; // Eye reconstruction parameters
		PSFStackParameters m_psfParameters; // Psf evaluation parameters
		PSFStack m_psfStack; // The corresponding PSF stack
		PSFStackPreviewParameters m_psfPreview; // Preview parameters
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Structure describing a set of named aberration presets. */
	using WavefrontAberrationPresets = std::unordered_map<std::string, Aberration::WavefrontAberration>;

	////////////////////////////////////////////////////////////////////////////////
	WavefrontAberrationPresets loadAberrationPresets(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void initAberrationStructure(Scene::Scene& scene, WavefrontAberration& aberration, WavefrontAberrationPresets& presets);

	////////////////////////////////////////////////////////////////////////////////
	void loadNeuralNetworks(Scene::Scene& scene, WavefrontAberration& aberration);

	////////////////////////////////////////////////////////////////////////////////
	void loadShaders(Scene::Scene& scene, WavefrontAberration& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateObjectDistances(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateObjectDioptres(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateHorizontalAxes(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateVerticalAxes(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateApertureDiameters(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateFocusDioptres(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateFocusDistances(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumObjectDistances(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumHorizontalAngles(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumVerticalAngles(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumLambdas(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumApertures(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumFocuses(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumPsfsTotal(Scene::Scene& scene, WavefrontAberration const& aberration);

	////////////////////////////////////////////////////////////////////////////////
	float blurRadiusAngle(const float blurSizeMuM);

	////////////////////////////////////////////////////////////////////////////////
	float blurRadiusPixels(const float blurRadiusDeg, const glm::ivec2 renderResolution, const float fovy);

	////////////////////////////////////////////////////////////////////////////////
	float blurRadiusPixels(Aberration::PsfStackElements::PsfEntry const& psf, const glm::ivec2 renderResolution, const float fovy);

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsf(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, size_t radius);

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsfNormalized(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, size_t radius);

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsf(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, float radius);

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsfNormalized(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, float radius);

	////////////////////////////////////////////////////////////////////////////////
	Psf getProjectedPsf(Scene::Scene& scene, WavefrontAberration& aberration, Aberration::PsfStackElements::PsfEntry const& psf, const glm::ivec2 renderResolution, const float fovy);

	////////////////////////////////////////////////////////////////////////////////
	Psf getProjectedPsfNormalized(Scene::Scene& scene, WavefrontAberration& aberration, Aberration::PsfStackElements::PsfEntry const& psf, const glm::ivec2 renderResolution, const float fovy);

	////////////////////////////////////////////////////////////////////////////////
	using PsfStackComputation = size_t;
	enum PsfStackComputation_ : size_t
	{
		// Eye parameters
		PsfStackComputation_RelaxedEyeParameters = std::bit_mask(8),
		PsfStackComputation_FocusedEyeParameters = std::bit_mask(9),
		PsfStackComputation_AberrationCoefficients = std::bit_mask(10),
		PsfStackComputation_EyeParameters = PsfStackComputation_RelaxedEyeParameters | 
			PsfStackComputation_FocusedEyeParameters | PsfStackComputation_AberrationCoefficients,

		// PSF parameters
		PsfStackComputation_PsfUnits = std::bit_mask(16),
		PsfStackComputation_PsfEnzCoefficients = std::bit_mask(17),
		PsfStackComputation_PsfBesselTerms = std::bit_mask(18),
		PsfStackComputation_PsfParameters = PsfStackComputation_PsfUnits | PsfStackComputation_PsfEnzCoefficients | 
			PsfStackComputation_PsfBesselTerms,

		// PSFs
		PsfStackComputation_Psfs = std::bit_mask(24),

		// Everything
		PsfStackComputation_Everything = PsfStackComputation_EyeParameters | 
			PsfStackComputation_PsfParameters | 
			PsfStackComputation_Psfs
	};

	////////////////////////////////////////////////////////////////////////////////
	void computePSFStack(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation);

	////////////////////////////////////////////////////////////////////////////////
	void freeCacheResources(Scene::Scene& scene, WavefrontAberration& aberration);

	////////////////////////////////////////////////////////////////////////////////
	std::array<bool, 2> generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, 
		WavefrontAberration& aberration, std::unordered_map<std::string, WavefrontAberration>& presets);

	////////////////////////////////////////////////////////////////////////////////
	PsfIndex getPsfIndex(const PsfStackElements::PsfEntries::size_type* shape, const PsfStackElements::PsfEntries::index* strides, const size_t psfIndex);

	////////////////////////////////////////////////////////////////////////////////
	PsfIndex getPsfIndex(Scene::Scene& scene, WavefrontAberration& aberration, const size_t psfIndex);

	////////////////////////////////////////////////////////////////////////////////
	PsfStackElements::PsfEntryParams& getPsfEntryParameters(Scene::Scene& scene, WavefrontAberration& aberration, PsfIndex const& psfIndex);

	////////////////////////////////////////////////////////////////////////////////
	PsfStackElements::PsfEntry& getPsfEntry(Scene::Scene& scene, WavefrontAberration& aberration, PsfIndex const& psfIndex);

	////////////////////////////////////////////////////////////////////////////////
	struct PsfIndexIterator
	{
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = PsfIndex;
		using pointer = PsfIndex*;
		using reference = PsfIndex&;

		using size_ptr_type = const PsfStackElements::PsfEntries::size_type*;
		using index_ptr_type = const PsfStackElements::PsfEntries::index*;

		PsfIndexIterator(size_ptr_type shape, index_ptr_type strides, const size_t id = 0) :
			m_shape(shape), m_strides(strides), m_id(id) {}

		PsfIndex operator*() const { return getPsfIndex(m_shape, m_strides, m_id); }

		PsfIndexIterator& operator++() { ++m_id; return *this; }
		PsfIndexIterator operator++(int) { PsfIndexIterator tmp = *this; ++m_id; return tmp; }

		friend bool operator== (PsfIndexIterator const& a, PsfIndexIterator const& b) { return a.m_id == b.m_id; };
		friend bool operator!= (PsfIndexIterator const& a, PsfIndexIterator const& b) { return a.m_id != b.m_id; };

	private:
		size_ptr_type m_shape;
		index_ptr_type m_strides;
		size_t m_id;
	};

	////////////////////////////////////////////////////////////////////////////////
	PsfIndexIterator psfStackBegin(Scene::Scene& scene, WavefrontAberration& aberration);

	////////////////////////////////////////////////////////////////////////////////
	PsfIndexIterator psfStackEnd(Scene::Scene& scene, WavefrontAberration& aberration);

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn, typename Fp>
	void forEachPsfIndex(Scene::Scene& scene, WavefrontAberration& aberration, Fp const& filterPred, Fn const& fn)
	{
		// Traverse the PSF indices
		for (size_t defocusId = 0; defocusId < getNumObjectDistances(scene, aberration); ++defocusId)
		for (size_t horizontalId = 0; horizontalId < getNumHorizontalAngles(scene, aberration); ++horizontalId)
		for (size_t verticalId = 0; verticalId < getNumVerticalAngles(scene, aberration); ++verticalId)
		for (size_t lambdaId = 0; lambdaId < getNumLambdas(scene, aberration); ++lambdaId)
		for (size_t apertureId = 0; apertureId < getNumApertures(scene, aberration); ++apertureId)
		for (size_t focusId = 0; focusId < getNumFocuses(scene, aberration); ++focusId)
		{
			PsfIndex const& index = PsfIndex{ defocusId, horizontalId, verticalId, lambdaId, apertureId, focusId };
			if (filterPred(index)) fn(scene, aberration, index);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	void forEachPsfIndex(Scene::Scene& scene, WavefrontAberration& aberration, Fn const& fn)
	{
		forEachPsfIndex(scene, aberration, [](auto const& index) { return true; }, fn);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn, typename Fp>
	void forEachPsfStackIndex(Scene::Scene& scene, WavefrontAberration& aberration, Fp const& filterPred, Fn const& fn)
	{
		// Traverse the PSF indices
		for (size_t defocusId = 0; defocusId < aberration.m_psfStack.m_psfEntryParameters.size(); ++defocusId)
		for (size_t horizontalId = 0; horizontalId < aberration.m_psfStack.m_psfEntryParameters[0].size(); ++horizontalId)
		for (size_t verticalId = 0; verticalId < aberration.m_psfStack.m_psfEntryParameters[0][0].size(); ++verticalId)
		for (size_t lambdaId = 0; lambdaId < aberration.m_psfStack.m_psfEntryParameters[0][0][0].size(); ++lambdaId)
		for (size_t apertureId = 0; apertureId < aberration.m_psfStack.m_psfEntryParameters[0][0][0][0].size(); ++apertureId)
		for (size_t focusId = 0; focusId < aberration.m_psfStack.m_psfEntryParameters[0][0][0][0][0].size(); ++focusId)
		{
			PsfIndex const& index = PsfIndex{ defocusId, horizontalId, verticalId, lambdaId, apertureId, focusId };
			if (filterPred(index)) fn(scene, aberration, index);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	void forEachPsfStackIndex(Scene::Scene& scene, WavefrontAberration& aberration, Fn const& fn)
	{
		forEachPsfStackIndex(scene, aberration, [](auto const& index) { return true; }, fn);
	}
}