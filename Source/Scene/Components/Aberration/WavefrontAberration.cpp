#include "PCH.h"
#include "WavefrontAberration.h"

// TODO: make a custom object for previewing aberrations, instead of 'PSF' inside the preview settings

namespace Aberration
{
	////////////////////////////////////////////////////////////////////////////////
	// Names of the various neural networks
	static const std::string s_eyeEstimator = "eye_reconstruction.EyeEstimator";
	static const std::string s_refocusEstimator = "eye_refocusing.RefocusEstimator";
	static const std::string s_aberrationEstimatorOnAxis = "eye_reconstruction.AberrationEstimator";
	static const std::string s_aberrationEstimatorOffAxis = "eye_aberrations.AberrationEstimator";
	static const std::vector<std::string> s_estimators = { s_eyeEstimator, s_refocusEstimator, s_aberrationEstimatorOnAxis, s_aberrationEstimatorOffAxis };

	////////////////////////////////////////////////////////////////////////////////
	// Various shader-related parameters
	static const GLenum s_vnmTextureFormat = GL_RG32F;
	static const std::string s_vnmTextureFormatStr = "rg32f";

	////////////////////////////////////////////////////////////////////////////////
	namespace ZernikeIndices
	{
		////////////////////////////////////////////////////////////////////////////////
		/* This function converts single to double index in Zernike polynomials. */
		std::pair<int, int> single2doubleAnsi(int jj)
		{
			const int n = glm::floor(glm::sqrt(2 * jj + 1) + 0.5) - 1;
			const int m = 2 * jj - n * (n + 2);

			return std::make_pair(n, m);
		}

		////////////////////////////////////////////////////////////////////////////////
		/* This function converts single to double index in Zernike polynomials. */
		std::pair<int, int> single2doubleNoll(int jj)
		{
			int n = 0;
			int j1 = jj - 1;
			while (j1 > n)
			{
				n += 1;
				j1 -= n;
			}

			const int m = std::pow(-1, jj) * ((n % 2) + 2 * int((j1 + ((n + 1) % 2)) / 2.0));
			return std::make_pair(n, m);
		}

		////////////////////////////////////////////////////////////////////////////////
		/* This function converts double to single index in Zernike polynomials. */
		int double2singleAnsi(int n, int m)
		{
			return (n * n + 2 * n + m) / 2;
		}
		int double2singleAnsi(glm::ivec2 nm)
		{
			return double2singleAnsi(nm.x, nm.y);
		}

		////////////////////////////////////////////////////////////////////////////////
		/* This function converts double to single index in Zernike polynomials. */
		int double2singleNoll(int n, int m)
		{
			for (int jj = 1; ; ++jj)
			{
				auto [nn, mm] = single2doubleNoll(jj);
				if (n == nn && m == mm) return jj;
			}
			return -1;
		}
		int double2singleNoll(glm::ivec2 nm)
		{
			return double2singleNoll(nm.x, nm.y);
		}

		////////////////////////////////////////////////////////////////////////////////
		int ansi2noll(int jj)
		{
			const auto [n, m] = single2doubleAnsi(jj);
			return double2singleNoll(n, m);
		}

		////////////////////////////////////////////////////////////////////////////////
		int noll2ansi(int jj)
		{
			const auto [n, m] = single2doubleNoll(jj);
			return double2singleAnsi(n, m);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Names of all various zernike polynomials. */
		struct ZernikeName { int m_noll; int m_ansi; glm::ivec2 m_degree; std::string m_name; std::string m_description; };
		const std::vector<ZernikeName> s_zernikeNames =
		{
			// noll | ansi | degree | name | description
			{  0,  0, glm::ivec2(-1), "INVALID", "INVALID" },
			{  1,  0, glm::ivec2(0,  0), "Z1", "Piston" },
			{  2,  2, glm::ivec2(1,  1), "Z3", "x Tilt" },
			{  3,  1, glm::ivec2(1, -1), "Z2", "y Tilt" },
			{  4,  4, glm::ivec2(2,  0), "Z5", "Defocus" },
			{  5,  3, glm::ivec2(2, -2), "Z4", "Primary Oblique Astigmatism" },
			{  6,  5, glm::ivec2(2,  2), "Z6", "Primary Vertical Astigmatism" },
			{  7,  7, glm::ivec2(3, -1), "Z8", "y Coma" },
			{  8,  8, glm::ivec2(3,  1), "Z9", "x Coma" },
			{  9,  6, glm::ivec2(3, -3), "Z7", "y Trefoil" },
			{ 10,  9, glm::ivec2(3,  3), "Z10", "x Trefoil" },
			{ 11, 12, glm::ivec2(4,  0), "Z13", "Primary Spherical" },
			{ 12, 13, glm::ivec2(4,  2), "Z14", "Secondary Verical Astigmatism" },
			{ 13, 11, glm::ivec2(4, -2), "Z12", "Secondary Oblique Astigmatism" },
			{ 14, 14, glm::ivec2(4,  4), "Z15", "x Quadrafoil" },
			{ 15, 10, glm::ivec2(4, -4), "Z11", "y Quadrafoil" },
			{ 16, 18, glm::ivec2(5,  1), "Z19", "Secondary x Coma" },
			{ 17, 17, glm::ivec2(5, -1), "Z18", "Secondary y Coma" },
			{ 18, 19, glm::ivec2(5,  3), "Z20", "Secondary x Trefoil" },
			{ 19, 16, glm::ivec2(5, -3), "Z17", "Secondary y Trefoil" },
			{ 20, 20, glm::ivec2(5,  5), "Z21", "Pentafoil x" },
			{ 21, 15, glm::ivec2(5, -5), "Z16", "Pentafoil y" },
			{ 22, 24, glm::ivec2(6,  0), "Z25", "Secondary Spherical" },
			{ 23, 23, glm::ivec2(6, -2), "Z24", "Tertiary Oblique Astigmatism" },
			{ 24, 25, glm::ivec2(6,  2), "Z26", "Tertiary Vertical Astigmatism" },
			{ 25, 22, glm::ivec2(6, -4), "Z23", "Secondary y Quadrafoil" },
			{ 26, 26, glm::ivec2(6,  4), "Z27", "Secondary x Quadrafoil" },
			{ 27, 21, glm::ivec2(6, -6), "Z22", "y Hexafoil" },
			{ 28, 27, glm::ivec2(6,  6), "Z28", "x Hexafoil" },
			{ 29, 31, glm::ivec2(7, -1), "Z32", "Tertiary y Coma" },
			{ 30, 32, glm::ivec2(7,  1), "Z33", "Tertiary x Coma" },
			{ 31, 30, glm::ivec2(7, -3), "Z31", "Tertiary y Trefoil" },
			{ 32, 33, glm::ivec2(7,  3), "Z34", "Tertiary x Trefoil" },
			{ 33, 29, glm::ivec2(7, -5), "Z30", "Secondary y Pentafoil" },
			{ 34, 34, glm::ivec2(7,  5), "Z35", "Secondary x Pentafoil" },
			{ 35, 28, glm::ivec2(7, -7), "Z29", "y Heptafoil" },
			{ 36, 35, glm::ivec2(7,  7), "Z36", "x Heptafoil" },
			{ 37, 40, glm::ivec2(8,  0), "Z41", "Quadrary Spherical" },
			{ 38, 41, glm::ivec2(8,  2), "Z42", "Quadrary Vertical Astigmatism" },
			{ 39, 39, glm::ivec2(8, -2), "Z40", "Quadrary Oblique Astigmatism" },
			{ 40, 42, glm::ivec2(8,  4), "Z43", "Tertiary Quadrafoil x" },
			{ 41, 38, glm::ivec2(8, -4), "Z39", "Tertiary Quadrafoil y" },
			{ 42, 43, glm::ivec2(8,  6), "Z44", "Secondary Hexafoil x" },
			{ 43, 37, glm::ivec2(8, -6), "Z38", "Secondary Hexafoil y" },
			{ 44, 44, glm::ivec2(8,  8), "Z45", "Octafoil x" },
			{ 45, 36, glm::ivec2(8, -8), "Z37", "Octafoil y" },
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	struct ZernikePositiveCoeff
	{
		glm::ivec2 m_degrees[2];
		int m_ansi[2];
		int m_noll[2];
		ZernikePositiveCoeff(const int n, const int m):
			m_degrees{ glm::ivec2(n, m), glm::ivec2(n, -m) },
			m_ansi{ ZernikeIndices::double2singleAnsi(m_degrees[0]), ZernikeIndices::double2singleAnsi(m_degrees[1]) },
			m_noll{ ZernikeIndices::double2singleNoll(m_degrees[0]), ZernikeIndices::double2singleNoll(m_degrees[1]) }
		{}
	};

	////////////////////////////////////////////////////////////////////////////////
	struct ZernikeDegreesPositiveOnly
	{
		const int m_maxDegree;
		std::vector<ZernikePositiveCoeff> m_coeffs;

		////////////////////////////////////////////////////////////////////////////////
		ZernikeDegreesPositiveOnly(const int maxDegrees) :
			m_maxDegree(maxDegrees)
		{
			for (int n = 0; n <= m_maxDegree; ++n)
			for (int m = n % 2; m <= n; m += 2)
			{
				m_coeffs.push_back(ZernikePositiveCoeff(n, m));
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikePositiveCoeff const& coeff(const int coeffId) const
		{
			return m_coeffs[coeffId];
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	static ZernikeDegreesPositiveOnly s_positiveDegrees = ZernikeDegreesPositiveOnly(100);

	////////////////////////////////////////////////////////////////////////////////
	ZernikePositiveCoeff zernikeCoeffPositiveOnly(const int coeffId)
	{
		return s_positiveDegrees.coeff(coeffId);
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace AberrationPreset
	{
		////////////////////////////////////////////////////////////////////////////////
		static const std::string s_presetExtension = ".abp";
		static const std::string s_presetsFolder = "Aberrations";

		////////////////////////////////////////////////////////////////////////////////
		std::filesystem::path presetsFolder()
		{
			return EnginePaths::assetsFolder() / s_presetsFolder;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string fullPresetPath(std::string const& presetName)
		{
			return s_presetsFolder + "/" + presetName + s_presetExtension;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string extractPresetName(std::filesystem::path filepath)
		{
			return filepath.relative_path().stem().string();
		}

		////////////////////////////////////////////////////////////////////////////////
		bool save(Scene::Scene& scene, const std::string& presetName, Aberration::WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, presetName);

			Debug::log_trace() << "Saving aberration preset: " << presetName << Debug::end;

			// Full file name for the preset file
			std::string fullFileName = fullPresetPath(presetName);

			// Generate the resulting file contents
			std::stringstream fileContents;
			fileContents << "Ap" << " = " << aberration.m_aberrationParameters.m_apertureDiameter << std::endl;
			fileContents << "Wa" << " = " << aberration.m_aberrationParameters.m_lambda << std::endl;
			for (size_t i = 2; i < aberration.m_aberrationParameters.m_coefficients.size(); ++i)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(i);
				const float alpha = aberration.m_aberrationParameters.m_coefficients[i];
				if (alpha != 0.0f) fileContents << "Z[" << n << ", " << m << "] = " << alpha << std::endl;
			}

			// Save the generated contents
			return Asset::saveTextFile(scene, fullFileName, fileContents);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::optional<Aberration::WavefrontAberration> load(Scene::Scene& scene, std::string const& presetName)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, presetName);

			Debug::log_trace() << "Loading aberration preset: " << presetName << Debug::end;

			// Full file name for the preset file
			std::string fullFileName = fullPresetPath(presetName);

			// Load the preset
			std::optional<std::string> contents = Asset::loadTextFile(scene, fullFileName);

			if (contents.has_value() == false)
			{
				Debug::log_error() << "Unable to load aberration preset: " << presetName << Debug::end;
				return std::nullopt;
			}

			// Resulting aberration
			Aberration::WavefrontAberration result;

			// Store its name
			result.m_name = presetName;
			result.m_aberrationParameters.m_type = AberrationParameters::Preset;

			// Regular expression for a coefficient
			std::string const& floatRegex = "([\\+\\-]?[0-9]+\\.[0-9]+)";
			std::regex coefficientRegex("Z\\[[[:space:]]*([0-9]+),[[:space:]]*([\\-]?)[[:space:]]*([0-9]+)\\][[:space:]]*[\\=\\:]+[[:space:]]*([\\+\\-]?[0-9]+\\.[0-9]+)");
			std::regex apertureRegex("Ap[[:space:]]*[\\=\\:]+[[:space:]]*" + floatRegex);
			std::regex lambdaRegex("Wa[[:space:]]*[\\=\\:]+[[:space:]]*" + floatRegex);
			std::regex nameRegex("Name[[:space:]]*[\\=\\:]+[[:space:]]*" + floatRegex);

			// Process the data
			std::stringstream lines(*contents);

			for (std::string line; std::getline(lines, line);)
			{
				// Try to match a coefficient
				std::smatch capture;
				if (std::regex_search(line, capture, coefficientRegex))
				{
					// Extract the index and value
					const int n = std::stoi(capture[1]);
					const int m = std::stoi(capture[2].str() + capture[3].str());
					const float value = std::stof(capture[4]);
					const int index = ZernikeIndices::double2singleNoll(n, m);

					// Store the values
					result.m_aberrationParameters.m_coefficients[index] = value;
				}

				// try to match the aperture diameter
				else if (std::regex_search(line, capture, apertureRegex))
				{
					// Extract the index and value
					const float value = std::stof(capture[1]);

					// Store the value
					result.m_aberrationParameters.m_apertureDiameter = value;
				}

				// try to match the measurement lambda
				else if (std::regex_search(line, capture, lambdaRegex))
				{
					// Extract the index and value
					const float value = std::stof(capture[1]);

					// Store the value
					result.m_aberrationParameters.m_lambda = value;
				}

				// try to match the aberration name
				else if (std::regex_search(line, capture, nameRegex))
				{
					result.m_shortName = capture[1];
				}
			}

			Debug::log_trace() << "Successfully loaded aberration preset: " << presetName << Debug::end;

			return result;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	WavefrontAberrationPresets loadAberrationPresets(Scene::Scene& scene)
	{
		WavefrontAberrationPresets result;
		for (auto const& file : std::filesystem::directory_iterator(AberrationPreset::presetsFolder()))
		{
			if (file.path().extension() == AberrationPreset::s_presetExtension)
			{
				std::string const& presetName = AberrationPreset::extractPresetName(file.path());
				if (result.find(presetName) == result.end())
					if (auto const& aberration = AberrationPreset::load(scene, presetName); aberration.has_value())
						result[presetName] = aberration.value();
			}
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initAberrationStructure(Scene::Scene& scene, WavefrontAberration& aberration, WavefrontAberrationPresets& presets)
	{
		presets = loadAberrationPresets(scene);
		if (aberration.m_aberrationParameters.m_type == Aberration::AberrationParameters::Preset)
		{
			if (presets.find(aberration.m_name) != presets.end())
				aberration.m_aberrationParameters = presets[aberration.m_name].m_aberrationParameters;
			else
				Debug::log_error() << "Unable to find requested aberration preset: " << aberration.m_name << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadNeuralNetworks(Scene::Scene& scene, WavefrontAberration& aberration)
	{
		std::for_each(s_estimators.begin(), s_estimators.end(), 
			[&](std::string const& estimator) { Asset::loadTfModel(scene, estimator); });
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadShaders(Scene::Scene& scene, WavefrontAberration& aberration)
	{
		// Vnm inner cache computation shader
		Asset::loadShader(scene, "Aberration/PsfStack", "compute_vnm_inner", "PsfStack/compute_vnm_inner");

		// Vnm computation shader, with no inner term caching
		Asset::ShaderParameters shaderParametersNoInner;
		shaderParametersNoInner.m_defines =
		{
			"VNM_TEXTURE_FORMAT " + s_vnmTextureFormatStr,
			"USE_CACHED_INNER_TERM 0"
		};
		Asset::loadShader(scene, "Aberration/PsfStack", "compute_vnm", "PsfStack/compute_vnm_no_cache", shaderParametersNoInner);

		// Vnm computation shader, using inner term caching
		Asset::ShaderParameters shaderParametersWithInner;
		shaderParametersWithInner.m_defines =
		{
			"VNM_TEXTURE_FORMAT " + s_vnmTextureFormatStr,
			"USE_CACHED_INNER_TERM 1"
		};
		Asset::loadShader(scene, "Aberration/PsfStack", "compute_vnm", "PsfStack/compute_vnm_with_cache", shaderParametersWithInner);

		// Psf computation shader
		Asset::loadShader(scene, "Aberration/PsfStack", "compute_psf", "PsfStack/compute_psf");
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Coefficients
	{
		////////////////////////////////////////////////////////////////////////////////
		float opdToPhase(ScalarZernikeCoeffs opd, float lambda, bool cumulative = true)
		{
			//const float i = cumulative ? (opd / lambda) : opd < 0.0f ? (1.0f - glm::fract((opd / lambda))) : glm::fract((opd / lambda));
			//const float i = cumulative ? (opd / lambda) : opd < 0.0f ? (1.0f - glm::fract(glm::abs(opd / lambda))) : glm::fract((opd / lambda));
			const float i = cumulative ? (opd / lambda) : (opd < 0.0f ? opd / lambda - glm::ceil(opd / lambda) : opd / lambda - glm::floor(opd / lambda));
			return i * glm::two_pi<float>();
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha opdToPhase(WavefrontAberration const& aberration, ZernikeCoefficientsAlpha const& source, float lambda, bool cumulative = true)
		{
			ZernikeCoefficientsAlpha result;
			std::transform(source.begin() + 1, source.end(), result.begin() + 1,
				[&](ScalarZernikeCoeffs coeff) { return opdToPhase(coeff, lambda, cumulative); });
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha const& selectCoefficients(PsfStackElements::AberrationCoefficientVariants const& coefficients,
			Aberration::PSFStackParameters::CoefficientVariation variation)
		{
			switch (variation)
			{
				case PSFStackParameters::CoefficientVariation::AlphaOpd: return coefficients.m_alpha;
				case PSFStackParameters::CoefficientVariation::AlphaPhaseCumulative: return coefficients.m_alphaPhaseCumulative;
				case PSFStackParameters::CoefficientVariation::AlphaPhaseResidual: return coefficients.m_alphaPhaseResidual;
			}

			// Default to OPD
			return coefficients.m_alpha;
		}

		////////////////////////////////////////////////////////////////////////////////
		namespace AlphaToBeta
		{
			////////////////////////////////////////////////////////////////////////////////
			void makeRhotabRow(Eigen::MatrixXd& rhotab, const int max_deg, const int c, const int n, const int m)
			{
				for (int s = 0; s <= (n - m) / 2; ++s)
				{
					const double a = double(glm::pow(-1.0f, s) * gsl_sf_fact(n - s));
					const double b = double(gsl_sf_fact(s) * gsl_sf_fact((n + m) / 2 - s) * gsl_sf_fact((n - m) / 2 - s));
					rhotab(c, (n - 2 * s)) = a / b;
				}
			}

			////////////////////////////////////////////////////////////////////////////////
			// Computes the coefficients of R_n^m
			Eigen::MatrixXd makeRhotab(const int numCoeffs, const int max_deg)
			{
				Eigen::MatrixXd rhotab = Eigen::MatrixXd::Zero(numCoeffs, max_deg + 1);
				int count = 1;
				for (int ni = 1; ni <= max_deg; ++ni)
				{
					int mi = ni % 2;
					while (mi <= ni)
					{
						if (mi == 0)
						{
							makeRhotabRow(rhotab, max_deg, count, ni, mi);
							count += 1;
						}
						else
						{
							makeRhotabRow(rhotab, max_deg, count + 0, ni, mi);
							makeRhotabRow(rhotab, max_deg, count + 1, ni, mi);
							count += 2;
						}
						mi += 2;
					}
				}
				rhotab(0, 0) = 1.0f;
				return rhotab;
			}

			////////////////////////////////////////////////////////////////////////////////
			// Computes the coefficients of int[R_n^m * rho]
			Eigen::MatrixXd makeRhoitab(const int numCoeffs, const int max_deg, Eigen::MatrixXd const& rhotab)
			{
				Eigen::MatrixXd rhoitab = Eigen::MatrixXd::Zero(numCoeffs, max_deg + 3);
				for (int ci = 0; ci < numCoeffs; ++ci)
					for (int ni = 0; ni <= max_deg; ++ni)
						rhoitab(ci, ni + 2) = rhotab(ci, ni) / (ni + 2);
				return rhoitab;
			}

			////////////////////////////////////////////////////////////////////////////////
			double polyval(const double x, Eigen::RowVectorXd const& coefficients)
			{
				//return Eigen::poly_eval(coefficients, x);
				double sum = 0.0;
				for (size_t i = 0; i < coefficients.size(); ++i)
					sum += coefficients[i] * glm::pow(x, i);
				return sum;
			}

			////////////////////////////////////////////////////////////////////////////////
			double Rnmrho_I(const int c, Eigen::MatrixXd const& rhoitab, const float r)
			{
				return polyval(r, rhoitab.row(c - 1));
			};

			////////////////////////////////////////////////////////////////////////////////
			double normTermBeta(const int n, const int m)
			{
				return glm::sqrt(double(n + 1.0));
			}

			////////////////////////////////////////////////////////////////////////////////
			double normTermBeta(const int c)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(c);
				return normTermBeta(n, m);
			}

			////////////////////////////////////////////////////////////////////////////////
			double normTermAlpha(const int n, const int m)
			{
				return (m == 0 ? glm::sqrt(double(n + 1.0)) : glm::sqrt(double(2.0 * (n + 1.0))));
			}

			////////////////////////////////////////////////////////////////////////////////
			double normTermAlpha(const int c)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(c);
				return normTermAlpha(n, m);
			}

			////////////////////////////////////////////////////////////////////////////////
			double radialComponentBeta(const int c, const double r, Eigen::MatrixXd const& rhotab)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(c);
				return normTermBeta(n, m) * polyval(r, rhotab.row(c - 1));
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd radialComponentBeta(const int c, Eigen::RowVectorXd const& rho_j, Eigen::MatrixXd const& rhotab)
			{
				return rho_j.unaryExpr([&](const double r) { return radialComponentBeta(c, r, rhotab); });
			}

			////////////////////////////////////////////////////////////////////////////////
			std::complex<double> angularComponentBeta(const int c, const double phi)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(c);
				return std::exp(std::complex<double>(0, double(m) * phi));
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXcd angularComponentBeta(const int c, Eigen::RowVectorXd const& phi_i)
			{
				return phi_i.cast<std::complex<double>>().unaryExpr([=](const std::complex<double> th) { return angularComponentBeta(c, th.real()); });
			}

			////////////////////////////////////////////////////////////////////////////////
			double radialComponentAlpha(const int c, const double r, Eigen::MatrixXd const& rhotab)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(c);
				return normTermAlpha(n, m) * polyval(r, rhotab.row(c - 1));
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd radialComponentAlpha(const int c, Eigen::RowVectorXd const& rho_j, Eigen::MatrixXd const& rhotab)
			{
				return rho_j.unaryExpr([&](const double r) { return radialComponentAlpha(c, r, rhotab); });
			}

			////////////////////////////////////////////////////////////////////////////////
			double angularComponentAlpha(const int c, const double phi)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(c);
				return m >= 0 ? glm::cos(double(m) * phi) : glm::sin(double(-m) * phi);
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd angularComponentAlpha(const int c, Eigen::RowVectorXd const& phi_i)
			{
				return phi_i.unaryExpr([=](const double th) { return angularComponentAlpha(c, th); });
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd linearSampling(const int N, const double min_val, const double max_val, const double offset)
			{
				Eigen::RowVectorXd Ns = Eigen::RowVectorXd::LinSpaced(N, 0.0, N - 1.0);
				return Ns.unaryExpr([=](const double v) 
				{ 
					return min_val + (max_val - min_val) * ((v + offset) / N); 
				});
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd squareRootSampling(const int N, const double min_val, const double max_val, const double offset)
			{
				Eigen::RowVectorXd Ns = Eigen::RowVectorXd::LinSpaced(N, 0.0, N - 1.0);
				return Ns.unaryExpr([=](const double v) 
				{ 
					return min_val + (max_val - min_val) * glm::sqrt((v + offset) / N);
				});
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd cosineSampling(const int N, const double min_val, const double max_val, const double offset)
			{
				Eigen::RowVectorXd Ns = Eigen::RowVectorXd::LinSpaced(N, 0.0, N - 1.0);
				return Ns.unaryExpr([=](const double v) 
				{ 
					return min_val + (max_val - min_val) * glm::cos(glm::pi<double>() * (N - (v + offset)) / (2.0f * N)); 
				});
			}

			////////////////////////////////////////////////////////////////////////////////
			Eigen::RowVectorXd samplePupil(PSFStackParameters::PupilSampling sampling, const int N, const double min_val, const double max_val, const double offset)
			{
				switch (sampling)
				{
				case PSFStackParameters::PupilSampling::LinearSampling: return linearSampling(N, min_val, max_val, offset);
				case PSFStackParameters::PupilSampling::SquareRootSampling: return squareRootSampling(N, min_val, max_val, offset);
				case PSFStackParameters::PupilSampling::CosineSampling: return cosineSampling(N, min_val, max_val, offset);
				}
				return Eigen::RowVectorXd();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<ZernikeCoefficientsBeta> alphaToBeta(std::vector<ZernikeCoefficientsAlpha> const& alpha, const size_t betaDegrees,
			PSFStackParameters::PupilSampling L_sampling, PSFStackParameters::PupilSampling K_sampling, const size_t L, const size_t K)
		{
			// TODO: experiment with other linear solvers from eigen (householder QR, etc.) for more precise fitting

			const size_t alphaDegrees = ZernikeCoefficientsAlpha::MAX_DEGREES;
			const size_t numAlphaCoefficients = numZernikeCoefficients(alphaDegrees);
			const size_t numBetaCoefficients = numZernikeCoefficients(betaDegrees);

			/** Sample rho and phi */
			Eigen::RowVectorXd phi_i = AlphaToBeta::samplePupil(L_sampling, L, 0.0, glm::two_pi<double>(), 0.5);
			Eigen::RowVectorXd phi_a = AlphaToBeta::samplePupil(L_sampling, L, 0.0, glm::two_pi<double>(), 0.0);
			Eigen::RowVectorXd phi_b = AlphaToBeta::samplePupil(L_sampling, L, 0.0, glm::two_pi<double>(), 1.0);
			Eigen::RowVectorXd rho_j = AlphaToBeta::samplePupil(K_sampling, K, 0.0, 1.0, 0.5);
			Eigen::RowVectorXd rho_a = AlphaToBeta::samplePupil(K_sampling, K, 0.0, 1.0, 0.0);
			Eigen::RowVectorXd rho_b = AlphaToBeta::samplePupil(K_sampling, K, 0.0, 1.0, 1.0);

			//Debug::log_debug() << "phi_i: " << phi_i << Debug::end;
			//Debug::log_debug() << "phi_a: " << phi_a << Debug::end;
			//Debug::log_debug() << "phi_b: " << phi_b << Debug::end;
			//Debug::log_debug() << "rho_j: " << rho_j << Debug::end;
			//Debug::log_debug() << "rho_a: " << rho_a << Debug::end;
			//Debug::log_debug() << "rho_b: " << rho_b << Debug::end;

			/** Phi integrands. */
			Eigen::RowVectorXd I_cosm = Eigen::RowVectorXd(betaDegrees * L);
			Eigen::RowVectorXd I_sinm = Eigen::RowVectorXd(betaDegrees * L);
			for (size_t m = 1; m <= betaDegrees; ++m)
			for (size_t thi = 0; thi < L; ++thi)
			{
				const size_t idx = (m - 1) * L + thi;
				I_cosm[idx] = (1.0f / m) * (glm::sin(m * phi_b[thi]) - glm::sin(m * phi_a[thi]));
				I_sinm[idx] = (1.0f / m) * (-glm::cos(m * phi_b[thi]) + glm::cos(m * phi_a[thi]));
			}

			//Debug::log_debug() << "I_cosm: " << I_cosm << Debug::end;
			//Debug::log_debug() << "I_sinm: " << I_sinm << Debug::end;

			/** R_n^m coefficients. */
			const size_t rhotabNumCoeffs = std::max(numAlphaCoefficients, numBetaCoefficients);
			const size_t rhoTabDegrees = std::max(alphaDegrees, betaDegrees);
			Eigen::MatrixXd rhotab = AlphaToBeta::makeRhotab(rhotabNumCoeffs, rhoTabDegrees);
			Eigen::MatrixXd rhoitab = AlphaToBeta::makeRhoitab(rhotabNumCoeffs, rhoTabDegrees, rhotab);

			//Debug::log_debug() << "rhotab: " << rhotab << Debug::end;
			//Debug::log_debug() << "rhoitab: " << rhoitab << Debug::end;

			/** R_n^m integrands. */
			Eigen::RowVectorXd I_Rnmrho = Eigen::RowVectorXd(numBetaCoefficients * K);
			for (size_t c = 0; c < numBetaCoefficients; ++c)
			for (size_t rhi = 0; rhi < K; ++rhi)
			{
				const size_t idx = c * K + rhi;
				I_Rnmrho[idx] = AlphaToBeta::Rnmrho_I(c + 1, rhoitab, rho_b[rhi]) - AlphaToBeta::Rnmrho_I(c + 1, rhoitab, rho_a[rhi]);
			}

			//Debug::log_debug() << "I_Rnmrho: " << I_Rnmrho << Debug::end;

			// Compute the right side of the dot product [K * L x c]
			Eigen::MatrixXcd A = Eigen::MatrixXcd(K * L, numBetaCoefficients);
			for (size_t coeffId = 0; coeffId < numBetaCoefficients; ++coeffId)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(coeffId + 1);

				const size_t offK = K * coeffId;
				const size_t offL = L * (std::abs(m) - 1);
				const double sgn = glm::sign<double>(-m);

				if (m == 0)
				{
					// Normalization term
					const double ck = glm::sqrt(n + 1.0f) * double(2.0f / L);

					// Go through each L and K
					for (size_t k = 0; k < K; ++k)
					for (size_t l = 0; l < L; ++l)
					{
						A(L * k + l, coeffId) = ck * I_Rnmrho[offK + k];
					}
				}
				else
				{
					// Normalization term
					const double ck = glm::sqrt(n + 1.0f) / glm::pi<double>();

					// Go through each L and K
					for (size_t k = 0; k < K; ++k)
					for (size_t l = 0; l < L; ++l)
					{
						A(L * k + l, coeffId) = ck * I_Rnmrho[offK + k] * (I_cosm[offL + l] + 1.0i * sgn * I_sinm[offL + l]);
					}
				}
			}

			//Debug::log_debug() << "A: " << A << Debug::end;

			// Transpose it [c x K * L]
			const Eigen::MatrixXcd At = A.transpose();

			//Debug::log_debug() << "A^T: " << At << Debug::end;

			//Debug::log_debug() << "A^T (sum): (" << At.rowwise().sum() / (K * L) << ")" << Debug::end;

			// Evaluate the real-valued Zernike basis functions [K * L x c]
			Eigen::MatrixXd zz = Eigen::MatrixXd::Zero(K * L, numAlphaCoefficients);
			for (size_t coeffId = 0; coeffId < numAlphaCoefficients; ++coeffId)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(coeffId + 1);

				// Compute the radial and angular components
				const Eigen::RowVectorXd rad = AlphaToBeta::radialComponentAlpha(coeffId + 1, rho_j, rhotab); // [1 x K]
				const Eigen::RowVectorXd ang = AlphaToBeta::angularComponentAlpha(coeffId + 1, phi_i); // [1 x L]

				//Debug::log_debug() << "radial[" << n << "," << m << "]: (" << rad << ")" << Debug::end;
				//Debug::log_debug() << "angular[" << n << "," << m << "]: (" << ang << ")" << Debug::end;

				// Write out their product
				const Eigen::MatrixXd prod = ang.transpose() * rad; // [L x K]
				zz.block(0, coeffId, K * L, 1) = Eigen::Map<const Eigen::MatrixXd>(prod.data(), K * L, 1);

				//Debug::log_debug() << "sum radial[" << n << "," << m << "](" << K << "): " << rad.sum() / K << Debug::end;
				//Debug::log_debug() << "sum angular[" << n << "," << m << "](" << L << "): " << ang.sum() / L << Debug::end;
				//Debug::log_debug() << "sum prod[" << n << "," << m << "](" << K * L << "): " << prod.sum() / (K * L) << Debug::end;
			}

			//Debug::log_debug() << "zz: " << zz << Debug::end;
			//Debug::log_debug() << "zz (sum): " << zz.colwise().sum() / (K * L) << Debug::end;

			// Eigen vector for the phase coefficients [c x 1]
			Eigen::VectorXd phaseCf = Eigen::VectorXd(numAlphaCoefficients);

			// Produce the resulting coefficients
			std::vector<ZernikeCoefficientsBeta> result(alpha.size(), ZernikeCoefficientsBeta(betaDegrees));
			for (size_t i = 0; i < result.size(); ++i)
			{
				// Fill the phase coeff vector
				std::copy_n(alpha[i].begin() + 1, numAlphaCoefficients, phaseCf.data());

				// Compute the reference frame [K * L x 1]
				const Eigen::MatrixXcd P = (zz * phaseCf).cast<std::complex<double>>().unaryExpr(
					[](std::complex<double> coeff) { return std::complex<double>(std::cos(coeff.real()), std::sin(coeff.real())); });

				// Compute the inner product [c x 1]
				Eigen::MatrixXcf::Map(&result[i].data()[1], numBetaCoefficients, 1) = (At * P).cast<ComplexZernikeCoeffs>();
			}

			// Return the result
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<ZernikeCoefficientsBeta> alphaToBeta(WavefrontAberration const& aberration, 
			std::vector<ZernikeCoefficientsAlpha> const& alpha)
		{
			// Convert them to beta
			return alphaToBeta(alpha,
				aberration.m_psfParameters.m_betaDegrees,
				aberration.m_psfParameters.m_alphaToBetaLSampling,
				aberration.m_psfParameters.m_alphaToBetaKSampling,
				aberration.m_psfParameters.m_alphaToBetaL,
				aberration.m_psfParameters.m_alphaToBetaK);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Converts a spectacle lens prescription to zernike coefficients */
		ZernikeCoefficientsAlpha spectacleToZernike(SpectacleLens lens, float apertureDiameterMM)
		{
			float S = lens.m_sphere;
			float C = lens.m_cylinder;
			float A = lens.m_axis;
			float R = apertureDiameterMM / 2.0f;

			float Z4 = -(R * R * (S + C / 2.0f)) / (4.0f * glm::sqrt(3.0f));
			float Z5 = (R * R * C * glm::sin(2.0f * A)) / (4.0f * glm::sqrt(6.0f));
			float Z6 = (R * R * C * glm::cos(2.0f * A)) / (4.0f * glm::sqrt(6.0f));

			return ZernikeCoefficientsAlpha({ 1.0f, 0.0f, 0.0f, Z4, Z5, Z6 });
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Converts a spectacle lens prescription to zernike coefficients */
		ZernikeCoefficientsAlpha spectacleToZernike(WavefrontAberration const& aberration)
		{
			return spectacleToZernike(
				aberration.m_aberrationParameters.m_spectacleLens, 
				aberration.m_aberrationParameters.m_apertureDiameter);
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha getZernikeCoefficients(WavefrontAberration const& aberration)
		{
			switch (aberration.m_aberrationParameters.m_type)
			{
			case AberrationParameters::Spectacle: return spectacleToZernike(aberration);
			case AberrationParameters::Preset:    return aberration.m_aberrationParameters.m_coefficients;
			}
			return aberration.m_aberrationParameters.m_coefficients;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Ranges
	{
		////////////////////////////////////////////////////////////////////////////////
		int numSteps(PSFStackParameters::ParameterRange range)
		{
			return std::max(range.m_numSteps, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		float stepSize(PSFStackParameters::ParameterRange range)
		{
			return (range.m_max - range.m_min) / std::max(numSteps(range) - 1, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<float> generateLinear(PSFStackParameters::ParameterRange range)
		{
			std::vector<float> result(numSteps(range), 0.0f);
			for (size_t i = 0; i < result.size(); ++i)
				result[i] = range.m_min + i * stepSize(range);
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<float> generateDioptres(PSFStackParameters::ParameterRange range)
		{
			std::vector<float> result(numSteps(range), 0.0f);
			for (size_t i = 0; i < result.size(); ++i)
				result[i] = 1.0f / (range.m_min + i * stepSize(range));
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<float> concatRanges(std::vector<float> const& a, std::vector<float> const& b)
		{
			std::vector<float> result(a.size() + b.size());
			std::copy(a.begin(), a.end(), result.begin());
			std::copy(b.begin(), b.end(), result.begin() + a.size());
			return result;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Defocus
	{
		////////////////////////////////////////////////////////////////////////////////
		// Fractional binomial coefficient
		float binom(int a, int b, int k)
		{
			float sum = 1;
			for (int m = 1; m <= k; ++m)
			{
				sum *= float(a - b * m + b) / float(b * m);
			}
			return sum;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Evaluates D_2n^0(alpha)
		float D_2n_0(const float s0, const int n, const int a, const int b)
		{
			float sum = 0.0f;
			for (int k = n; k < 60; ++k)
			{
				sum +=
					glm::pow(s0, 2.0f * k) *
					(glm::pow(-1, k) * float(binom(a, b, k)) * float(gsl_sf_choose(k, n))) /
					float(gsl_sf_choose(n + k + 1, k));
			}
			return ((2.0f * n + 1) / (n + 1.0f)) * sum;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Evaluates \gamma_2n^0
		float Gamma_2n_0(const float d0, const int n)
		{
			return 0.5f * (glm::pow(d0, n - 1) / (2.0f * n - 1.0f) - glm::pow(d0, n + 1) / float(2.0f * n + 3.0f));
		}

		////////////////////////////////////////////////////////////////////////////////
		// Evaluates G_2n^2(alpha)
		float G_2n_2(const float s0, const int n, const int a, const int b)
		{
			float sum = 0.0f;
			for (int k = n; k < 100; ++k)
			{
				sum +=
					glm::pow(s0, 2.0f * k) *
					(glm::pow(-1, k) * float(binom(a, b, k)) * float(gsl_sf_choose(k - 1, n - 1))) /
					float(gsl_sf_choose(n + k + 1, k + 1));
			}
			return -((2.0f * n + 1) / n) * sum;
		};

		////////////////////////////////////////////////////////////////////////////////
		// Evaluates G_2n^2
		float E_2n_2(const float s0, const int n, const float c0, const float gamma_00)
		{
			return -(1.0f / (2.0f * (1.0f - c0))) *
			(
				(1.0f - gamma_00 * (1.0f - c0)) * G_2n_2(s0, n, -1, 4) -
				(2.0f - gamma_00 * (1.0f - c0)) * G_2n_2(s0, n, 1, 4) +
				G_2n_2(s0, n, 3, 4)
			);
		};

		////////////////////////////////////////////////////////////////////////////////
		// Evaluates C_2n^0(alpha)
		float C_2n_0(const float s0, const int n, const float c0, const float gamma_00)
		{
			return (1.0f / (2.0f * (1.0f - c0))) *
			(
				(1.0f - gamma_00 * (1.0f - c0)) * D_2n_0(s0, n, -1, 4) -
				gamma_00 * (1.0f - c0) * D_2n_0(s0, n, 1, 4) -
				D_2n_0(s0, n, 3, 4)
			);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Based on: Janssen, A., van Haver, S., Braat, J., & Dirksen, P. (2007). Strehl ratio and optimum focus of 
		//           high-numerical-aperture beams. Journal Of The European Optical Society - Rapid Publications, 2. 
		//           doi:10.2971/jeos.2007.07008
		// TODO: add support for coefficients with 'm < 0'
		float computeBestFocus(Scene::Scene& scene, WavefrontAberration const& aberration, const float s0, 
			ZernikeCoefficientsAlpha const& coefficients)
		{
			// Certain constants
			const float c0 = glm::sqrt(1.0f - s0 * s0);
			const float d0 = glm::pow((1.0f - c0) / s0, 2.0f);
			const float gamma_00 = (1.0f + 2.0f * c0) / (3.0f * (1.0f + c0));

			// Sum of the C, E and Gamma terms
			float sc = 0.0f, se = 0.0f, sg = 0.0f;

			// Computes the summation / start from 2 as we ignore piston
			for (int i = 2; i < coefficients.size(); ++i)
			{
				// Corresponding zernike degrees
				auto [zn, zm] = ZernikeIndices::single2doubleNoll(i);
				const int n = zn / 2;

				// We are only interested in the a_2n^0 terms
				if (zn % 2 != 0) continue;

				// Zernike coefficient
				const float alpha = coefficients[i];

				// \alpha_2n^0 terms
				if (zm == 0)
				{
					// Evalute the various terms
					const float c_2n_0 = C_2n_0(s0, n, c0, gamma_00); // C_2n^0 
					const float gamma_2n_0 = Gamma_2n_0(d0, n);       // G_2n^0

					// This term is due to the fact that our Zernike polynomials are normalized using 'sqrt(n+1)' for m==0, 
					// while the Strehl ratio minimization paper assumes 'sqrt(2*(n+1))'
					const float scale = glm::sqrt(2.0f * (zn + 1)) / glm::sqrt((zn + 1));

					// Add the terms to the sum
					sc += (scale * alpha * c_2n_0) / (2.0f * (2.0f * n + 1.0f));
					sg += (c_2n_0 * gamma_2n_0) / (2.0f * (2.0f * n + 1.0f));
				}

				// \alpha_2n^2 terms
				if (zm == 2)
				{
					// Evalute the various terms 
					const float e_2n_2 = E_2n_2(s0, n, c0, gamma_00); // E_2n^2

					// Add the terms to the sum
					se += (alpha * e_2n_2) / (2.0f * (2.0f * n + 1.0f));
				}
			}

			// Best focus parameters
			return -(sc - 0.5f * se) / sg;
		}

		////////////////////////////////////////////////////////////////////////////////
		float computeFocalShiftFromAberration(Scene::Scene& scene, WavefrontAberration const& aberration, const float s0, 
			const float focalShiftToDefocus, ZernikeCoefficientsAlpha const& coefficients)
		{
			return computeBestFocus(scene, aberration, s0, coefficients) / focalShiftToDefocus;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateObjectDistances(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateDioptres(aberration.m_psfParameters.m_objectDistances);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateObjectDioptres(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateLinear(aberration.m_psfParameters.m_objectDistances);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateHorizontalAxes(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateLinear(aberration.m_psfParameters.m_incidentAnglesHorizontal);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateVerticalAxes(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateLinear(aberration.m_psfParameters.m_incidentAnglesVertical);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateApertureDiameters(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateLinear(aberration.m_psfParameters.m_apertureDiameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateFocusDioptres(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateLinear(aberration.m_psfParameters.m_focusDistances);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<float> generateFocusDistances(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return Ranges::generateDioptres(aberration.m_psfParameters.m_focusDistances);
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumObjectDistances(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return aberration.m_psfParameters.m_objectDistances.m_numSteps;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumHorizontalAngles(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return aberration.m_psfParameters.m_incidentAnglesHorizontal.m_numSteps;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumVerticalAngles(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return aberration.m_psfParameters.m_incidentAnglesVertical.m_numSteps;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumLambdas(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return aberration.m_psfParameters.m_lambdas.size();
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumApertures(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return aberration.m_psfParameters.m_apertureDiameters.m_numSteps;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumFocuses(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return aberration.m_psfParameters.m_focusDistances.m_numSteps;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t getNumPsfsTotal(Scene::Scene& scene, WavefrontAberration const& aberration)
	{
		return getNumObjectDistances(scene, aberration) *
			getNumHorizontalAngles(scene, aberration) *
			getNumVerticalAngles(scene, aberration) *
			getNumLambdas(scene, aberration) *
			getNumApertures(scene, aberration) *
			getNumFocuses(scene, aberration);
	}

	////////////////////////////////////////////////////////////////////////////////
	float blurRadiusPixels(const float blurRadiusDeg, const glm::ivec2 renderResolution, const float fovy)
	{
		return ((blurRadiusDeg / glm::degrees(fovy)) * renderResolution.y);
	}

	////////////////////////////////////////////////////////////////////////////////
	float blurRadiusPixels(PsfStackElements::PsfEntry const& psf, const glm::ivec2 renderResolution, const float fovy)
	{
		return blurRadiusPixels(psf.m_blurRadiusDeg, renderResolution, fovy);
	}

	////////////////////////////////////////////////////////////////////////////////
	float blurRadiusAngle(const float blurSizeMuM)
	{
		/** Size of one degree on the human eye retina. */
		static const float EYE_DEGREE_MUM = 288;

		// Divide the blur size with the one-degree area 
		return blurSizeMuM / EYE_DEGREE_MUM;
	}

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsf(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, size_t radius)
	{
		static std::unordered_map<PSFStackParameters::InterpolationType, cv::InterpolationFlags> s_interpolationModes =
		{
			{ PSFStackParameters::InterpolationType::Nearest, cv::INTER_NEAREST },
			{ PSFStackParameters::InterpolationType::Bilinear, cv::INTER_LINEAR },
			{ PSFStackParameters::InterpolationType::Cubic, cv::INTER_CUBIC },
			{ PSFStackParameters::InterpolationType::Lanczos, cv::INTER_LANCZOS4 },
			{ PSFStackParameters::InterpolationType::Area, cv::INTER_AREA },
		};

		return Eigen::resize(psf, Eigen::Vector2i(radius * 2 + 1, radius * 2 + 1), s_interpolationModes[aberration.m_psfParameters.m_interpolationType]);
	}

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsfNormalized(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, size_t radius)
	{
		Psf const& psfResized = resizePsf(scene, aberration, psf, radius);
		return psfResized / psfResized.sum();
	}

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsf(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, float radius)
	{
		// Special case for when the output radius is exactly 0
		if (radius < 1e-3f) return resizePsf(scene, aberration, psf, size_t(0));

		// Resize to the two closest radii
		Psf larger = resizePsf(scene, aberration, psf, size_t(glm::ceil(radius)));
		larger = larger / larger.sum();

		Psf smaller = Eigen::pad(resizePsf(scene, aberration, psf, size_t(glm::floor(radius))), larger.rows(), larger.cols());
		smaller = smaller / smaller.sum();

		// Interpolate between the two
		return (1.0f - glm::fract(radius)) * smaller + glm::fract(radius) * larger;
	}

	////////////////////////////////////////////////////////////////////////////////
	Psf resizePsfNormalized(Scene::Scene& scene, WavefrontAberration& aberration, Psf const& psf, float radius)
	{
		Psf const& psfResized = resizePsf(scene, aberration, psf, radius);
		return psfResized / psfResized.sum();	
	}

	////////////////////////////////////////////////////////////////////////////////
	Psf getProjectedPsf(Scene::Scene& scene, WavefrontAberration& aberration, Aberration::PsfStackElements::PsfEntry const& psf, const glm::ivec2 renderResolution, const float fovy)
	{
		return resizePsf(scene, aberration, psf.m_psf, blurRadiusPixels(psf, renderResolution, fovy));
	}

	////////////////////////////////////////////////////////////////////////////////
	Psf getProjectedPsfNormalized(Scene::Scene& scene, WavefrontAberration& aberration, Aberration::PsfStackElements::PsfEntry const& psf, const glm::ivec2 renderResolution, const float fovy)
	{
		Psf const& psfProjected = getProjectedPsf(scene, aberration, psf, renderResolution, fovy);
		return psfProjected / psfProjected.sum();
	}

	////////////////////////////////////////////////////////////////////////////////
	PsfIndex getPsfIndex(const PsfStackElements::PsfEntries::size_type* shape, const PsfStackElements::PsfEntries::index* strides, const size_t psfIndex)
	{
		PsfIndex result;
		for (size_t d = 0; d < 6; ++d)
			result[d] = (psfIndex / strides[d] % shape[d]);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	PsfIndex getPsfIndex(Scene::Scene& scene, WavefrontAberration& aberration, const size_t psfIndex)
	{
		return getPsfIndex(
			aberration.m_psfStack.m_psfs.shape(),
			aberration.m_psfStack.m_psfs.strides(),
			psfIndex);
	}

	////////////////////////////////////////////////////////////////////////////////
	PsfStackElements::PsfEntryParams& getPsfEntryParameters(Scene::Scene& scene, WavefrontAberration& aberration, Aberration::PsfIndex const& psfIndex)
	{
		return aberration.m_psfStack.m_psfEntryParameters(psfIndex);
	}

	////////////////////////////////////////////////////////////////////////////////
	PsfStackElements::PsfEntry& getPsfEntry(Scene::Scene& scene, WavefrontAberration& aberration, Aberration::PsfIndex const& psfIndex)
	{
		return aberration.m_psfStack.m_psfs(psfIndex);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool isIndexNeighbor(const size_t a, const size_t b)
	{
		return (a - b) <= 1 || (b - a) <= 1;
	}

	////////////////////////////////////////////////////////////////////////////////
	PsfIndexIterator psfStackBegin(Scene::Scene& scene, WavefrontAberration& aberration)
	{
		return PsfIndexIterator(
			aberration.m_psfStack.m_psfEntryParameters.shape(),
			aberration.m_psfStack.m_psfEntryParameters.strides(),
			0);
	}

	////////////////////////////////////////////////////////////////////////////////
	PsfIndexIterator psfStackEnd(Scene::Scene& scene, WavefrontAberration& aberration)
	{
		return PsfIndexIterator(
			aberration.m_psfStack.m_psfEntryParameters.shape(),
			aberration.m_psfStack.m_psfEntryParameters.strides(),
			aberration.m_psfStack.m_psfEntryParameters.num_elements());
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace EyeEstimation
	{
		////////////////////////////////////////////////////////////////////////////////
		struct ErrorMetrics
		{
			float m_meanAbs = 0.0f;
			float m_mae = 0.0f;
			float m_mape = 0.0f;
			size_t m_numValues = 0;

			void append(ErrorMetrics other)
			{
				m_meanAbs += other.m_meanAbs;
				m_mae += other.m_mae;
				m_mape += other.m_mape;
				m_numValues += other.m_numValues;
			}

			void append(const float vtrue, const float vact, const float mapeEps = 1e-3f)
			{
				m_meanAbs += glm::abs(vtrue);
				m_mae += glm::abs(vtrue - vact);
				m_mape += 100.0f * (glm::abs(vtrue - vact) / glm::max(glm::abs(vtrue), mapeEps));
				++m_numValues;
			}

			void average()
			{
				m_meanAbs /= float(m_numValues);
				m_mae /= float(m_numValues);
				m_mape /= float(m_numValues);
				m_numValues = 1;
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		std::pair<ErrorMetrics, ErrorMetrics> computeErrorMetricsCoefficients(ZernikeCoefficientsAlpha const& coeffsTrue, ZernikeCoefficientsAlpha const& coeffsActual, float mapeEps = 1e-3f)
		{
			ErrorMetrics result, resultNonZero;
			for (size_t i = 2; i < coeffsTrue.size(); ++i) // Start from 2, as we ignore piston
			{
				const float ctrue = coeffsTrue[i];
				const float cact = coeffsActual[i];
				result.append(ctrue, cact, mapeEps);
				if (cact != 0.0f)
					resultNonZero.append(ctrue, cact, mapeEps);
			}
			result.average();
			resultNonZero.average();

			return { result, resultNonZero };
		}

		////////////////////////////////////////////////////////////////////////////////
		ErrorMetrics computeErrorMetricsCoefficientsStack(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			ErrorMetrics errorMetricsTotal;
			forEachPsfStackIndex(scene, aberration,
				[&](Scene::Scene& scene, WavefrontAberration& aberration, PsfIndex const& psfIndex)
				{
					if (psfIndex[0] == 0) // coeffs do not change with defocus, so only consider the first defocus index
					{
						PsfStackElements::PsfEntryParams& psfParameters = aberration.m_psfStack.m_psfEntryParameters(psfIndex);
						std::pair<ErrorMetrics, ErrorMetrics> errorMetrics = computeErrorMetricsCoefficients(
							psfParameters.m_debug.m_alphaTrue, psfParameters.m_coefficients.m_alpha);
						errorMetricsTotal.append(errorMetrics.first);
					}
				});
			errorMetricsTotal.average();

			return errorMetricsTotal;
		}

		////////////////////////////////////////////////////////////////////////////////
		ErrorMetrics computeErrorMetricsFocusStack(Scene::Scene& scene, WavefrontAberration& aberration, float mapeEps = 1e-3f)
		{
			ErrorMetrics result;
			for (size_t a = 0; a < aberration.m_psfStack.m_focusedEyeParameters.size(); ++a)
			for (size_t f = 0; f < aberration.m_psfStack.m_focusedEyeParameters[0].size(); ++f)
			{
				PsfStackElements::PsfEntryParams& psfParameters = aberration.m_psfStack.m_psfEntryParameters[0][0][0][0][a][f];
				const float ftrue = psfParameters.m_units.m_focusDistanceM;
				const float fact = psfParameters.m_debug.m_focusDistance;
				result.append(ftrue, fact, mapeEps);
			}
			result.average();

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		ErrorMetrics computeErrorMetricsFocusParamsStack(Scene::Scene& scene, WavefrontAberration& aberration, float mapeEps = 1e-3f)
		{
			ErrorMetrics result;
			for (size_t a = 0; a < aberration.m_psfStack.m_focusedEyeParameters.size(); ++a)
			for (size_t f = 0; f < aberration.m_psfStack.m_focusedEyeParameters[0].size(); ++f)
			{
				const float ldtrue = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_debugInformation.m_trueLensD;
				const float ldact = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_eyeParameters["LensD"];
				result.append(ldtrue, ldact, mapeEps);
				const float attrue = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_debugInformation.m_trueAqueousT;
				const float atact = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_eyeParameters["AqueousT"];
				result.append(attrue, atact, mapeEps);
			}
			result.average();

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		ErrorMetrics computeErrorMetricsFocusParamsDeltaStack(Scene::Scene& scene, WavefrontAberration& aberration, float mapeEps = 1e-3f)
		{
			ErrorMetrics result;
			for (size_t a = 0; a < aberration.m_psfStack.m_focusedEyeParameters.size(); ++a)
			for (size_t f = 0; f < aberration.m_psfStack.m_focusedEyeParameters[0].size(); ++f)
			{
				const float ldbase = aberration.m_psfStack.m_relaxedEyeParameters.m_eyeParameters["LensD"];
				const float ldtrue = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_debugInformation.m_trueLensD;
				const float ldact = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_eyeParameters["LensD"];
				result.append(ldtrue - ldbase, ldact - ldbase, mapeEps);
				const float atbase = aberration.m_psfStack.m_relaxedEyeParameters.m_eyeParameters["AqueousT"];
				const float attrue = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_debugInformation.m_trueAqueousT;
				const float atact = aberration.m_psfStack.m_focusedEyeParameters[a][f].m_eyeParameters["AqueousT"];
				result.append(attrue - atbase, atact - atbase, mapeEps);
			}
			result.average();

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		Eigen::RowVectorXd vectorToEigen(std::vector<float>& v)
		{
			return Eigen::Map<Eigen::RowVectorXf>(v.data(), 1, v.size()).cast<double>();
		}

		////////////////////////////////////////////////////////////////////////////////
		Eigen::MatrixXd aberrationsToEigen(Scene::Scene& scene, WavefrontAberration const& aberration)
		{
			// Compute the actual zernike coeffs
			auto coefficients = aberration.m_aberrationParameters.m_coefficients;

			// Fill the matrix with the coeffs
			Eigen::MatrixXd result(coefficients.size() - 2, 2); // -2: ignore piston and the empty alpha[0]
			for (int i = 2; i < coefficients.size(); ++i)
			{
				result(i - 2, 0) = ZernikeIndices::noll2ansi(i) + 1; // +1 as MATLAB indexing starts at 1
				result(i - 2, 1) = coefficients[i];
			}

			// Return the result
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		#ifdef HAS_Matlab
		ZernikeCoefficientsAlpha extractMatlabAberrations(matlab::data::Array const& arr)
		{
			ZernikeCoefficientsAlpha result;
			for (size_t c = 1; c < result.size(); ++c)
			{
				const size_t coefficientId = ZernikeIndices::ansi2noll(c - 1);
				const float coefficient = arr[c - 1];

				result[coefficientId] = coefficient;
			}
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha extractMatlabAberrations(matlab::data::Array const& arr, 
			const size_t h, const size_t v, const size_t l, const size_t a, const size_t f)
		{
			ZernikeCoefficientsAlpha result;
			for (size_t c = 1; c < result.size(); ++c)
			{
				const size_t coefficientId = ZernikeIndices::ansi2noll(c - 1);
				const float coefficient = arr[h][v][l][a][f][c - 1];

				result[coefficientId] = coefficient;
			}
			return result;
		}
		#endif

		////////////////////////////////////////////////////////////////////////////////
		TensorFlow::DataSample getEyeEstimationInputs(Scene::Scene& scene, WavefrontAberration const& aberration)
		{
			// Resulting dataframe
			TensorFlow::DataSample sample;

			// Fill in the measurement parameters
			sample["Lambda"] = aberration.m_aberrationParameters.m_lambda;
			sample["PupilD"] = aberration.m_aberrationParameters.m_apertureDiameter;

			// Fill in the coefficients
			auto const& coefficients = aberration.m_aberrationParameters.m_coefficients;
			for (int i = 2; i < coefficients.size(); ++i)
			{
				const std::string name = "Aberration" + ZernikeIndices::s_zernikeNames[i].m_name;
				sample[name] = coefficients[i];
			}

			// Return the result
			return sample;
		}

		////////////////////////////////////////////////////////////////////////////////
		TensorFlow::DataSample getRefocusEstimationInputs(Scene::Scene& scene, WavefrontAberration const& aberration,
			TensorFlow::DataSample const& eyeParameters, size_t apertureId, size_t focusId)
		{
			// Resulting dataframe - use the eye parameters as-is
			TensorFlow::DataSample sample = eyeParameters;

			// Fill in the measurement parameters
			sample["FocusDioptres"] = aberration.m_psfParameters.m_evaluatedParameters.m_focusDioptres[focusId];
			sample["PupilD"] = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[apertureId];

			// Return the result
			return sample;
		}

		////////////////////////////////////////////////////////////////////////////////
		TensorFlow::DataSample getRefocusedEyeParameters(Scene::Scene& scene, WavefrontAberration const& aberration,
			TensorFlow::DataSample const& eyeParameters, TensorFlow::DataSample const& refocusedParameters)
		{
			// Resulting dataframe - use the eye parameters as-is
			TensorFlow::DataSample sample = eyeParameters;

			// Calculate the ACD offset
			sample["LensD"] += refocusedParameters["DeltaLensD"];
			sample["AqueousT"] += refocusedParameters["DeltaAqueousT"];
			sample["VitreousT"] -= refocusedParameters["DeltaAqueousT"];

			// Return the result
			return sample;
		}

		////////////////////////////////////////////////////////////////////////////////
		TensorFlow::DataSample getAberrationEstimationInputs(Scene::Scene& scene, WavefrontAberration const& aberration,
			TensorFlow::DataSample const& eyeParameters, float horizontalAngle, float verticalAngle, float lambda, float aperture)
		{
			// Resulting dataframe - use the eye parameters as-is
			TensorFlow::DataSample sample = eyeParameters;

			// Fill in the measurement parameters
			sample["AngleHor"] = horizontalAngle;
			sample["AngleVert"] = verticalAngle;
			sample["Lambda"] = lambda;
			sample["PupilD"] = aperture;

			// Return the result
			return sample;
		}

		////////////////////////////////////////////////////////////////////////////////
		TensorFlow::DataSample getAberrationEstimationInputs(Scene::Scene& scene, WavefrontAberration const& aberration,
			TensorFlow::DataSample const& eyeParameters, size_t horizontalId, size_t verticalId, size_t lambdaId, size_t apertureId)
		{
			return getAberrationEstimationInputs(scene, aberration, eyeParameters, 
				aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal[horizontalId],
				aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical[verticalId],
				aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[lambdaId],
				aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[apertureId]);
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha extractEstimatedAberrations(Scene::Scene& scene, WavefrontAberration const& aberration,
			TensorFlow::DataSample const& aberrationCoefficients)
		{
			ZernikeCoefficientsAlpha result;
			for (size_t c = 2; c < result.size(); ++c)
			{
				const std::string name = "Aberration" + ZernikeIndices::s_zernikeNames[c].m_name;
				const size_t coefficientId = ZernikeIndices::s_zernikeNames[c].m_noll;
				result[coefficientId] = aberrationCoefficients[name];
			}
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string aberrationEstimatorName(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			return aberration.m_psfParameters.m_forceOnAxisNetwork ?
				s_aberrationEstimatorOnAxis :
				s_aberrationEstimatorOffAxis;
		}
		
		#ifdef HAS_Matlab
		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha computeEyeAberrationsMatlab(Scene::Scene& scene, WavefrontAberration& aberration, 
			TensorFlow::DataSample const& eyeParameters, float pupilD, float hAngle, float vAngle, float lambda)
		{
			const int numRays = scene.m_tfModels[aberrationEstimatorName(scene, aberration)].m_metadata["data_config"]["num_rays"].get<int>();

			matlab::data::Array aberrations = Matlab::g_matlab->feval(
				"compute_eye_aberrations",
				{
					// Input ray parameters
					Matlab::scalarToDataArray<double>(pupilD),
					Matlab::scalarToDataArray<double>(hAngle),
					Matlab::scalarToDataArray<double>(vAngle),
					Matlab::scalarToDataArray<double>(lambda),

					// Zernike parameters
					Matlab::scalarToDataArray<double>(ZernikeCoefficientsAlpha::MAX_DEGREES),

					// Number of rays
					Matlab::scalarToDataArray<double>(numRays),

					// Eye parameters
					Matlab::tfDataSampleToCellArray<double>(eyeParameters)
				},
				aberration.m_psfParameters.m_logProgress ? Matlab::logged_buffer(Debug::Info) : Matlab::logged_buffer(Debug::Null),
				Matlab::logged_buffer(Debug::Error));

			return extractMatlabAberrations(aberrations);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::array<float, 4> computeFocusedEyeParamsMatlab(Scene::Scene& scene, WavefrontAberration& aberration,
			TensorFlow::DataSample const& eyeParameters, const float pupilD, const float focusM)
		{
			const int numRays = scene.m_tfModels[s_refocusEstimator].m_metadata["data_config"]["num_rays"].get<int>();
			const int numPasses = scene.m_tfModels[s_refocusEstimator].m_metadata["data_config"]["num_passes"].get<int>();
			const int numSubdivs = scene.m_tfModels[s_refocusEstimator].m_metadata["data_config"]["num_subdivisions"].get<int>();

			std::vector<matlab::data::Array> returnVal = Matlab::g_matlab->feval(
				"compute_focused_eye",
				4,
				{
					// Eye arameters
					Matlab::scalarToDataArray<double>(pupilD),
					Matlab::scalarToDataArray<double>(focusM),

					// Focus estimation parameters
					Matlab::scalarToDataArray<double>(numPasses),
					Matlab::scalarToDataArray<double>(numRays),
					Matlab::scalarToDataArray<double>(numSubdivs),

					// Eye parameters
					Matlab::tfDataSampleToCellArray<double>(eyeParameters)
				},
				aberration.m_psfParameters.m_logProgress ? Matlab::logged_buffer(Debug::Info) : Matlab::logged_buffer(Debug::Null),
				Matlab::logged_buffer(Debug::Error));

			return { returnVal[0][0], returnVal[1][0], returnVal[2][0], returnVal[3][0] };
		}

		////////////////////////////////////////////////////////////////////////////////
		float computeFocusDistanceMatlab(Scene::Scene& scene, WavefrontAberration& aberration,
			TensorFlow::DataSample const& eyeParameters, float pupilD)
		{
			const int numRays = scene.m_tfModels[s_refocusEstimator].m_metadata["data_config"]["num_rays"].get<int>();
			const int numPasses = scene.m_tfModels[s_refocusEstimator].m_metadata["data_config"]["num_passes"].get<int>();
			const int numSubdivs = scene.m_tfModels[s_refocusEstimator].m_metadata["data_config"]["num_subdivisions"].get<int>();

			matlab::data::Array fdist = Matlab::g_matlab->feval(
				"compute_eye_focus_distance",
				{
					// Eye arameters
					Matlab::scalarToDataArray<double>(pupilD),

					// Focus estimation parameters
					Matlab::scalarToDataArray<double>(numPasses),
					Matlab::scalarToDataArray<double>(numRays),
					Matlab::scalarToDataArray<double>(numSubdivs),

					// Eye parameters
					Matlab::tfDataSampleToCellArray<double>(eyeParameters)
				},
				aberration.m_psfParameters.m_logProgress ? Matlab::logged_buffer(Debug::Info) : Matlab::logged_buffer(Debug::Null),
				Matlab::logged_buffer(Debug::Error));

			return fdist[0];
		}
		#endif

		////////////////////////////////////////////////////////////////////////////////
		float computeTrueFocusDistanceMatlab(Scene::Scene& scene, WavefrontAberration& aberration, 
			TensorFlow::DataSample const& eyeParameters, const float pupilDiameter)
		{
			float result = 0.0f;
			#ifdef HAS_Matlab
			if (Matlab::matlabEnabled())
				result = computeFocusDistanceMatlab(scene, aberration, eyeParameters, pupilDiameter);
			#endif
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		float computeTrueFocusDistanceMatlab(Scene::Scene& scene, WavefrontAberration& aberration, const size_t a, const size_t f)
		{
			return computeFocusDistanceMatlab(scene, aberration,
				aberration.m_psfStack.m_focusedEyeParameters[a][f].m_eyeParameters,
				aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[a]);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::array<float, 4> computeTrueFocusedEyeParamsMatlab(Scene::Scene& scene, WavefrontAberration& aberration,
			TensorFlow::DataSample const& eyeParameters, const float pupilDiameter, const float focusDistance)
		{
			std::array<float, 4> result{ 0.0f };
			#ifdef HAS_Matlab
			if (Matlab::matlabEnabled())
				result = computeFocusedEyeParamsMatlab(scene, aberration, eyeParameters, pupilDiameter, focusDistance);
			#endif
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::array<float, 4> computeTrueFocusedEyeParamsMatlab(Scene::Scene& scene, WavefrontAberration& aberration, const size_t a, const size_t f)
		{
			return computeTrueFocusedEyeParamsMatlab(scene, aberration,
				aberration.m_psfStack.m_relaxedEyeParameters.m_eyeParameters,
				aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[a],
				aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances[f]);
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha computeTrueMeasurementAberrationsNN(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			TensorFlow::DataSample aberrationEstimationInputs = EyeEstimation::getAberrationEstimationInputs(
				scene, aberration, aberration.m_psfStack.m_relaxedEyeParameters.m_eyeParameters,
				0.0f, 0.0f,
				aberration.m_aberrationParameters.m_lambda,
				aberration.m_aberrationParameters.m_apertureDiameter);
			TensorFlow::DataSample aberrationCoefficients = TensorFlow::predictDf(
				scene.m_tfModels[s_aberrationEstimatorOnAxis], aberrationEstimationInputs).toSample();

			//Debug::log_debug() << "Aberration estimator inputs: " << aberrationEstimationInputs << Debug::end;
			//Debug::log_debug() << "Corresponding aberration coefficients: " << aberrationCoefficients << Debug::end;

			return EyeEstimation::extractEstimatedAberrations(scene, aberration, aberrationCoefficients);
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha computeTrueMeasurementAberrationsMatlab(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			ZernikeCoefficientsAlpha result;
			#ifdef HAS_Matlab
			if (Matlab::matlabEnabled()) 
				result = computeEyeAberrationsMatlab(scene, aberration, 
					aberration.m_psfStack.m_relaxedEyeParameters.m_eyeParameters,
					aberration.m_aberrationParameters.m_apertureDiameter, 
					0.0f, 0.0f, 
					aberration.m_aberrationParameters.m_lambda);
			#endif
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha computeTrueEyeAberrationsMatlab(Scene::Scene& scene, WavefrontAberration& aberration, 
			TensorFlow::DataSample const& eyeParameters, const size_t a, const size_t h, const size_t v, const size_t l)
		{
			const float aperture = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[a];
			const float angleHorizontal = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal[h];
			const float angleVertical = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical[v];
			const float lambda = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[l];

			ZernikeCoefficientsAlpha result;
			#ifdef HAS_Matlab
			if (Matlab::matlabEnabled()) 
				result = computeEyeAberrationsMatlab(scene, aberration, eyeParameters, 
					aperture, angleHorizontal, angleVertical, lambda);
			#endif
			return result;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Vnm
	{
		// Scalar type used in the wkl computations
		using WklScalar = long double;
		using InterpolationScalar = double;
		using ScalarRowVectorInterpolation = Eigen::Matrix<InterpolationScalar, 1, Eigen::Dynamic>;

		// computes ln(x)
		WklScalar ln(WklScalar x)
		{
			return gsl_sf_log(x);
			//return std::log(x);
		}

		// computes exp(x)
		WklScalar exp(WklScalar x)
		{
			return gsl_sf_exp(x);
			//return std::exp(x);
		}

		// computes gamma(x)
		WklScalar lngamma(WklScalar x)
		{
			//return gsl_sf_lngamma(x);
			return std::lgamma(x);
		}

		// computes the log of k!
		WklScalar lnfact(unsigned int k)
		{
			//return gsl_sf_lnfact(k);
			return lngamma(k + 1.0);
		}

		// computes the log of n choose k
		WklScalar lnchoose(unsigned int n, unsigned int m)
		{
			return gsl_sf_lnchoose(n, m);
			//if (m == n || m == 0) return 0.0;
			//if (m * 2 > n) m = n - m;
			//return lnfact(n) - lnfact(m) - lnfact(n - m);
		}

		// evaluates the spherical bessel function
		WklScalar sph_bessel(unsigned int v, WklScalar x)
		{
			return boost::math::sph_bessel(v, x);
		}

		// evaluates the cylindrical bessel function
		WklScalar cyl_bessel(unsigned int v, WklScalar x)
		{
			return boost::math::cyl_bessel_j(v, x);
		}

		// Computes ln(A_k)
		WklScalar Ak_ln(long long k)
		{
			return lnchoose(2 * k, k);
		};

		// Computes log(f_ps^m) for s = 0, ..., p
		// NOTE: this is missing the -1^(p-s) term to force the arg of log to be always positive
		WklScalar fpsm_ln(long long p, long long s, long long m)
		{
			return
				ln(WklScalar(2 * s + 1)) - ln(WklScalar(p + s + 1)) +
				lnchoose(m + p - s - 1, m - 1) + lnchoose(m + p + s, s) - lnchoose(p + s, s);
		};
		
		// Computes f_ps^m for s = 0, ..., p
		WklScalar fpsm(long long p, long long s, long long m)
		{
			return
				WklScalar(std::pow(-1, p - s)) *
				(WklScalar(2 * s + 1) / WklScalar(p + s + 1)) *
				exp(WklScalar(lnchoose(m + p - s - 1, m - 1) + lnchoose(m + p + s, s)) - WklScalar(lnchoose(p + s, s)));
		};

		// Computes log(g_ul^m) for u = l, ..., l + m
		WklScalar gulm_ln(long long u, long long l, long long m)
		{
			return
				ln(WklScalar(m + 2 * l + 1)) - ln(WklScalar(m + u + l + 1)) +
				lnchoose(m, u - l) + lnchoose(u + l, l) - lnchoose(m + l + u, m + l);
		};

		// Computes g_ul^m for u = l, ..., l + m
		WklScalar gulm(long long u, long long l, long long m)
		{
			return
				(u < l || u > l + m) ? 0.0 :
				(WklScalar(m + 2 * l + 1) / WklScalar(m + u + l + 1)) *
				exp(WklScalar(lnchoose(m, u - l) + lnchoose(u + l, l)) - WklScalar(lnchoose(m + l + u, m + l)));
		};

		// Computes log(b_s2s2t) for t = 0, ..., min(s1, s2)
		WklScalar bs1s2t_ln(long long s1, long long s2, long long t)
		{
			return
				ln(WklScalar(2 * s1 + 2 * s2 - 4 * t + 1)) - ln(WklScalar(2 * s1 + 2 * s2 - 2 * t + 1)) +
				Ak_ln(s1 - t) + Ak_ln(t) + Ak_ln(s2 - t) - Ak_ln(s1 + s2 - t);
		};

		// Computes b_s2s2t for t = 0, ..., min(s1, s2)
		WklScalar bs1s2t(long long s1, long long s2, long long t)
		{
			return 
				(WklScalar(2 * s1 + 2 * s2 - 4 * t + 1) / WklScalar(2 * s1 + 2 * s2 - 2 * t + 1)) *
				exp(WklScalar(Ak_ln(s1 - t) + Ak_ln(t) + Ak_ln(s2 - t)) - WklScalar(Ak_ln(s1 + s2 - t)));
		};

		// Computes w_kl
		WklScalar wkl(long long n, long long m, long long p, long long q, long long k, long long l)
		{
			WklScalar wkl = 0.0;
			if (m == 0) // Handle the special case
			{
				long long j = (k + p - l) / 2; // from: l = k + p - 2j
				if (j <= glm::min(k, p) && (k + p - l) % 2 == 0)
				{
					wkl = exp(bs1s2t_ln(k, p, j));
				}
			}
			else // m <> 0
			{
				for (long long s = 0; s <= p; ++s)
				{
					const Vnm::WklScalar f = fpsm_ln(p, s, m);
					for (long long t = 0; t <= glm::min(k, s); ++t)
					{
						const long long u = k + s - 2 * t;
						if ((u >= l && u <= l + m))
						{
							const WklScalar b = bs1s2t_ln(k, s, t);
							const WklScalar g = gulm_ln(u, l, m);
							wkl += WklScalar(std::pow(-1, p - s)) * exp(f + b + g); // NOTE: apply -1^(p-s) from f_ps^m here
						}
					}
				}
			}
			return wkl;
		}
		////////////////////////////////////////////////////////////////////////////////
		EigenTypes::ScalarRowVectorComputation computeVnmInnerTerms(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack,
			long long n, long long m, long long am, long long k, long long p, long long q, size_t samples)
		{
			// Accumulate the wkl * Jn values
			EigenTypes::ScalarRowVectorComputation result = EigenTypes::ScalarRowVectorComputation::Zero(samples);
			for (long long l = glm::max(0ll, glm::max(k - q, p - k)), li = 0; l <= k + p; ++l, ++li)
				result += stack.m_enzWklCoefficients[n][am][k][li] * stack.m_enzCylindricalBessel[am + 2 * l + 1];
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		EigenTypes::ComplexMatrixComputation computeVnm(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack,
			const size_t psfId, const size_t coeffId, 
			PsfStackElements::PsfEntryParams const& psfParams, EigenTypes::ScalarMatrixComputation const& rPupil)
		{
			// Double zernike indices
			const auto [n, m] = ZernikeIndices::single2doubleNoll(coeffId);
			const long long am = glm::abs(m), p = (n - am) / 2, q = (n + am) / 2;

			// Weighting term
			const ComplexComputation totalWeight = std::exp(ComplexComputation(0.0, 0.5 * psfParams.m_focus.m_defocusParam));

			// Loop through each k
			EigenTypes::ComplexRowVectorComputation vnms = EigenTypes::ComplexRowVectorComputation::Zero(psfParams.m_enzSampling.m_samples);
			for (int k = 0; k < psfParams.m_enzSampling.m_terms; ++k)
			{
				// Extract the corresponding spherical bessel term (j_k)
				const ScalarComputation jk = stack.m_enzSphericalBessel[k][psfId];

				// Weighting factor for the current term
				const ComplexComputation termWeight = totalWeight * ScalarComputation(2 * k + 1) * std::pow(ComplexComputation(0, 1), k) * jk;

				if (aberration.m_psfParameters.m_precomputeVnmLSum) // use the pre-computed inner terms
				{
					vnms += termWeight * stack.m_enzVnmInnerTerms[n][am][k].block(0, 0, 1, psfParams.m_enzSampling.m_samples);
				}
				else // compute the inner terms on the fly
				{
					vnms += termWeight * computeVnmInnerTerms(scene, aberration, stack, n, m, am, k, p, q, psfParams.m_enzSampling.m_samples);
				}
			}

			// Handle the case when we have only 1 samples
			if (psfParams.m_sampling.m_samples == 1)
				return EigenTypes::ComplexMatrixComputation({ { vnms[0] } });

			// Init the interpolation objects
			ScalarRowVectorInterpolation radiusInterpolation = stack.m_enzEntrySamplingParameters.m_radius.cast<InterpolationScalar>();
			gsl_interp* lerp = gsl_interp_alloc(gsl_interp_linear, vnms.size());
			gsl_interp_accel* accel = gsl_interp_accel_alloc();

			//Debug::log_debug() << radiusInterpolation[0] << ", " << rPupil.minCoeff() << Debug::end;
			//Debug::log_debug() << radiusInterpolation[vnms.size() - 1] << ", " << rPupil.maxCoeff() << Debug::end;

			// Interpolate the real Vnm terms
			ScalarRowVectorInterpolation reInterpolation = vnms.real().cast<InterpolationScalar>();
			EigenTypes::ScalarMatrixComputation vnmRe(rPupil.rows(), rPupil.cols());
			gsl_interp_init(lerp, radiusInterpolation.data(), reInterpolation.data(), vnms.size());
			std::transform(rPupil.data(), rPupil.data() + rPupil.size(), vnmRe.data(), [&](const InterpolationScalar radius)
				{ return radius > psfParams.m_sampling.m_halfExtent ? 0.0f : 
					gsl_interp_eval(lerp, radiusInterpolation.data(), reInterpolation.data(), radius, accel); });

			// Interpolate the imaginary Vnm terms
			ScalarRowVectorInterpolation imInterpolation = vnms.imag().cast<InterpolationScalar>();
			EigenTypes::ScalarMatrixComputation vnmIm(rPupil.rows(), rPupil.cols());
			gsl_interp_init(lerp, radiusInterpolation.data(), imInterpolation.data(), vnms.size());
			std::transform(rPupil.data(), rPupil.data() + rPupil.size(), vnmIm.data(), [&](const InterpolationScalar radius)
				{ return radius > psfParams.m_sampling.m_halfExtent ? 0.0f : 
					gsl_interp_eval(lerp, radiusInterpolation.data(), imInterpolation.data(), radius, accel); });

			// Release the interpolator objects
			gsl_interp_free(lerp);
			gsl_interp_accel_free(accel);

			// Join the two components to produce the result
			return vnmRe + ComplexComputation(0.0, 1.0) * vnmIm;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace PsfOptimization
	{
		////////////////////////////////////////////////////////////////////////////////
		Eigen::Block<const Psf> centeredBlock(Psf const& psf, size_t radius)
		{
			const size_t startRow = psf.rows() / 2 - radius;
			const size_t startCol = psf.cols() / 2 - radius;
			const size_t numRows = 2 * radius + 1;
			const size_t numCols = 2 * radius + 1;
			return psf.block(startRow, startCol, numRows, numCols);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::array<Eigen::Block<const Psf>, 4> outerBlocks(Psf const& psf, size_t radius)
		{
			const size_t numRows = 2 * radius + 1;
			const size_t numCols = 2 * radius + 1;
			const size_t startRowTop = psf.rows() / 2 - radius;
			const size_t startColLeft = psf.cols() / 2 - radius;
			const size_t startRowBot = psf.rows() / 2 + radius;
			const size_t startColRight = psf.cols() / 2 + radius;
			return 
			{
				psf.block(startRowTop, startColLeft, numRows, 1),
				psf.block(startRowTop, startColRight, numRows, 1),
				psf.block(startRowTop, startColLeft, 1, numCols),
				psf.block(startRowBot, startColLeft, 1, numCols)
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		float blockSum(Psf const& psf, size_t radius)
		{
			return centeredBlock(psf, radius).sum();
		}

		////////////////////////////////////////////////////////////////////////////////
		float marginMaxCoeff(Psf const& psf, size_t radius)
		{
			auto const& blocks = outerBlocks(psf, radius);
			return glm::max
			(
				glm::max(blocks[0].maxCoeff(), blocks[1].maxCoeff()),
				glm::max(blocks[2].maxCoeff(), blocks[3].maxCoeff())
			);
		}

		////////////////////////////////////////////////////////////////////////////////
		Psf cropSum(Psf const& psf, ScalarFinal threshold)
		{
			// Traverse the varius radii and find the first radius where we are above the threshold
			for (size_t radius = 0; radius <= psf.rows() / 2; ++radius)
				if (blockSum(psf, radius) >= threshold)
					return centeredBlock(psf, radius);
				//{
				//	Debug::log_debug() << "Crop sum removed " << (psf.rows() / 2 - radius) << " rows" << Debug::end;
				//	return centeredBlock(psf, radius);
				//}

			// Fall back to the original
			return psf;
		}

		////////////////////////////////////////////////////////////////////////////////
		Psf cropCoeff(Psf const& psf, ScalarFinal threshold)
		{
			// Traverse the varius radii and find the first radius where we are above the threshold
			for (size_t radius = psf.rows() / 2; radius > 0; --radius)
			{
				//Debug::log_debug() << " > " << (psf.rows() / 2) << "/" << radius << ": " << marginMaxCoeff(psf, radius) << "/" << threshold << Debug::end;
				if (marginMaxCoeff(psf, radius) >= threshold)
					return centeredBlock(psf, radius);
				//{
				//	//Debug::log_debug() << "Crop coeff removed " << (psf.rows() / 2 - radius) << " rows" << Debug::end;
				//	return centeredBlock(psf, radius);
				//}
			}

			// Fall back to the original
			return psf;
		}

		////////////////////////////////////////////////////////////////////////////////
		Psf normalize(Psf const& psf)
		{
			return psf / psf.sum();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace ComputePsfStack
	{
		////////////////////////////////////////////////////////////////////////////////
		Debug::LogOutput& operator<<(Debug::LogOutput& output, PSFStackParameters::ParameterRange parameters)
		{
			return output << "{ " << "Min: " << parameters.m_min << ", Max: " << parameters.m_max << ", Steps: " << parameters.m_numSteps << ", Step Size: " << parameters.m_step << " }";
		}

		////////////////////////////////////////////////////////////////////////////////
		Debug::DebugOutputLevel progressLogLevel(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			return aberration.m_psfParameters.m_logProgress ? Debug::Info : Debug::Null;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldRecomputeVnmInner(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			if (!aberration.m_psfParameters.m_precomputeVnmLSum) return false;

			return
				((computation & PsfStackComputation_PsfBesselTerms || computation & PsfStackComputation_PsfEnzCoefficients) && result.m_enzEntrySamplingParameters.m_isCylindricalDirty) ||
				aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU && scene.m_genericBuffers.find("PsfStack_VnmInnerBuffer") == scene.m_genericBuffers.end();
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldRecomputeEnzWkl(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			return computation & PsfStackComputation_PsfEnzCoefficients && result.m_enzEntrySamplingParameters.m_isWklDirty;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldRecomputeSphericalBessel(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			return computation & PsfStackComputation_PsfBesselTerms;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldRecomputeCylindricalBessel(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			return computation & PsfStackComputation_PsfBesselTerms && 
				(!aberration.m_psfParameters.m_precomputeVnmLSum || result.m_enzEntrySamplingParameters.m_isCylindricalDirty);
		}

		#ifdef HAS_Matlab
		////////////////////////////////////////////////////////////////////////////////
		// Matlab solver names
		static std::unordered_map<Aberration::EyeReconstructionParameters::Optimizer, std::string> s_optimizerParamNames =
		{
			{ Aberration::EyeReconstructionParameters::PatternSearch, "patternsearch" },
		};

		////////////////////////////////////////////////////////////////////////////////
		// Matlab optimizer names
		static std::unordered_map<Aberration::EyeReconstructionParameters::Solver, std::string> s_solverParamNames =
		{
			{ Aberration::EyeReconstructionParameters::Gps2N, "GPS_2N" },
			{ Aberration::EyeReconstructionParameters::GpsNp1, "GPS_NP1" },
			{ Aberration::EyeReconstructionParameters::Gss2N, "GSS_2N" },
			{ Aberration::EyeReconstructionParameters::GssNp1, "GSS_NP1" },
			{ Aberration::EyeReconstructionParameters::Mads2N, "MADS_2N" },
			{ Aberration::EyeReconstructionParameters::MadsNp1, "MADS_NP1" },
			{ Aberration::EyeReconstructionParameters::Gps2NMads2N, "GPS_2N_MADS_2N" },
			{ Aberration::EyeReconstructionParameters::GpsNp1Mads2N, "GPS_NP1_MADS_2N" },
			{ Aberration::EyeReconstructionParameters::Gss2NMads2N, "GSS_2N_MADS_2N" },
			{ Aberration::EyeReconstructionParameters::GssNp1Mads2N, "GSS_NP1_MADS_2N" },
			{ Aberration::EyeReconstructionParameters::Mads2NGps2N, "MADS_2N_GPS_2N" },
			{ Aberration::EyeReconstructionParameters::MadsNp1Gps2N, "MADS_NP1_GPS_2N" },
		};

		////////////////////////////////////////////////////////////////////////////////
		void computeEyeParametersMatlab(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& result)
		{
			// Total number of PSFs
			const size_t numDefocues = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size();
			const size_t numAnglesHor = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size();
			const size_t numAnglesVert = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size();
			const size_t numLambdas = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size();
			const size_t numApertures = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size();
			const size_t numFocuses = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size();
			const size_t numPsfs = numDefocues * numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numCoefficients = numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;

			auto timer = result.m_timers.startComputation("Aberration Coefficients", numCoefficients);

			// Compute the aberration coeffs
			// Result layout: 
			//   0: aberrations
			//   1: pupil_retina_distances
			std::vector<matlab::data::Array> returnVal = Matlab::g_matlab->feval(
				"compute_aberrations_parametric",
				2,
				{
					// Aberration parameters
					Matlab::stringToDataArray(aberration.m_name),
					Matlab::scalarToDataArray<double>(aberration.m_aberrationParameters.m_apertureDiameter),
					Matlab::scalarToDataArray<double>(aberration.m_aberrationParameters.m_lambda),
					Matlab::eigenToDataArray(EyeEstimation::aberrationsToEigen(scene, aberration)),

					// Reconstruction parameters
					Matlab::scalarToDataArray<double>(aberration.m_reconstructionParameters.m_numRays),
					Matlab::scalarToDataArray<double>(ZernikeCoefficientsAlpha::MAX_DEGREES),
					Matlab::scalarToDataArray<double>(aberration.m_reconstructionParameters.m_timeLimit),
					Matlab::scalarToDataArray<double>(aberration.m_reconstructionParameters.m_anatomicalWeightBoundary),
					Matlab::scalarToDataArray<double>(aberration.m_reconstructionParameters.m_anatomicalWeightAverage),
					Matlab::scalarToDataArray<double>(aberration.m_reconstructionParameters.m_functionalWeightSpecified),
					Matlab::scalarToDataArray<double>(aberration.m_reconstructionParameters.m_functionalWeightUnspecified),
					Matlab::stringToDataArray(s_optimizerParamNames[aberration.m_reconstructionParameters.m_optimizer]),
					Matlab::stringToDataArray(s_solverParamNames[aberration.m_reconstructionParameters.m_solver]),

					// Parameter ranges
					Matlab::eigenToDataArray(EyeEstimation::vectorToEigen(aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal)),
					Matlab::eigenToDataArray(EyeEstimation::vectorToEigen(aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical)),
					Matlab::eigenToDataArray(EyeEstimation::vectorToEigen(aberration.m_psfParameters.m_evaluatedParameters.m_lambdas)),
					Matlab::eigenToDataArray(EyeEstimation::vectorToEigen(aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters)),
					Matlab::eigenToDataArray(EyeEstimation::vectorToEigen(aberration.m_psfParameters.m_evaluatedParameters.m_focusDioptres)),
				},
				aberration.m_psfParameters.m_logProgress ? Matlab::logged_buffer(Debug::Info) : Matlab::logged_buffer(Debug::Null),
				Matlab::logged_buffer(Debug::Error));

			// Extract a reference to the results
			matlab::data::Array const& aberrationCoefficients = returnVal[0];
			matlab::data::Array const& pupilRetinaDist = returnVal[1];

			// Convert them to Zernike coeff arrays
			for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
			for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
			for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
			for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
			for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
			{
				auto& coeffVariants = result.m_aberrationCoefficients[h][v][l][a][f];
				coeffVariants.m_alpha = EyeEstimation::extractMatlabAberrations(aberrationCoefficients, h, v, l, a, f);
			}

			// Also store the pupil-retina distances
			for (size_t a = 0; a < result.m_focusedEyeParameters.size(); ++a)
			for (size_t f = 0; f < result.m_focusedEyeParameters[0].size(); ++f)
			{
				result.m_focusedEyeParameters[a][f].m_pupilRetinaDistance = pupilRetinaDist[a][f];
			}
		}
		#endif

		////////////////////////////////////////////////////////////////////////////////
		void computeEyeParametersNeuralNetwork(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			// Total number of PSFs
			const size_t numDefocues = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size();
			const size_t numAnglesHor = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size();
			const size_t numAnglesVert = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size();
			const size_t numLambdas = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size();
			const size_t numApertures = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size();
			const size_t numFocuses = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size();
			const size_t numPsfs = numDefocues * numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numCoefficients = numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numFocusedEyes = numApertures * numFocuses;

			// Compute the relaxed eye parameters
			if (computation & PsfStackComputation_RelaxedEyeParameters)
			{
				auto timer = result.m_timers.startComputation("Eye Parameters", 1);

				TensorFlow::DataSample eyeEstimationInputs = EyeEstimation::getEyeEstimationInputs(scene, aberration);
				TensorFlow::DataFrame eyeParametersDF;
				{
					auto timer = result.m_timers.startComputation("Eye Parameters (NN)", 1);

					eyeParametersDF = TensorFlow::predictDf(scene.m_tfModels[s_eyeEstimator], eyeEstimationInputs);
				}
				result.m_relaxedEyeParameters.m_eyeParameters = eyeParametersDF.toSample();

				//Debug::log_debug() << "Eye estimator inputs: " << eyeEstimationInputs << Debug::end;
				//Debug::log_debug() << "Corresponding eye parameters: " << eyeParameters << Debug::end;

				if (aberration.m_psfParameters.m_collectDebugInfo)
				{
					result.m_debugInformationCommon.m_eyeParameters = result.m_relaxedEyeParameters.m_eyeParameters.data();
					result.m_debugInformationCommon.m_alphaTrueNN = EyeEstimation::computeTrueMeasurementAberrationsNN(scene, aberration);
					result.m_debugInformationCommon.m_alphaTrueRT = EyeEstimation::computeTrueMeasurementAberrationsMatlab(scene, aberration);
				}
			}

			// Compute the refocused eye parameters
			if (computation & PsfStackComputation_FocusedEyeParameters)
			{
				auto timer = result.m_timers.startComputation("Refocused Eye Parameters", numFocusedEyes);

				// Create a tensor with all the refocus estimation inputs
				TensorFlow::DataFrame refocusEstimationInputs;
				refocusEstimationInputs.reserve(numApertures * numFocuses);
				for (size_t a = 0; a < numApertures; ++a)
				for (size_t f = 0; f < numFocuses; ++f)
				{
					refocusEstimationInputs.append(EyeEstimation::getRefocusEstimationInputs(
						scene, aberration, result.m_relaxedEyeParameters.m_eyeParameters, a, f));
				}

				// Compute the refocused parameters
				TensorFlow::DataFrame refocusedParameters;
				{
					auto timer = result.m_timers.startComputation("Refocused Eye Parameters (NN)", numFocusedEyes);

					refocusedParameters = TensorFlow::predictDf(
						scene.m_tfModels[s_refocusEstimator], refocusEstimationInputs);
				}

				//Debug::log_debug() << "Refocus estimation inputs: " << refocusEstimationInputs << Debug::end;
				//Debug::log_debug() << "Refocus estimation results" << refocusedParameters << Debug::end;

				for (size_t a = 0; a < numApertures; ++a)
				for (size_t f = 0; f < numFocuses; ++f)
				{
					// Construct the full refocused eye parameters
					const size_t refocusRowId = a * numFocuses + f;
					TensorFlow::DataSample refocusedEyeParameters = EyeEstimation::getRefocusedEyeParameters(
						scene, aberration, result.m_relaxedEyeParameters.m_eyeParameters, refocusedParameters.toSample(refocusRowId));

					// Store the eye parameter tensor
					result.m_focusedEyeParameters[a][f].m_eyeParameters = refocusedEyeParameters;

					// Store the various focal lengths
					result.m_focusedEyeParameters[a][f].m_pupilRetinaDistance =
						refocusedEyeParameters["EyeT"] - refocusedEyeParameters["CorneaT"] - refocusedEyeParameters["AqueousT"];

					// Compute the true focus distance of the resulting eye
					if (aberration.m_psfParameters.m_collectDebugInfo)
					{
						// L_D, A_T, delta_L_D, delta_A_T
						auto focusParams = EyeEstimation::computeTrueFocusedEyeParamsMatlab(scene, aberration, a, f);
						result.m_focusedEyeParameters[a][f].m_debugInformation.m_trueLensD = focusParams[0];
						result.m_focusedEyeParameters[a][f].m_debugInformation.m_trueAqueousT = focusParams[1];
						//result.m_focusedEyeParameters[a][f].m_debugInformation.m_trueFocusDistance = 
						//	EyeEstimation::computeTrueFocusDistanceMatlab(scene, aberration, a, f);
					}
				}
			}

			// Compute the aberration coefficients
			if (computation & PsfStackComputation_AberrationCoefficients)
			{
				auto timer = result.m_timers.startComputation("Aberration Coefficients", numCoefficients);

				// Create the aberration estimator input tensor
				TensorFlow::DataFrame aberrationEstimationInputs;
				aberrationEstimationInputs.reserve(numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses);
				for (size_t a = 0; a < numApertures; ++a)
				for (size_t f = 0; f < numFocuses; ++f)
				{
					// Extract the corresponding eye parameters
					TensorFlow::DataSample const& refocusedEyeParameters = result.m_focusedEyeParameters[a][f].m_eyeParameters;

					// Store the estimator properties
					for (size_t h = 0; h < numAnglesHor; ++h)
					for (size_t v = 0; v < numAnglesVert; ++v)
					for (size_t l = 0; l < numLambdas; ++l)
					{
						aberrationEstimationInputs.append(EyeEstimation::getAberrationEstimationInputs(
							scene, aberration, refocusedEyeParameters, h, v, l, a));
					}
				}
				
				// Compute the prediced coefficients
				TensorFlow::DataFrame aberrationCoefficients;
				{
					auto timer = result.m_timers.startComputation("Aberration Coefficients (NN)", numCoefficients);

					aberrationCoefficients = TensorFlow::predictDf(
						scene.m_tfModels[EyeEstimation::aberrationEstimatorName(scene, aberration)], aberrationEstimationInputs);
				}

				//Debug::log_debug() << "Aberration estimation inputs: " << aberrationEstimationInputs << Debug::end;
				//Debug::log_debug() << "Aberration estimation results: " << aberrationCoefficients << Debug::end;

				// Store the results
				size_t coeffRowId = 0;
				for (size_t a = 0; a < numApertures; ++a)
				for (size_t f = 0; f < numFocuses; ++f)
				for (size_t h = 0; h < numAnglesHor; ++h)
				for (size_t v = 0; v < numAnglesVert; ++v)
				for (size_t l = 0; l < numLambdas; ++l)
				{
					// Extract the coefficients
					TensorFlow::DataSample coeffs = aberrationCoefficients.toSample(coeffRowId);
					++coeffRowId;

					auto& coeffVariants = result.m_aberrationCoefficients[h][v][l][a][f];
					coeffVariants.m_alpha = EyeEstimation::extractEstimatedAberrations(scene, aberration, coeffs);
					if (aberration.m_psfParameters.m_collectDebugInfo)
					{
						// Extract the corresponding eye parameters
						TensorFlow::DataSample const& refocusedEyeParameters = result.m_focusedEyeParameters[a][f].m_eyeParameters;

						// Store the eye parameters
						coeffVariants.m_eyeParameters = refocusedEyeParameters.data();

						// Store the true coeffs
						coeffVariants.m_alphaTrue = EyeEstimation::computeTrueEyeAberrationsMatlab(scene, aberration, refocusedEyeParameters, a, h, v, l);
					}
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		ZernikeCoefficientsAlpha const& getAlphaToBetaCoefficient(Scene::Scene& scene, WavefrontAberration& aberration, 
			PsfStackElements::AberrationCoefficientVariants const& coefficients)
		{
			return Coefficients::selectCoefficients(coefficients, aberration.m_psfParameters.m_alphaToBetaCoefficient);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeBetaCoefficients(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& result)
		{
			// Total number of PSFs
			const size_t numDefocues = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size();
			const size_t numAnglesHor = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size();
			const size_t numAnglesVert = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size();
			const size_t numLambdas = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size();
			const size_t numApertures = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size();
			const size_t numFocuses = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size();
			const size_t numPsfs = numDefocues * numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numCoefficients = numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;

			auto timer = result.m_timers.startComputation("Beta Coefficients", numCoefficients);

			// Convert them to Zernike coeff arrays
			for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
			for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
			for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
			for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
			for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
			{
				auto& coeffVariants = result.m_aberrationCoefficients[h][v][l][a][f];

				coeffVariants.m_alpha[1] = -std::accumulate(coeffVariants.m_alpha.begin() + 2, coeffVariants.m_alpha.end(), 0.0);
				coeffVariants.m_alphaPhaseCumulative = Coefficients::opdToPhase(aberration, coeffVariants.m_alpha, 
					aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[l] * 1e-3f, true);
				coeffVariants.m_alphaPhaseResidual = Coefficients::opdToPhase(aberration, coeffVariants.m_alpha,
					aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[l] * 1e-3f, false);
			}

			// Collect the phase coefficients
			std::vector<ZernikeCoefficientsAlpha> alphas(numCoefficients);
			std::transform(result.m_aberrationCoefficients.data(), result.m_aberrationCoefficients.data() + numCoefficients,
				alphas.begin(), [&](auto const& coeffs) { return getAlphaToBetaCoefficient(scene, aberration, coeffs); });

			// Convert them to beta
			std::vector<ZernikeCoefficientsBeta> betas = Coefficients::alphaToBeta(aberration, alphas);
			for (size_t i = 0; i < numCoefficients; ++i)
				result.m_aberrationCoefficients.data()[i].m_beta = betas[i];
		}

		////////////////////////////////////////////////////////////////////////////////
		float computeFocalShiftAberration(Scene::Scene& scene, WavefrontAberration const& aberration, 
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			return Defocus::computeFocalShiftFromAberration(scene, aberration, psfParameters.m_units.m_s0, 
				psfParameters.m_units.m_focalShiftToDefocus, psfParameters.m_coefficients.m_alphaPhaseCumulative);
		}

		////////////////////////////////////////////////////////////////////////////////
		// TODO: should this actually use the focal length?
		// TODO: shouldn't this actually depend on wavelength?
		float computeFocalShiftObjectDepth(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters, const float planeDistance)
		{
			const float objectDistanceFocalLengthM = 1.0f / (1.0f / psfParameters.m_units.m_pupilRetinaDistanceM - 1.0f / planeDistance);
			return (psfParameters.m_units.m_pupilRetinaDistanceM - objectDistanceFocalLengthM) * 1e6f;
		}

		////////////////////////////////////////////////////////////////////////////////
		double computeTotalDefocus(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			// Use the manual defocus if requested
			if (aberration.m_psfParameters.m_manualDefocus)
				return aberration.m_psfParameters.m_desiredDefocus;

			// Compute the defocus param
			// TODO: this shouldn't be abs
			return glm::abs(psfParameters.m_focus.m_imageShiftMuM * psfParameters.m_units.m_focalShiftToDefocus);
		}

		////////////////////////////////////////////////////////////////////////////////
		float computeSamplingUnits(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			//const float focalShiftToSamplingUnits = -(glm::two_pi<float>() * psfParameters.m_units.m_refractiveIndex) / psfParameters.m_units.m_diffractionUnit * 1e-3f;
			//const float focalShiftToSamplingUnits = -(glm::two_pi<float>() * psfParameters.m_units.m_lambdaMuM) * 1e-3f;
			//const float focalShiftToSamplingUnits = -(glm::two_pi<float>() * psfParameters.m_units.m_diffractionUnit) * 1e-4f;
			const float focalShiftToSamplingUnits = -(glm::two_pi<float>() * psfParameters.m_units.m_s0) / psfParameters.m_units.m_lambdaMuM * 5e-3f;

			const float samplingUnits = glm::abs(psfParameters.m_focus.m_imageShiftMuM * focalShiftToSamplingUnits);
			//const float samplingUnits = glm::max(
			//	glm::abs(psfParameters.m_focus.m_imageShiftAberrationMuM * focalShiftToSamplingUnits), 
			//	glm::abs(psfParameters.m_focus.m_imageShiftObjectDepthMuM * focalShiftToSamplingUnits));
			const float samplingUnitsMin = aberration.m_psfParameters.m_minSamplingUnits;
			const float samplingUnitsMax = aberration.m_psfParameters.m_maxSamplingUnits;
			return glm::clamp(samplingUnits, samplingUnitsMin, samplingUnitsMax);
		}

		////////////////////////////////////////////////////////////////////////////////
		float computePsfSampling(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			const double diffractionUnit = psfParameters.m_units.m_diffractionUnit;
			const double samplingUnits = psfParameters.m_sampling.m_samplingUnits;
			const double approximationSampleSize = aberration.m_psfParameters.m_approximationSampleSize;
			const double sampleSizeMultiplier = aberration.m_psfParameters.m_psfSampleSizeMultiplier;
			return approximationSampleSize * diffractionUnit * sampleSizeMultiplier * glm::ceil(samplingUnits);
		}

		////////////////////////////////////////////////////////////////////////////////
		int computeNumPsfSamples(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			const double samplingUnits = psfParameters.m_sampling.m_samplingUnits;
			const int samples = int(glm::ceil(aberration.m_psfParameters.m_psfSampleCountMultiplier * samplingUnits));
			const int samplesMin = aberration.m_psfParameters.m_psfSamplesMin;
			const int samplesMax = aberration.m_psfParameters.m_psfSamplesMax;
			return (glm::clamp(samples, samplesMin, samplesMax) / 2) * 2 + 1;
		}

		////////////////////////////////////////////////////////////////////////////////
		int computeNumApproximationSamples(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			const int samples = glm::ceil(psfParameters.m_sampling.m_halfExtent / psfParameters.m_enzSampling.m_sampling) + 1;
			const int samplesMax = aberration.m_psfParameters.m_maxApproximationSamples;
			if (samples > samplesMax)
				Debug::log_error() << "Number of approximation samples (" << samples << ") exceeds the maximum allowed (" << samplesMax << ")" << Debug::end;
			return samples;
		}

		////////////////////////////////////////////////////////////////////////////////
		int computeNumApproximationTerms(Scene::Scene& scene, WavefrontAberration const& aberration,
			PsfStackElements::PsfEntryParams const& psfParameters)
		{
			const double samplingUnits = psfParameters.m_sampling.m_samplingUnits;
			const int terms = int(aberration.m_psfParameters.m_approximationTermsMultiplier * samplingUnits);
			const int termsMin = aberration.m_psfParameters.m_approximationTermsMin;
			const int termsMax = aberration.m_psfParameters.m_approximationTermsMax;
			return glm::clamp(terms, termsMin, termsMax);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computePsfEntryParameters(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, size_t defocusId, size_t horizontalId, size_t verticalId, size_t lambdaId, size_t apertureId, size_t focusId)
		{
			// Extract the corresponding values
			const PsfIndex psfIndex{ defocusId, horizontalId, verticalId, lambdaId, apertureId, focusId };
			const PsfCoeffsIndex psfCoeffsIndex{ horizontalId, verticalId, lambdaId, apertureId, focusId };
			const float planeDistance = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances[defocusId];
			const float horizontalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal[horizontalId];
			const float verticalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical[verticalId];
			const float lambdaNM = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[lambdaId];
			const float apertureDiameterMM = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[apertureId];
			const float focusDistance = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances[focusId];
			PsfStackElements::PsfEntryParams& result = stack.m_psfEntryParameters(psfIndex);

			Profiler::ScopedCpuPerfCounter perfCounter(scene, 
				"PSF Distance: " + std::to_string(planeDistance) + "m, " +
				"Horizontal: " + std::to_string(glm::degrees(horizontalAngle)) + "deg, " +
				"Vertical: " + std::to_string(glm::degrees(verticalAngle)) + "deg, " +
				"Wavelength: " + std::to_string(lambdaNM) + "nm, " +
				"Aperture Diameter: " + std::to_string(apertureDiameterMM) + "mm, " +
				"Focus Distance: " + std::to_string(focusDistance) + "m",
				false,  Threading::currentThreadId());

			// Fill in the PSF parameter structure
			result.m_units.m_focusDistanceM = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances[focusId];
			result.m_units.m_objectDistanceM = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances[defocusId];
			result.m_units.m_horizontalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal[horizontalId];
			result.m_units.m_verticalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical[verticalId];
			result.m_units.m_pupilRetinaDistanceM = stack.m_focusedEyeParameters[apertureId][focusId].m_pupilRetinaDistance * 1e-3f;
			result.m_units.m_pupilRetinaDistanceMuM = result.m_units.m_pupilRetinaDistanceM * 1e6f;
			result.m_units.m_apertureDiameterMM = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[apertureId];
			result.m_units.m_apertureDiameterMuM = result.m_units.m_apertureDiameterMM * 1e3f;
			result.m_units.m_apertureRadiusMuM = result.m_units.m_apertureDiameterMuM * 0.5f;
			result.m_units.m_refractiveIndex = aberration.m_refractiveIndex;
			result.m_units.m_lambdaMuM = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[lambdaId] * 1e-3f;
			// TODO: should this actually use the focal length?
			result.m_units.m_s0 = result.m_units.m_apertureDiameterMuM / (2.0f * result.m_units.m_pupilRetinaDistanceMuM);
			result.m_units.m_u0 = 1.0f - glm::sqrt(1.0f - result.m_units.m_s0 * result.m_units.m_s0);
			result.m_units.m_diffractionUnit = result.m_units.m_lambdaMuM / result.m_units.m_s0;
			result.m_units.m_axialDiffractionUnit = result.m_units.m_lambdaMuM / result.m_units.m_u0;
			result.m_units.m_focalShiftToDefocus = -(glm::two_pi<float>() * result.m_units.m_refractiveIndex) / result.m_units.m_axialDiffractionUnit;
			result.m_coefficients.m_alpha = stack.m_aberrationCoefficients(psfCoeffsIndex).m_alpha;
			if (aberration.m_psfParameters.m_collectDebugInfo)
			{
				result.m_debug.m_eyeParameters = stack.m_aberrationCoefficients(psfCoeffsIndex).m_eyeParameters;
				result.m_debug.m_alphaTrue = stack.m_aberrationCoefficients(psfCoeffsIndex).m_alphaTrue;
				result.m_debug.m_focusDistance = stack.m_focusedEyeParameters[apertureId][focusId].m_debugInformation.m_trueFocusDistance;
			}
			result.m_coefficients.m_alphaPhaseCumulative = stack.m_aberrationCoefficients(psfCoeffsIndex).m_alphaPhaseCumulative;
			result.m_coefficients.m_alphaPhaseResidual = stack.m_aberrationCoefficients(psfCoeffsIndex).m_alphaPhaseResidual;
			result.m_coefficients.m_beta = stack.m_aberrationCoefficients(psfCoeffsIndex).m_beta;
			result.m_focus.m_imageShiftAberrationMuM = computeFocalShiftAberration(scene, aberration, result);
			result.m_focus.m_imageShiftObjectDepthMuM = computeFocalShiftObjectDepth(scene, aberration, result, planeDistance);
			result.m_focus.m_imageShiftMuM = result.m_focus.m_imageShiftAberrationMuM + result.m_focus.m_imageShiftObjectDepthMuM;
			result.m_focus.m_defocusParam = computeTotalDefocus(scene, aberration, result);
			result.m_focus.m_defocusUnits = glm::abs(result.m_focus.m_defocusParam) / (glm::pi<double>() / 2.0);
			result.m_sampling.m_samplingUnits = computeSamplingUnits(scene, aberration, result);
			result.m_sampling.m_samplingMuM = computePsfSampling(scene, aberration, result);
			result.m_sampling.m_samples = computeNumPsfSamples(scene, aberration, result);
			result.m_sampling.m_halfExtentMuM = (result.m_sampling.m_samples / 2) * result.m_sampling.m_samplingMuM;
			result.m_sampling.m_halfExtent = result.m_sampling.m_halfExtentMuM / result.m_units.m_diffractionUnit;
			result.m_enzSampling.m_sampling = aberration.m_psfParameters.m_approximationSampleSize;
			result.m_enzSampling.m_samples = computeNumApproximationSamples(scene, aberration, result);
			result.m_enzSampling.m_extent = (result.m_enzSampling.m_samples * result.m_enzSampling.m_sampling);
			result.m_enzSampling.m_terms = computeNumApproximationTerms(scene, aberration, result);
		}

		/////////////////////////////////////////////////////////////////////////////////
		void computeEnzEntrySamplingParameters(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack)
		{
			PsfStackElements::EnzEntrySamplingParameters& result = stack.m_enzEntrySamplingParameters;

			Profiler::ScopedCpuPerfCounter perfCounter(scene, "ENZ Sampling Parameters");

			// Max number of Vnm samples
			result.m_maxSamples = aberration.m_psfParameters.m_maxApproximationSamples;

			// Max PSF extent
			result.m_maxExtent = result.m_maxSamples * aberration.m_psfParameters.m_approximationSampleSize;

			// Determine if the ENZ cache needs to be rebuilt or not
			result.m_isWklDirty =
				result.m_maxDegree != aberration.m_psfParameters.m_betaDegrees ||
				result.m_maxOrder != aberration.m_psfParameters.m_approximationTermsMax;

			// Determine if the ENZ cache needs to be rebuilt or not
			result.m_isCylindricalDirty =
				!aberration.m_psfParameters.m_precomputeVnmLSum ||
				result.m_maxDegree != aberration.m_psfParameters.m_betaDegrees ||
				result.m_maxOrder != aberration.m_psfParameters.m_approximationTermsMax ||
				result.m_maxExtent != result.m_radius[result.m_radius.cols() - 1] ||
				stack.m_debugInformationCommon.m_backend != aberration.m_psfParameters.m_backend;

			// Maximum K
			result.m_maxOrder = aberration.m_psfParameters.m_approximationTermsMax;

			// Maximum Beta coefficient degree
			result.m_maxDegree = aberration.m_psfParameters.m_betaDegrees;

			// Maximum number of coefficients
			result.m_maxCoefficients = numZernikeCoefficients(result.m_maxDegree, true);

			// Maximum orders for the various terms
			result.m_maxTermOrder = 2 * result.m_maxOrder + aberration.m_psfParameters.m_betaDegrees + 1 + 1;

			// Determine the coefficient limits
			result.m_maxTermsPerOrder = aberration.m_psfParameters.m_betaDegrees + 1 + 1 + 1;

			// Radial coordinates for computation
			result.m_radius = EigenTypes::ScalarRowVectorComputation::LinSpaced(result.m_maxSamples, 0.0f, result.m_maxExtent);

			// Defocus params for calculation
			result.m_defocusParams = EigenTypes::ScalarRowVectorComputation(stack.m_psfEntryParameters.num_elements());
			std::transform(psfStackBegin(scene, aberration), psfStackEnd(scene, aberration), result.m_defocusParams.begin(),
				[&](PsfIndex const& index) { return 0.5 * stack.m_psfEntryParameters(index).m_focus.m_defocusParam; });
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeEnzWklCoefficients(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, size_t coeffId)
		{
			// Coefficient ids
			const auto [n, m] = ZernikeIndices::single2doubleNoll(coeffId);
			long long p = (n - m) / 2, q = (n + m) / 2;

			if (m < 0) return; // Only do this for m >= 0

			// Loop through each order of k
			for (long long k = 0; k < stack.m_enzEntrySamplingParameters.m_maxOrder; ++k)
			{
				// Calculate w_kl for each l of this k
				Vnm::WklScalar wklSum = 0.0;
				for (long long l = glm::max(0ll, glm::max(k - q, p - k)), li = 0; l <= k + p; ++l, ++li)
				{
					const Vnm::WklScalar wkl = Vnm::wkl(n, m, p, q, k, l);
					if (wkl < -1e-4f) Debug::log_warning() << "wkl[" << n << ", " << m << ", " << k << ", " << l << "] < 0 (" << wkl << ")" << Debug::end;
					stack.m_enzWklCoefficients[n][m][k][li] = glm::pow(Vnm::WklScalar(-1.0), l) * wkl;
					wklSum += wkl; // Also append to the w_k sum to verify that sum(w_k) == 1
				}

				if (glm::abs(wklSum - 1.0) > 1e-3) 
					Debug::log_warning() << "Sum of wk[" << n << ", " << m << ", " << k << "] != 1 (" << wklSum << ")" << Debug::end;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadEnzWklCoefficientsGPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack)
		{
			// Extract the corresponding values
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;

			const size_t numPsfs = stack.m_psfEntryParameters.num_elements();
			const size_t numDegrees = enzSampling.m_maxDegree + 1;
			const size_t numCoeffs = enzSampling.m_maxCoefficients;
			const size_t numMaxVnmOrder = enzSampling.m_maxOrder;
			const size_t numMaxVnmSamples = enzSampling.m_maxSamples;
			const float vnmMaxExtent = enzSampling.m_maxExtent;
			const size_t maxTermsPerOrder = enzSampling.m_maxTermsPerOrder;
			const size_t maxTermOrder = enzSampling.m_maxTermOrder;

			// Wkl terms
			std::vector<float> wkls(numDegrees * numDegrees * numMaxVnmOrder * maxTermsPerOrder);
			for (size_t n = 0; n < numDegrees; ++n)
			for (size_t m = 0; m < numDegrees; ++m)
			for (size_t k = 0; k < numMaxVnmOrder; ++k)
			for (size_t l = 0; l < maxTermsPerOrder; ++l)
			{
				const size_t idx =
					n * numDegrees * numMaxVnmOrder * maxTermsPerOrder +
					m * numMaxVnmOrder * maxTermsPerOrder +
					k * maxTermsPerOrder +
					l;
				wkls[idx] = stack.m_enzWklCoefficients[n][m][k][l];
			}
			Scene::createGPUBuffer(scene, "PsfStack_WklBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_2, GL_DYNAMIC_STORAGE_BIT, wkls);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeEnzSphericalBesselSingle(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, size_t k)
		{
			// Compute the spherical Bessel terms
			stack.m_enzSphericalBessel[k] = stack.m_enzEntrySamplingParameters.m_defocusParams.unaryExpr([=](ScalarComputation d)
			{
				return ScalarComputation(Vnm::sph_bessel(k, d));
			});
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeEnzSphericalBesselsRange(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, int kMin, int kMax)
		{
			// Extract the defocus parameters
			EigenTypes::ScalarRowVectorComputation const& defocusParams = stack.m_enzEntrySamplingParameters.m_defocusParams;
			EigenTypes::ScalarRowVectorComputation const& invDefocusParams = defocusParams.cwiseInverse();

			// j_k vectors with all the values computed
			std::vector<EigenTypes::ScalarRowVectorComputation> jk(kMax - kMin + 2);

			// Indices into the vectors
			size_t i_kp1 = jk.size() - 1, i_k = i_kp1 - 1, i_km1 = i_k - 1;

			// Compute j_(k+1) and j_k to start
			jk[i_kp1] = defocusParams.unaryExpr([=](ScalarComputation d) { return Vnm::sph_bessel(kMax + 1, d); });
			jk[i_k] = defocusParams.unaryExpr([=](ScalarComputation d) { return Vnm::sph_bessel(kMax, d); });

			// Loop through all the orders backwards to compute them all
			for (int k = kMax; k >= kMin; --k)
			{
				stack.m_enzSphericalBessel[k] = jk[i_k];
				if (k > kMin) jk[i_km1] = jk[i_k].cwiseProduct((2 * k + 1) * invDefocusParams) - jk[i_kp1];
				--i_kp1; --i_k; --i_km1;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadEnzSphericalBesselGPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack)
		{
			// Extract the corresponding values
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;

			const size_t numPsfs = stack.m_psfEntryParameters.num_elements();
			const size_t numDegrees = enzSampling.m_maxDegree + 1;
			const size_t numCoeffs = enzSampling.m_maxCoefficients;
			const size_t numMaxVnmOrder = enzSampling.m_maxOrder;
			const size_t numMaxVnmSamples = enzSampling.m_maxSamples;
			const float vnmMaxExtent = enzSampling.m_maxExtent;
			const size_t maxTermsPerOrder = enzSampling.m_maxTermsPerOrder;
			const size_t maxTermOrder = enzSampling.m_maxTermOrder;

			// Upload the spherical Bessel coefficients
			std::vector<glm::vec2> sphericalBessels(numMaxVnmOrder * numPsfs);
			for (size_t k = 0; k < numMaxVnmOrder; ++k)
			for (size_t p = 0; p < numPsfs; ++p)
			{
				const ScalarComputation defocusParam = stack.m_psfEntryParameters.data()[p].m_focus.m_defocusParam;
				const ComplexComputation weight = 
					std::exp(ComplexComputation(0.0, 0.5 * defocusParam)) * 
					ScalarComputation(2 * k + 1) * 
					ComplexComputation(std::pow(1.0if, k));
				const ComplexComputation jk = weight * stack.m_enzSphericalBessel[k][p];
				//const size_t idx = p * numMaxVnmOrder + k;
				const size_t idx = k * numPsfs + p;
				sphericalBessels[idx] = glm::vec2(jk.real(), jk.imag());
			}
			Scene::createGPUBuffer(scene, "PsfStack_SphericalBesselBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_4, GL_DYNAMIC_STORAGE_BIT, sphericalBessels);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeEnzCylindricalBesselSingle(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, size_t K)
		{
			// Compute the corresponding cylindrical Bessel (J_n) vector
			stack.m_enzCylindricalBessel[K] = stack.m_enzEntrySamplingParameters.m_radius.unaryExpr([=](ScalarComputation r)
			{
				const ScalarComputation rp = r * glm::two_pi<ScalarComputation>();
				return rp < 1e-6f ? (K == 0 ? 1.0f : 0.0f ) : ScalarComputation(Vnm::cyl_bessel(K, rp) / rp);
			});
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeEnzCylindricalBesselsRange(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, int nMin, int nMax)
		{
			// Scaled radius vector
			EigenTypes::ScalarRowVectorComputation const& radii = stack.m_enzEntrySamplingParameters.m_radius * glm::two_pi<ScalarComputation>();
			EigenTypes::ScalarRowVectorComputation const& invRadii = radii.cwiseInverse();

			// J_n vectors with all the values computed
			std::vector<EigenTypes::ScalarRowVectorComputation> Jn(nMax - nMin + 2);

			// Indices into the vectors
			size_t i_np1 = Jn.size() - 1, i_n = i_np1 - 1, i_nm1 = i_n - 1;

			// Compute J_(n+1) and J_n to start
			Jn[i_np1] = radii.unaryExpr([=](ScalarComputation r){ return Vnm::cyl_bessel(nMax + 1, r); });
			Jn[i_n] = radii.unaryExpr([=](ScalarComputation r) { return Vnm::cyl_bessel(nMax, r); });

			// Loop through all the orders backwards to compute them all
			for (int n = nMax; n >= nMin; --n)
			{
				stack.m_enzCylindricalBessel[n] = Jn[i_n].cwiseProduct(invRadii);
				stack.m_enzCylindricalBessel[n][0] = n > 0 ? ScalarComputation(0.0) : ScalarComputation(1.0);
				if (n > nMin) Jn[i_nm1] = Jn[i_n].cwiseProduct((2 * n) * invRadii) - Jn[i_np1];
				--i_np1; --i_n; --i_nm1;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadEnzCylindricalBesselGPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack)
		{
			// Extract the corresponding values
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;

			const size_t numPsfs = stack.m_psfEntryParameters.num_elements();
			const size_t numDegrees = enzSampling.m_maxDegree + 1;
			const size_t numCoeffs = enzSampling.m_maxCoefficients;
			const size_t numMaxVnmOrder = enzSampling.m_maxOrder;
			const size_t numMaxVnmSamples = enzSampling.m_maxSamples;
			const float vnmMaxExtent = enzSampling.m_maxExtent;
			const size_t maxTermsPerOrder = enzSampling.m_maxTermsPerOrder;
			const size_t maxTermOrder = enzSampling.m_maxTermOrder;

			// Upload the cylindrical Bessel coefficients
			std::vector<float> cylindricalBessels(maxTermOrder * numMaxVnmSamples);
			for (size_t k = 0; k < maxTermOrder; ++k)
			for (size_t s = 0; s < numMaxVnmSamples; ++s)
			{
				const size_t idx = k * numMaxVnmSamples + s;
				cylindricalBessels[idx] = stack.m_enzCylindricalBessel[k][s];
			}
			Scene::createGPUBuffer(scene, "PsfStack_CylindricalBesselBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1, GL_DYNAMIC_STORAGE_BIT, cylindricalBessels);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeInnerVnmTermsCPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, size_t coeffId, size_t k)
		{
			// Coefficient ids
			const auto [n, m] = ZernikeIndices::single2doubleNoll(coeffId);
			const long long am = glm::abs(m);
			const long long p = (n - m) / 2, q = (n + m) / 2;

			if (m < 0) return; // Only do this for m >= 0

			// Compute and store the resulting vector
			stack.m_enzVnmInnerTerms[n][am][k] = Vnm::computeVnmInnerTerms(scene, aberration, stack, 
				n, m, am, k, p, q, stack.m_enzEntrySamplingParameters.m_maxSamples);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeInnerVnmTermsGPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack)
		{
			// Extract the corresponding values
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;

			const size_t numDegrees = enzSampling.m_maxDegree + 1;
			const size_t numCoeffs = enzSampling.m_maxCoefficients;
			const size_t numMaxVnmOrder = enzSampling.m_maxOrder;
			const size_t numMaxVnmSamples = enzSampling.m_maxSamples;
			const float vnmMaxExtent = enzSampling.m_maxExtent;
			const size_t maxTermsPerOrder = enzSampling.m_maxTermsPerOrder;
			const size_t maxTermOrder = enzSampling.m_maxTermOrder;

			// Vnm inner term buffer
			Scene::createGPUBuffer(scene, "PsfStack_VnmInnerBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_3, 0,
				numCoeffs * numMaxVnmOrder * numMaxVnmSamples * sizeof(GLfloat));

			Scene::bindBuffer(scene, "PsfStack_WklBuffer");
			Scene::bindBuffer(scene, "PsfStack_SphericalBesselBuffer");
			Scene::bindBuffer(scene, "PsfStack_CylindricalBesselBuffer");
			Scene::bindBuffer(scene, "PsfStack_VnmInnerBuffer");

			// Bind the corresponding shader
			Scene::bindShader(scene, "PsfStack/compute_vnm_inner");

			// Generate it for each coefficient
			for (size_t coeffId = 0; coeffId < numCoeffs; ++coeffId)
			{
				// Compute the corresponding n and m terms
				const ZernikePositiveCoeff coeff = zernikeCoeffPositiveOnly(coeffId);
				const int n = coeff.m_degrees[0][0], m = coeff.m_degrees[0][1], am = abs(m);
				const int p = (n - m) / 2, q = (n + m) / 2;
				const int wklOffset = (n * numDegrees * numMaxVnmOrder * maxTermsPerOrder) + m * numMaxVnmOrder * maxTermsPerOrder;

				// Upload the necessary uniforms
				glUniform1i(0, numCoeffs);
				glUniform1i(1, numMaxVnmOrder);
				glUniform1i(2, numMaxVnmSamples);
				glUniform1i(3, maxTermsPerOrder);
				glUniform1i(4, wklOffset);
				glUniform1i(5, coeffId);
				glUniform1i(6, n);
				glUniform1i(7, m);
				glUniform1i(8, p);
				glUniform1i(9, q);

				// Dispatch the compute shader
				const glm::uvec3 groupSize = glm::uvec3(32, 32, 1);
				const glm::uvec3 numWorkItems = glm::uvec3(numMaxVnmSamples, numMaxVnmOrder, 1);
				const glm::uvec3 numGroups = (numWorkItems + groupSize - glm::uvec3(1)) / groupSize;
				glDispatchCompute(numGroups.x, numGroups.y, numGroups.z);
			}

			// Make sure the wkl * jk cache is visible inside the PSF calculation
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// Submit the work immediately
			glFlush();

			Scene::waitForGpu(scene);
		}

		////////////////////////////////////////////////////////////////////////////////
		Psf optimizePsf(Scene::Scene& scene, WavefrontAberration& aberration, Psf psf)
		{
			// Crop the PSF
			if (aberration.m_psfParameters.m_cropThresholdCoeff > 0.0f)
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Crop (Coeff)", false, Threading::currentThreadId());
				psf = PsfOptimization::cropCoeff(psf, aberration.m_psfParameters.m_cropThresholdCoeff * psf.maxCoeff());
			}

			if (aberration.m_psfParameters.m_cropThresholdSum < 1.0f)
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Crop (Sum)", false, Threading::currentThreadId());
				psf = PsfOptimization::cropSum(psf, aberration.m_psfParameters.m_cropThresholdSum * psf.sum());
			}

			// Normalize the PSF
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Normalization", false, Threading::currentThreadId());
				psf = PsfOptimization::normalize(psf);
			}

			return psf;
		}

		////////////////////////////////////////////////////////////////////////////////
		void makePolarGrid(PsfStackElements::PsfEntryParams const& psfParameters,
			EigenTypes::ScalarMatrixComputation& rPupil, EigenTypes::ScalarMatrixComputation& phiPupil)
		{
			// Cartesian coordinates
			EigenTypes::ScalarRowVectorComputation radius = EigenTypes::ScalarRowVectorComputation::LinSpaced(
				psfParameters.m_sampling.m_samples, -psfParameters.m_sampling.m_halfExtent, psfParameters.m_sampling.m_halfExtent);
			EigenTypes::ScalarMatrixComputation xPupil = radius.replicate(psfParameters.m_sampling.m_samples, 1);
			EigenTypes::ScalarMatrixComputation yPupil = xPupil.transpose();

			// Radial coordinates
			rPupil = (xPupil.cwiseProduct(xPupil) + yPupil.cwiseProduct(yPupil)).cwiseSqrt();
			phiPupil = yPupil.binaryExpr(xPupil, [](ScalarComputation y, ScalarComputation x) { return glm::atan(y, x); });
		}

		////////////////////////////////////////////////////////////////////////////////
		void computePsfCPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, PsfIndex const& psfIndex)
		{
			// Extract the corresponding values
			const float planeDistance = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances[psfIndex[0]];
			const float horizontalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal[psfIndex[1]];
			const float verticalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical[psfIndex[2]];
			const float lambdaNM = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[psfIndex[3]];
			const float apertureDiameterMM = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[psfIndex[4]];
			const float focusDistance = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances[psfIndex[5]];
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;
			PsfStackElements::PsfEntryParams& psfParameters = stack.m_psfEntryParameters(psfIndex);
			PsfStackElements::PsfEntry& result = stack.m_psfs(psfIndex);
			const size_t psfId = std::distance(stack.m_psfEntryParameters.data(), &psfParameters);
			const ScalarComputation defocusParam = psfParameters.m_focus.m_defocusParam;

			Profiler::ScopedCpuPerfCounter perfCounter(scene,
				"PSF Distance: " + std::to_string(planeDistance) + "m, " +
				"Horizontal: " + std::to_string(glm::degrees(horizontalAngle)) + "deg, " +
				"Vertical: " + std::to_string(glm::degrees(verticalAngle)) + "deg, " +
				"Wavelength: " + std::to_string(lambdaNM) + "nm, " +
				"Aperture Diameter: " + std::to_string(apertureDiameterMM) + "mm, " +
				"Focus Distance: " + std::to_string(focusDistance) + "m",
				false, Threading::currentThreadId());


			// Polar coordinates
			EigenTypes::ScalarMatrixComputation rPupil, phiPupil;
			makePolarGrid(psfParameters, rPupil, phiPupil);

			// Resulting complex pupil function
			EigenTypes::ComplexMatrixComputation U = EigenTypes::ComplexMatrixComputation::Zero(rPupil.rows(), rPupil.cols());

			// Evaluate the coefficients
			for (size_t coeffId = 1; coeffId < psfParameters.m_coefficients.m_beta.size(); ++coeffId)
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Z[" + std::to_string(coeffId) + "]", false, Threading::currentThreadId());

				// Extract the ith coefficient
				const ComplexZernikeCoeffs coeff = psfParameters.m_coefficients.m_beta[coeffId];
				const auto [_n, _m] = ZernikeIndices::single2doubleNoll(coeffId);
				const int n = _n, m = _m, am = std::abs(m); // workaround for "structured binding cannot be captured" in the lambda below

				// Interpolate the Vnm terms for the whole pupil
				EigenTypes::ComplexMatrixComputation vnmPupil = EigenTypes::ComplexMatrixComputation::Zero(rPupil.rows(), rPupil.cols());
				if (!aberration.m_psfParameters.m_omitVnmCalculation)
				{
					vnmPupil = Vnm::computeVnm(scene, aberration, stack, psfId, coeffId, psfParameters, rPupil);
				}

				// Angular term
				if (!aberration.m_psfParameters.m_omitPsfCalculation)
				{
					EigenTypes::ComplexMatrixComputation Z = phiPupil.cast<ComplexComputation>().unaryExpr(
						[=](ComplexComputation phi) { return std::exp(ComplexComputation(0.0, m * phi.real())); });

					// Beta coefficient
					const ComplexComputation beta =
						ScalarComputation(2.0) *
						glm::sqrt(ScalarComputation(n + 1.0)) *
						std::pow(ComplexComputation(0, 1), glm::abs(m)) *
						ComplexComputation(coeff);

					// Accumulate
					U += beta * vnmPupil.cwiseProduct(Z);
				}
			}

			// Write out the PSF and its properties
			if (!aberration.m_psfParameters.m_omitPsfCalculation)
			{
				result.m_psf = optimizePsf(scene, aberration, U.cwiseAbs2().cast<ScalarFinal>());
			}
			else
			{
				result.m_psf = U.cwiseAbs2().cast<ScalarFinal>();
			}
			result.m_kernelSizePx = result.m_psf.cols();
			result.m_blurRadiusMuM = (result.m_kernelSizePx / 2) * psfParameters.m_sampling.m_samplingMuM;
			result.m_blurRadiusDeg = blurRadiusAngle(result.m_blurRadiusMuM);
			result.m_blurSizeMuM = result.m_kernelSizePx * psfParameters.m_sampling.m_samplingMuM;
			result.m_blurSizeDeg = blurRadiusAngle(result.m_blurSizeMuM);
		}

		////////////////////////////////////////////////////////////////////////////////
		void initPsfComputationGpu(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& stack)
		{
			// Extract the corresponding values
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;

			// Various counts
			const size_t numPsfs = stack.m_psfEntryParameters.num_elements();
			const size_t numDegrees = enzSampling.m_maxDegree + 1;
			const size_t numCoeffs = enzSampling.m_maxCoefficients;
			const size_t numMaxVnmOrder = enzSampling.m_maxOrder;
			const size_t numMaxVnmSamples = enzSampling.m_maxSamples;
			const float vnmMaxExtent = enzSampling.m_maxExtent;
			const size_t maxTermsPerOrder = enzSampling.m_maxTermsPerOrder;
			const size_t maxTermOrder = enzSampling.m_maxTermOrder;

			// Vnm texture
			Scene::createTexture(scene, "PsfStack_VnmTexture", GL_TEXTURE_2D,
				(int)numMaxVnmSamples, (int)numCoeffs, 1, s_vnmTextureFormat, GL_RG,
				GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);

			// Upload the PSF properties and betas
			struct alignas(sizeof(glm::vec2)) PsfProperties
			{
				GLint m_numCoefficients = 0;
				GLint m_numTotalCoefficients = 0;
				GLint m_numPsfSamples = 0;
				GLfloat m_psfTexelSize = 0.0f;
				GLfloat m_psfHalfExtent = 0.0f;
				alignas(sizeof(glm::vec2)) glm::vec2 m_vnmUvOffset{ 0.0f };
				alignas(sizeof(glm::vec2)) glm::vec2 m_vnmUvScale{ 0.0f };
			};
			std::vector<PsfProperties> psfProperties(numPsfs);

			forEachPsfStackIndex(scene, aberration,
				[&](Scene::Scene& scene, WavefrontAberration& aberration, PsfIndex const& psfIndex)
				{
					// Various sampling parameters
					PsfStackElements::PsfEntryParams& psfParameters = stack.m_psfEntryParameters(psfIndex);
					const size_t psfId = std::distance(stack.m_psfEntryParameters.data(), &psfParameters);

					// Sampling properties
					const float enzSamples = psfParameters.m_enzSampling.m_samples;
					const float enzExtent = psfParameters.m_enzSampling.m_extent;
					const size_t numPsfSamples = psfParameters.m_sampling.m_samples;
					const float psfHalfExtent = psfParameters.m_sampling.m_halfExtent;

					// Write out the properties
					PsfProperties& result = psfProperties[psfId];
					result.m_numCoefficients = 0;
					result.m_numTotalCoefficients = numCoeffs;
					result.m_numPsfSamples = numPsfSamples;
					result.m_psfTexelSize = (2.0f * psfHalfExtent) / (numPsfSamples - 1);
					result.m_psfHalfExtent = psfHalfExtent;
					result.m_vnmUvOffset = glm::vec2(0.5f) / glm::vec2(numMaxVnmSamples, numCoeffs);
					result.m_vnmUvScale = glm::vec2((1.0f / vnmMaxExtent), 1.0f / numCoeffs);
				});

			// Upload the PSF beta coefficients
			struct PsfBeta
			{
				GLint m_coeffId = 0;
				GLint m_n = 0;
				GLint m_m[2] = { 0, 0 };
				glm::vec2 m_beta[2] = { glm::vec2(0.0f), glm::vec2(0.0f) };
			};
			std::vector<PsfBeta> psfBetaCoefficients(numPsfs * numCoeffs);

			// Go through each individual coefficient and upload it
			int n = 0, m = 0;
			for (size_t coeffId = 0; coeffId < numCoeffs; ++coeffId)
			{
				// Beta coefficient indices and corresponding noll ID's
				const ZernikePositiveCoeff coeff = zernikeCoeffPositiveOnly(coeffId);
				const glm::ivec2 degrees[2] = { coeff.m_degrees[0], coeff.m_degrees[1] };
				const size_t betaIndices[2] = { coeff.m_noll[0], coeff.m_noll[1] };

				// Go through each PSF and upload the corresponding coefficients
				for (size_t psfId = 0; psfId < numPsfs; ++psfId)
				{
					// PSF coefficients
					ZernikeCoefficientsBeta const& coefficients = stack.m_psfEntryParameters.data()[psfId].m_coefficients.m_beta;
					const ComplexZernikeCoeffs betas[2] = { coefficients[betaIndices[0]], coefficients[betaIndices[1]] };

					// Skip invalid coefficients
					if (std::max(std::abs(betas[0]), std::abs(betas[1])) < aberration.m_psfParameters.m_betaThreshold)
						continue;

					// Resulting properties
					PsfProperties& properties = psfProperties[psfId];
					PsfBeta& result = psfBetaCoefficients[psfId * numCoeffs + properties.m_numCoefficients];
					++properties.m_numCoefficients;

					for (int k = 0; k < (m == 0 ? 1 : 2); ++k)
					{
						// Computed the total beta term
						const ComplexComputation beta =
							ScalarComputation(2.0) *
							glm::sqrt(ScalarComputation(n + 1.0)) *
							std::pow(ComplexComputation(0, 1), m) *
							ComplexComputation(betas[k]);

						// Store the result
						result.m_coeffId = coeffId;
						result.m_n = degrees[k][0];
						result.m_m[k] = degrees[k][1];
						result.m_beta[k] = glm::vec2(beta.real(), beta.imag());
					}
				}

				// Jump to the next coefficient
				if (n == m) { ++n; m = n % 2; }
				else { m += 2; }
			}

			Scene::createGPUBuffer(scene, "PsfStack_PsfPropertiesBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_5, GL_DYNAMIC_STORAGE_BIT, psfProperties);

			Scene::createGPUBuffer(scene, "PsfStack_PsfBetaBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_6, GL_DYNAMIC_STORAGE_BIT, psfBetaCoefficients);

			// Figure out the maximum PSF size
			const int maxPsfSize = std::accumulate(stack.m_psfEntryParameters.data(), stack.m_psfEntryParameters.data() + numPsfs, 0,
				[](int maxSize, auto const& psfParams) { return glm::max(maxSize, psfParams.m_sampling.m_samples); });

			// Resulting buffer
			Scene::createGPUBuffer(scene, "PsfStack_PsfResultBuffer", GL_SHADER_STORAGE_BUFFER, true, true,
				GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_7, GL_MAP_READ_BIT | GL_CLIENT_STORAGE_BIT,
				maxPsfSize * maxPsfSize * sizeof(GLfloat));

			// Bind the buffers
			// - Vnm
			Scene::bindBuffer(scene, "PsfStack_WklBuffer");
			Scene::bindBuffer(scene, "PsfStack_SphericalBesselBuffer");
			Scene::bindBuffer(scene, "PsfStack_CylindricalBesselBuffer");
			Scene::bindBuffer(scene, "PsfStack_VnmInnerBuffer");
			// - PSF
			Scene::bindBuffer(scene, "PsfStack_PsfPropertiesBuffer");
			Scene::bindBuffer(scene, "PsfStack_PsfBetaBuffer");
			Scene::bindBuffer(scene, "PsfStack_PsfResultBuffer");

			// Bind the Vnm texture
			glBindImageTexture(0, scene.m_textures["PsfStack_VnmTexture"].m_texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, s_vnmTextureFormat);
			Scene::bindTexture(scene, "PsfStack_VnmTexture");

			Scene::waitForGpu(scene);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computePsfGPU(Scene::Scene& scene, WavefrontAberration& aberration, PSFStack& stack, PsfIndex const& psfIndex)
		{
			// Extract the corresponding values
			const float planeDistance = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances[psfIndex[0]];
			const float horizontalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal[psfIndex[1]];
			const float verticalAngle = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical[psfIndex[2]];
			const float lambdaNM = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas[psfIndex[3]];
			const float apertureDiameterMM = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters[psfIndex[4]];
			const float focusDistance = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances[psfIndex[5]];
			PsfStackElements::EnzEntrySamplingParameters& enzSampling = stack.m_enzEntrySamplingParameters;
			PsfStackElements::PsfEntryParams& psfParameters = stack.m_psfEntryParameters(psfIndex);
			PsfStackElements::PsfEntry& result = stack.m_psfs(psfIndex);
			const size_t psfId = std::distance(stack.m_psfEntryParameters.data(), &psfParameters);
			const ScalarComputation defocusParam = psfParameters.m_focus.m_defocusParam;

			// Start by calculating the Vnm coefficients
			if (aberration.m_psfParameters.m_precomputeVnmLSum)
				Scene::bindShader(scene, "PsfStack/compute_vnm_with_cache");
			else
				Scene::bindShader(scene, "PsfStack/compute_vnm_no_cache");

			// Various counts
			const size_t numPsfs = stack.m_psfEntryParameters.num_elements();
			const size_t numDegrees = enzSampling.m_maxDegree + 1;
			const size_t numCoeffs = enzSampling.m_maxCoefficients;
			const size_t numOrder = psfParameters.m_enzSampling.m_terms;
			const size_t numSamples = psfParameters.m_enzSampling.m_samples;
			const size_t numMaxOrder = enzSampling.m_maxOrder;
			const size_t numMaxSamples = enzSampling.m_maxSamples;
			const float vnmMaxExtent = enzSampling.m_maxExtent;
			const size_t maxTermsPerOrder = enzSampling.m_maxTermsPerOrder;
			const size_t maxTermOrder = enzSampling.m_maxTermOrder;
			
			for (size_t coeffId = 0; coeffId < numCoeffs; ++coeffId)
			{
				// Compute the corresponding n and m terms
				const ZernikePositiveCoeff coeff = zernikeCoeffPositiveOnly(coeffId);
				const int n = coeff.m_degrees[0][0], m = coeff.m_degrees[0][1], am = abs(m);
				const int p = (n - m) / 2, q = (n + m) / 2;
				const int wklOffset = (n * numDegrees * numMaxOrder * maxTermsPerOrder) + m * numMaxOrder * maxTermsPerOrder;

				// Make sure the coeff is relevant
				auto const& beta = stack.m_psfEntryParameters.data()[psfId].m_coefficients.m_beta;
				const ComplexZernikeCoeffs betas[2] = { beta[coeff.m_noll[0]], beta[coeff.m_noll[1]] };

				// Skip coeffs below the beta threshold
				if (std::max(std::abs(betas[0]), std::abs(betas[1])) < aberration.m_psfParameters.m_betaThreshold)
					continue;

				// Upload the necessary uniforms
				glUniform1i(0, numPsfs);
				glUniform1i(1, numCoeffs);
				glUniform1i(2, numOrder);
				glUniform1i(3, numSamples);
				glUniform1i(4, numMaxOrder);
				glUniform1i(5, numMaxSamples);
				glUniform1i(6, maxTermsPerOrder);
				glUniform1i(7, wklOffset);
				glUniform1i(8, psfId);
				glUniform1i(9, coeffId);
				glUniform1i(10, n);
				glUniform1i(11, m);
				glUniform1i(12, p);
				glUniform1i(13, q);

				// Dispatch the compute shader
				const glm::uvec3 groupSize = glm::uvec3(256, 1, 1);
				const glm::uvec3 numWorkItems = glm::uvec3(numMaxSamples, 1, 1);
				const glm::uvec3 numGroups = (numWorkItems + groupSize - glm::uvec3(1)) / groupSize;
				if (!aberration.m_psfParameters.m_omitVnmCalculation)
					glDispatchCompute(numGroups.x, numGroups.y, numGroups.z);
			}

			// Make sure the texture is updated before reading it
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

			// Bind the PSF computation shader
			Scene::bindShader(scene, "PsfStack/compute_psf");
			
			// Upload the necessary uniforms
			glUniform1i(0, psfId);

			// Dispatch the compute shader
			const size_t numPsfSamples = psfParameters.m_sampling.m_samples;
			const glm::uvec3 groupSize = glm::uvec3(8, 8, 1);
			const glm::uvec3 psfSize = glm::uvec3(numPsfSamples, numPsfSamples, 1);
			const glm::uvec3 numGroups = (psfSize + groupSize - glm::uvec3(1)) / groupSize;
			if (!aberration.m_psfParameters.m_omitPsfCalculation)
				glDispatchCompute(numGroups.x, numGroups.y, numGroups.z);

			// Make sure the texture is updated before reading it
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

			// Submit the work immediately
			glFlush();

			// Write out the PSF
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene.m_genericBuffers["PsfStack_PsfResultBuffer"].m_buffer);
			float* data = (float*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			result.m_psf = optimizePsf(scene, aberration, PsfGpu::Map(data, numPsfSamples, numPsfSamples));
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			// Also compute and store the kernel sizes
			result.m_kernelSizePx = result.m_psf.cols();
			result.m_blurRadiusMuM = (result.m_kernelSizePx / 2) * psfParameters.m_sampling.m_samplingMuM;
			result.m_blurRadiusDeg = blurRadiusAngle(result.m_blurRadiusMuM);
			result.m_blurSizeMuM = result.m_kernelSizePx * psfParameters.m_sampling.m_samplingMuM;
			result.m_blurSizeDeg = blurRadiusAngle(result.m_blurSizeMuM);
		}

		////////////////////////////////////////////////////////////////////////////////
		void finishPsfComputationGpu(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& stack)
		{
			// Release the unneeded GPU resources
			Scene::deleteGPUBuffer(scene, "PsfStack_PsfPropertiesBuffer");
			Scene::deleteGPUBuffer(scene, "PsfStack_PsfBetaBuffer");
			Scene::deleteGPUBuffer(scene, "PsfStack_PsfResultBuffer");
			Scene::deleteTexture(scene, "PsfStack_VnmTexture");
		}
	
		////////////////////////////////////////////////////////////////////////////////
		void phaseGPUSync(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			auto timer = result.m_timers.startComputation("GPU Sync", 1);

			if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
				Scene::waitForGpu(scene);
		}

		////////////////////////////////////////////////////////////////////////////////
		void phaseParamRanges(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Parameter Range Evaluation");

			// Compute the actual coefficients
			aberration.m_aberrationParameters.m_coefficients = Coefficients::getZernikeCoefficients(aberration);

			// Update the parameter vectors
			aberration.m_psfParameters.m_objectDistances.m_step = Ranges::stepSize(aberration.m_psfParameters.m_objectDistances);
			aberration.m_psfParameters.m_incidentAnglesHorizontal.m_step = Ranges::stepSize(aberration.m_psfParameters.m_incidentAnglesHorizontal);
			aberration.m_psfParameters.m_incidentAnglesVertical.m_step = Ranges::stepSize(aberration.m_psfParameters.m_incidentAnglesVertical);
			aberration.m_psfParameters.m_apertureDiameters.m_step = Ranges::stepSize(aberration.m_psfParameters.m_apertureDiameters);
			aberration.m_psfParameters.m_focusDistances.m_step = Ranges::stepSize(aberration.m_psfParameters.m_focusDistances);

			// Store the original ranges
			aberration.m_psfParameters.m_evaluatedParameters.m_objectDistancesRange = aberration.m_psfParameters.m_objectDistances;
			aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontalRange = aberration.m_psfParameters.m_incidentAnglesHorizontal;
			aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVerticalRange = aberration.m_psfParameters.m_incidentAnglesVertical;
			aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiametersRange = aberration.m_psfParameters.m_apertureDiameters;
			aberration.m_psfParameters.m_evaluatedParameters.m_focusDistancesRange = aberration.m_psfParameters.m_focusDistances;

			// Update the parameter vectors
			aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances = generateObjectDistances(scene, aberration);
			aberration.m_psfParameters.m_evaluatedParameters.m_objectDioptres = generateObjectDioptres(scene, aberration);
			aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal = generateHorizontalAxes(scene, aberration);
			aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical = generateVerticalAxes(scene, aberration);
			aberration.m_psfParameters.m_evaluatedParameters.m_lambdas = aberration.m_psfParameters.m_lambdas;
			aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters = generateApertureDiameters(scene, aberration);
			aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances = generateFocusDistances(scene, aberration);
			aberration.m_psfParameters.m_evaluatedParameters.m_focusDioptres = generateFocusDioptres(scene, aberration);

			// Print out the source and generated parameter ranges
			if (aberration.m_psfParameters.m_logDebug)
			{
				Debug::log_debug() << std::string(80, '=') << Debug::end;
				Debug::log_debug() << "Parameter Ranges" << Debug::end;
				Debug::log_debug() << std::string(80, '-') << Debug::end;
				Debug::log_debug() << "Object Distances: " << aberration.m_psfParameters.m_objectDistances << Debug::end;
				Debug::log_debug() << "Incident Angles (Horizontal): " << aberration.m_psfParameters.m_incidentAnglesHorizontal << Debug::end;
				Debug::log_debug() << "Incident Angles (Vertical): " << aberration.m_psfParameters.m_incidentAnglesVertical << Debug::end;
				Debug::log_debug() << "Aperture Diameters: " << aberration.m_psfParameters.m_apertureDiameters << Debug::end;
				Debug::log_debug() << "Focus Distances (Near): " << aberration.m_psfParameters.m_focusDistances << Debug::end;
				Debug::log_debug() << "Lambdas: " << aberration.m_psfParameters.m_lambdas << Debug::end;
				Debug::log_debug() << std::string(80, '=') << Debug::end;
				Debug::log_debug() << "Evaluated Parameters" << Debug::end;
				Debug::log_debug() << std::string(80, '-') << Debug::end;
				Debug::log_debug() << "Defocus Depths: " << aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances << Debug::end;
				Debug::log_debug() << "Incident Angles (Horizontal): " << aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal << Debug::end;
				Debug::log_debug() << "Incident Angles (Vertical): " << aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical << Debug::end;
				Debug::log_debug() << "Aperture Diameters: " << aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters << Debug::end;
				Debug::log_debug() << "Focus Distances: " << aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances << Debug::end;
				Debug::log_debug() << "Focus Dioptres: " << aberration.m_psfParameters.m_evaluatedParameters.m_focusDioptres << Debug::end;
				Debug::log_debug() << std::string(80, '=') << Debug::end;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void phaseEyeParameters(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Eye Parameters");

			if (aberration.m_psfParameters.m_logProgress)
				Debug::log_info() << "Computing eye parameters..." << Debug::end;

			Debug::log_trace() << "Eye parameters" << Debug::end;

			auto timer = result.m_timers.startComputation("Eye Parameters", 1);

			// Total number of PSFs
			const size_t numDefocues = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size();
			const size_t numAnglesHor = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size();
			const size_t numAnglesVert = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size();
			const size_t numLambdas = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size();
			const size_t numApertures = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size();
			const size_t numFocuses = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size();
			const size_t numPsfs = numDefocues * numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numCoefficients = numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;

			// Clear the arrays
			{
				if (computation & PsfStackComputation_FocusedEyeParameters)
				{
					result.m_focusedEyeParameters.resize(boost::extents[0][0]);
				}
				if (computation & PsfStackComputation_AberrationCoefficients)
				{
					result.m_aberrationCoefficients.resize(boost::extents[0][0][0][0][0]);
				}
			}

			// Resize the coefficient array
			result.m_aberrationCoefficients.resize(boost::extents
				[aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size()]
			);

			// Resize the focused eye parameter array
			result.m_focusedEyeParameters.resize(boost::extents
				[aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size()]
			);

			if (aberration.m_psfParameters.m_manualCoefficients)
			{
				// Write out the desired coefficients
				for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
				for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
				for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
				for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
				for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
				{
					auto& coeffVariants = result.m_aberrationCoefficients[h][v][l][a][f];
					for (size_t c = 1; c < coeffVariants.m_alpha.size(); ++c)
					{
						coeffVariants.m_alpha[c] = aberration.m_psfParameters.m_desiredCoefficients[c];
					}

					// Store the desired pupil-retina distance
					result.m_focusedEyeParameters[a][f].m_pupilRetinaDistance = aberration.m_psfParameters.m_desiredPupilRetinaDistance;
				}
			}

			else
			{
				// Compute the alpha coefficients
				switch (aberration.m_psfParameters.m_eyeEstimationMethod)
				{
				case PSFStackParameters::Matlab:
					#ifdef HAS_Matlab
					if (Matlab::matlabEnabled()) computeEyeParametersMatlab(scene, aberration, result);
					#endif
					break;

				case PSFStackParameters::NeuralNetworks:
					computeEyeParametersNeuralNetwork(scene, aberration, computation, result);
					break;
				}
			}

			// Compute the beta coefficients now
			if (computation & PsfStackComputation_AberrationCoefficients)
			{
				computeBetaCoefficients(scene, aberration, result);
			}

			// Log the corresponding stats
			if (aberration.m_psfParameters.m_logDebug)
			{
				// Log the initial coefficient array
				Debug::log_debug() << "Coefficient array: " << aberration.m_aberrationParameters.m_coefficients << Debug::end;

				// Log the aberration coefficientss
				for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
				for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
				for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
				for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
				for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
				{
					Debug::log_debug() << "Alpha[" << h << "][" << v << "][" << l << "][" << a << "][" << f << "]: " << result.m_aberrationCoefficients[h][v][l][a][f].m_alpha << Debug::end;
				}

				// Log the aberration coefficientss
				for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
				for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
				for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
				for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
				for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
				{
					Debug::log_debug() << "Alpha Phase[" << h << "][" << v << "][" << l << "][" << a << "][" << f << "]: " << result.m_aberrationCoefficients[h][v][l][a][f].m_alphaPhaseCumulative << Debug::end;
				}

				// Log the aberration coefficientss
				for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
				for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
				for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
				for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
				for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
				{
					Debug::log_debug() << "Alpha Phase Norm[" << h << "][" << v << "][" << l << "][" << a << "][" << f << "]: " << result.m_aberrationCoefficients[h][v][l][a][f].m_alphaPhaseResidual << Debug::end;
				}

				for (size_t h = 0; h < result.m_aberrationCoefficients.size(); ++h)
				for (size_t v = 0; v < result.m_aberrationCoefficients[0].size(); ++v)
				for (size_t l = 0; l < result.m_aberrationCoefficients[0][0].size(); ++l)
				for (size_t a = 0; a < result.m_aberrationCoefficients[0][0][0].size(); ++a)
				for (size_t f = 0; f < result.m_aberrationCoefficients[0][0][0][0].size(); ++f)
				{
					Debug::log_debug() << "Beta[" << h << "][" << v << "][" << l << "][" << a << "][" << f << "]: " << result.m_aberrationCoefficients[h][v][l][a][f].m_beta << Debug::end;
				}

				// Log the eye parameters
				for (size_t a = 0; a < result.m_focusedEyeParameters.size(); ++a)
				for (size_t f = 0; f < result.m_focusedEyeParameters[0].size(); ++f)
				{
					Debug::log_debug() << "Focused eye parameters[" << a << "][" << f << "] - "
						<< "Pupil-retina Distance: " << result.m_focusedEyeParameters[a][f].m_pupilRetinaDistance << Debug::end;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void phasePsfParameters(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Parameters");

			if (aberration.m_psfParameters.m_logProgress)
				Debug::log_info() << "Computing PSF parameters..." << Debug::end;

			auto timer = result.m_timers.startComputation("PSF Parameters", 1);

			// Total number of PSFs
			const size_t numDefocues = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size();
			const size_t numAnglesHor = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size();
			const size_t numAnglesVert = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size();
			const size_t numLambdas = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size();
			const size_t numApertures = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size();
			const size_t numFocuses = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size();
			const size_t numPsfs = numDefocues * numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numCoefficients = numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;

			// Compute the PSF entry parameters
			if (computation & PsfStackComputation_PsfUnits)
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "PSF Parameters");

				Debug::log_trace() << "PSF Parameters" << Debug::end;

				// Clear the stack first
				result.m_psfEntryParameters.resize(boost::extents[0][0][0][0][0][0]);

				// Allocate space for the PSF entry parameters
				result.m_psfEntryParameters.resize(boost::extents
					[aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size()]
					[aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size()]
					[aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size()]
					[aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size()]
					[aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size()]
					[aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size()]
				);

				// Compute the PSF entry parameters
				{
					auto timer = result.m_timers.startComputation("PSF Parameters", numPsfs);

					Threading::threadedExecuteIndices(
						Threading::ThreadedExecuteParams(Threading::numThreads(), " > PSF parameters", "PSF", progressLogLevel(scene, aberration)),
						[&](Threading::ThreadedExecuteEnvironment const& environment, size_t defocusId, size_t horizontalId, size_t verticalId, size_t lambdaId, size_t apertureId, size_t focusId)
						{
							computePsfEntryParameters(scene, aberration, result, defocusId, horizontalId, verticalId, lambdaId, apertureId, focusId);
						},
						aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size());
				}

				// Log the PSF sizes for reference
				if (aberration.m_psfParameters.m_logDebug)
				{
					Debug::log_debug() << std::string(80, '=') << Debug::end;
					Debug::log_debug() << "PSF Parameters:" << Debug::end;
					Debug::log_debug() << std::string(80, '-') << Debug::end;

					forEachPsfStackIndex(scene, aberration,
						[&](Scene::Scene& scene, WavefrontAberration& aberration, PsfIndex const& psfIndex)
						{
							auto const& psfParameters = getPsfEntryParameters(scene, aberration, psfIndex);
							Debug::log_debug() << "PSF" << psfIndex << ":" << Debug::end;
							Debug::log_debug() << " > defocus: " << psfParameters.m_focus.m_defocusParam << Debug::end;
							Debug::log_debug() << " > defocus units: " << psfParameters.m_focus.m_defocusUnits << Debug::end;
							Debug::log_debug() << " > sampling: " << psfParameters.m_sampling.m_samplingMuM << " MuM" << Debug::end;
							Debug::log_debug() << " > samples: " << psfParameters.m_sampling.m_samples << Debug::end;
							Debug::log_debug() << " > terms: " << psfParameters.m_enzSampling.m_terms << Debug::end;
							Debug::log_debug() << " > half extent: " << psfParameters.m_sampling.m_halfExtent << " (" << 
								psfParameters.m_sampling.m_halfExtentMuM << " MuM)" << Debug::end;
						});
					Debug::log_debug() << std::string(80, '=') << Debug::end;
				}
			}

			// Compute the ENZ entry parameters (defocus, zernike coefficients, etc.)
			if (computation & PsfStackComputation_PsfUnits)
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "ENZ Sampling Parameters");

				Debug::log_trace() << "ENZ Sampling Parameters" << Debug::end;

				// Perform the actual computation
				{
					auto timer = result.m_timers.startComputation("ENZ Sampling Parameters", 1);

					computeEnzEntrySamplingParameters(scene, aberration, result);
				}

				// Log the Vnm parameters for reference
				if (aberration.m_psfParameters.m_logDebug)
				{
					Debug::log_debug() << std::string(80, '=') << Debug::end;
					Debug::log_debug() << "Vnm parameters:" << Debug::end;
					Debug::log_debug() << std::string(80, '-') << Debug::end;
					{
						Debug::log_debug() << " > max degree: " << result.m_enzEntrySamplingParameters.m_maxDegree << Debug::end;
						Debug::log_debug() << " > max coefficients: " << result.m_enzEntrySamplingParameters.m_maxCoefficients << Debug::end;
						Debug::log_debug() << " > max samples: " << result.m_enzEntrySamplingParameters.m_maxSamples << Debug::end;
						Debug::log_debug() << " > max terms: " << result.m_enzEntrySamplingParameters.m_maxOrder << Debug::end;
						Debug::log_debug() << " > max term order: " << result.m_enzEntrySamplingParameters.m_maxTermOrder << Debug::end;
						Debug::log_debug() << " > max wkl terms per order: " << result.m_enzEntrySamplingParameters.m_maxTermsPerOrder << Debug::end;
						Debug::log_debug() << " > max extent: " << result.m_enzEntrySamplingParameters.m_maxExtent << Debug::end;
					}
					Debug::log_debug() << std::string(80, '=') << Debug::end;
				}
			}

			// Compute the complex ENZ coefficients
			if (shouldRecomputeEnzWkl(scene, aberration, computation, result))
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "wkl Coefficients");

				Debug::log_trace() << "wkl Coefficients" << Debug::end;

				// Clear the stack first
				result.m_enzWklCoefficients.resize(boost::extents[0][0][0][0]);
				if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
					Scene::deleteGPUBuffer(scene, "PsfStack_WklBuffer");

				// Allocate space for the ENZ coeffs
				result.m_enzWklCoefficients.resize(boost::extents
					[aberration.m_psfParameters.m_betaDegrees + 1]
					[aberration.m_psfParameters.m_betaDegrees + 1]
					[result.m_enzEntrySamplingParameters.m_maxOrder]
					[result.m_enzEntrySamplingParameters.m_maxTermsPerOrder]);
				std::fill_n(result.m_enzWklCoefficients.data(), result.m_enzWklCoefficients.num_elements(), 0.0f);

				// Perform the wkl computation on the CPU
				{
					auto timer = result.m_timers.startComputation("wkl Coefficients", numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees));

					Threading::threadedExecuteIndices(
						Threading::ThreadedExecuteParams(Threading::numThreads(), " > wkl coefficients", "coefficient", progressLogLevel(scene, aberration)),
						[&](Threading::ThreadedExecuteEnvironment const& environment, size_t coeffId)
						{
							computeEnzWklCoefficients(scene, aberration, result, coeffId + 1);
						},
						numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees));
				}

				// Upload the wkl coefficients to the GPU
				if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
				{
					uploadEnzWklCoefficientsGPU(scene, aberration, result);
				}

				// Log the statistics for reference
				if (aberration.m_psfParameters.m_logStats)
				{
					size_t totalWklBytes = result.m_enzWklCoefficients.num_elements() * sizeof(float);
					Debug::log_debug() << "Number of wkl coefficients: " << result.m_enzWklCoefficients.num_elements() << Debug::end;
					Debug::log_debug() << "Total memory consumed by wkl coefficients: " << Units::bytesToString(totalWklBytes) << Debug::end;
				}
			}

			// Upload the wkl coefficients to the GPU
			if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU && scene.m_genericBuffers.find("PsfStack_WklBuffer") == scene.m_genericBuffers.end())
			{
				uploadEnzWklCoefficientsGPU(scene, aberration, result);
			}

			// Compute the cylindrical Bessel coefficients
			if (shouldRecomputeCylindricalBessel(scene, aberration, computation, result))
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Cylindrical Bessel Coefficients");

				Debug::log_trace() << "Cylindrical Bessel Coefficients" << Debug::end;

				// Clear the stack first
				result.m_enzCylindricalBessel.clear();
				if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
					Scene::deleteGPUBuffer(scene, "PsfStack_CylindricalBesselBuffer");

				// Allocate space for the cylindrical Bessel coeffs
				result.m_enzCylindricalBessel.resize(result.m_enzEntrySamplingParameters.m_maxTermOrder);

				// Perform the actual computation
				{
					auto timer = result.m_timers.startComputation("Cylindrical Bessel Coefficients", result.m_enzCylindricalBessel.size());

					if (aberration.m_psfParameters.m_besselBatchSize <= 1)
					{
						Threading::threadedExecuteIndices(
							Threading::ThreadedExecuteParams(Threading::numThreads(), " > cylindrical Bessel coefficients", "coefficient", progressLogLevel(scene, aberration)),
							[&](Threading::ThreadedExecuteEnvironment const& environment, size_t K)
							{
								computeEnzCylindricalBesselSingle(scene, aberration, result, K);
							},
							result.m_enzCylindricalBessel.size());
					}
					else
					{
						size_t maxOrder = result.m_enzCylindricalBessel.size() - 1;
						size_t batchSize = aberration.m_psfParameters.m_besselBatchSize;
						size_t numBatches = (maxOrder + 1 + batchSize - 1) / batchSize;
						Threading::threadedExecuteIndices(
							Threading::ThreadedExecuteParams(Threading::numThreads(), " > cylindrical Bessel coefficients", "batch", progressLogLevel(scene, aberration)),
							[&](Threading::ThreadedExecuteEnvironment const& environment, size_t B)
							{
								const size_t nMin = B * batchSize;
								const size_t nMax = std::min((B + 1) * batchSize - 1, maxOrder);
								computeEnzCylindricalBesselsRange(scene, aberration, result, nMin, nMax);
							},
							numBatches);

						// Manually compute K = 0
						computeEnzCylindricalBesselSingle(scene, aberration, result, 0);
					}
				}

				// Upload the cylindrical bessel coefficients to the GPU
				if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
				{
					uploadEnzCylindricalBesselGPU(scene, aberration, result);
				}

				// Log some statistics for reference
				if (aberration.m_psfParameters.m_logStats)
				{
					size_t numCylindricalCoeffs = 0;
					for (auto const& bessel : result.m_enzCylindricalBessel)
					{
						numCylindricalCoeffs += bessel.size();
					}
					const size_t totalCylindricalBytes = numCylindricalCoeffs * sizeof(ScalarComputation);
					Debug::log_debug() << "Number of cylindrical Bessel terms: " << result.m_enzCylindricalBessel.size() << Debug::end;
					Debug::log_debug() << "Total memory consumed by cylindrical Bessel terms: " << Units::bytesToString(totalCylindricalBytes) << Debug::end;
				}
			}

			// Upload the cylindrical bessel coefficients to the GPU
			if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU && scene.m_genericBuffers.find("PsfStack_CylindricalBesselBuffer") == scene.m_genericBuffers.end())
			{
				uploadEnzCylindricalBesselGPU(scene, aberration, result);
			}

			// Compute the spherical Bessel coefficients
			if (shouldRecomputeSphericalBessel(scene, aberration, computation, result))
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Spherical Bessel Coefficients");

				Debug::log_trace() << "Spherical Bessel Coefficients" << Debug::end;

				// Clear the stack first
				result.m_enzSphericalBessel.clear();
				if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
					Scene::deleteGPUBuffer(scene, "PsfStack_SphericalBesselBuffer");

				// Allocate space for the coefficients
				result.m_enzSphericalBessel.resize(result.m_enzEntrySamplingParameters.m_maxOrder);

				// Perform the actual computation
				{
					auto timer = result.m_timers.startComputation("Spherical Bessel Coefficients", result.m_enzSphericalBessel.size());

					if (aberration.m_psfParameters.m_besselBatchSize <= 1)
					{
						Threading::threadedExecuteIndices(
							Threading::ThreadedExecuteParams(Threading::numThreads(), " > spherical Bessel coefficients", "coefficient", progressLogLevel(scene, aberration)),
							[&](Threading::ThreadedExecuteEnvironment const& environment, size_t k)
							{
								computeEnzSphericalBesselSingle(scene, aberration, result, k);
							},
							result.m_enzSphericalBessel.size());
					}
					else
					{
						size_t maxOrder = result.m_enzSphericalBessel.size() - 1;
						size_t batchSize = aberration.m_psfParameters.m_besselBatchSize;
						size_t numBatches = (maxOrder + 1 + batchSize - 1) / batchSize;
						Threading::threadedExecuteIndices(
							Threading::ThreadedExecuteParams(Threading::numThreads(), " > spherical Bessel coefficients", "batch", progressLogLevel(scene, aberration)),
							[&](Threading::ThreadedExecuteEnvironment const& environment, size_t B)
							{
								const size_t kMin = B * batchSize;
								const size_t kMax = std::min((B + 1) * batchSize - 1, maxOrder);
								computeEnzSphericalBesselsRange(scene, aberration, result, kMin, kMax);
							},
							numBatches);

						// Manually compute K = 0
						computeEnzSphericalBesselSingle(scene, aberration, result, 0);
					}
				}

				// Upload the spherical bessel coefficients to the GPU
				if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
				{
					uploadEnzSphericalBesselGPU(scene, aberration, result);
				}

				// Log some statistics for reference
				if (aberration.m_psfParameters.m_logStats)
				{
					size_t numSpherical = 0, numSphericalCoeffs = 0;
					for (auto const& bessel : result.m_enzSphericalBessel)
					{
						numSphericalCoeffs += bessel.size();
						++numSpherical;
					}
					size_t totalSphericalBytes = numSphericalCoeffs * sizeof(ScalarComputation);
					Debug::log_debug() << "Number of spherical Bessel terms: " << numSpherical << Debug::end;
					Debug::log_debug() << "Total memory consumed by spherical Bessel terms: " << Units::bytesToString(totalSphericalBytes) << Debug::end;
				}
			}

			// Upload the spherical bessel coefficients to the GPU
			if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU && scene.m_genericBuffers.find("PsfStack_SphericalBesselBuffer") == scene.m_genericBuffers.end())
			{
				uploadEnzSphericalBesselGPU(scene, aberration, result);
			}

			// Compute the inner Vnm terms
			if (shouldRecomputeVnmInner(scene, aberration, computation, result))
			{
				// Allocate space for the inner Vnm terms on the CPU
				if (aberration.m_psfParameters.m_backend == PSFStackParameters::CPU)
				{
					// Clear the array first
					result.m_enzVnmInnerTerms.resize(boost::extents[0][0][0]);

					// Resize the array to the correct size
					result.m_enzVnmInnerTerms.resize(boost::extents
						[aberration.m_psfParameters.m_betaDegrees + 1]
						[aberration.m_psfParameters.m_betaDegrees + 1]
						[result.m_enzEntrySamplingParameters.m_maxOrder]);
				}

				// Clear the GPU buffer first
				if (aberration.m_psfParameters.m_backend == PSFStackParameters::GPU)
				{
					Scene::deleteGPUBuffer(scene, "PsfStack_VnmInnerBuffer");
				}

				// Perform the computation
				{
					auto timer = result.m_timers.startComputation("Vnm Inner Terms", 
						result.m_enzEntrySamplingParameters.m_maxCoefficients * result.m_enzEntrySamplingParameters.m_maxOrder);

					if (aberration.m_psfParameters.m_backend == PSFStackParameters::CPU)
					{
						Threading::threadedExecuteIndices(
							Threading::ThreadedExecuteParams(Threading::numThreads(), " > Vnm inner terms", "term", progressLogLevel(scene, aberration)),
							[&](Threading::ThreadedExecuteEnvironment const& environment, size_t coeffId, size_t k)
							{
								computeInnerVnmTermsCPU(scene, aberration, result, coeffId + 1, k);
							},
							numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees),
							result.m_enzEntrySamplingParameters.m_maxOrder);
					}

					if (aberration.m_psfParameters.m_backend == PSFStackParameters::GPU)
					{
						computeInnerVnmTermsGPU(scene, aberration, result);
					}
				}

				// Log the statistics for reference
				if (aberration.m_psfParameters.m_logStats && aberration.m_psfParameters.m_backend == PSFStackParameters::CPU)
				{
					size_t numInnerVnmCoeffs = 0;
					for (size_t n = 0; n <= aberration.m_psfParameters.m_betaDegrees; ++n)
					for (size_t m = 0; m <= aberration.m_psfParameters.m_betaDegrees; ++m)
					for (size_t k = 0; k < result.m_enzEntrySamplingParameters.m_maxOrder; ++k)
					{
						numInnerVnmCoeffs += result.m_enzVnmInnerTerms[n][m][k].size();
					}
					const size_t totalInnerVnmBytes = numInnerVnmCoeffs * sizeof(ScalarComputation);
					Debug::log_debug() << "Total memory consumed by inner Vnm terms: " << Units::bytesToString(totalInnerVnmBytes) << Debug::end;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void phasePsfs(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation, PSFStack& result)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "PSFs");

			if (aberration.m_psfParameters.m_logProgress)
				Debug::log_info() << "Computing PSFs..." << Debug::end;

			Debug::log_trace() << "PSFs" << Debug::end;

			// Clear the array
			{
				result.m_psfs.resize(boost::extents[0][0][0][0][0][0]);
			}

			// Total number of PSFs
			const size_t numDefocues = aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size();
			const size_t numAnglesHor = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size();
			const size_t numAnglesVert = aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size();
			const size_t numLambdas = aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size();
			const size_t numApertures = aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size();
			const size_t numFocuses = aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size();
			const size_t numPsfs = numDefocues * numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;
			const size_t numCoefficients = numAnglesHor * numAnglesVert * numLambdas * numApertures * numFocuses;

			// Allocate space for the original PSFs
			result.m_psfs.resize(boost::extents
				[aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size()]
				[aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size()]
			);

			// Perform the PSF computation on the CPU
			if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::CPU)
			{
				auto timer = result.m_timers.startComputation("PSFs", numPsfs);

				// Perform the computation
				Threading::threadedExecuteIndices(
					Threading::ThreadedExecuteParams(Threading::numThreads(), " > PSFs", "PSF", progressLogLevel(scene, aberration)),
						[&](Threading::ThreadedExecuteEnvironment const& environment, size_t defocusId, size_t horizontalId, size_t verticalId, size_t lambdaId, size_t apertureId, size_t focusId)
						{
							computePsfCPU(scene, aberration, result, PsfIndex{ defocusId, horizontalId, verticalId, lambdaId, apertureId, focusId });
						},
						aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size());
			}

			// Perform the PSF computation on the GPU
			if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
			{
				auto timer = result.m_timers.startComputation("PSFs", numPsfs);

				// Initiate the GPU computation
				{
					auto timer = result.m_timers.startComputation("GPU Init", 1);

					initPsfComputationGpu(scene, aberration, computation, result);
				}

				{
					auto timer = result.m_timers.startComputation("Main", numPsfs);

					// Perform the computation
					Threading::threadedExecuteIndices(
						Threading::ThreadedExecuteParams(1, " > PSFs", "PSF", progressLogLevel(scene, aberration)),
						[&](Threading::ThreadedExecuteEnvironment const& environment, size_t defocusId, size_t horizontalId, size_t verticalId, size_t lambdaId, size_t apertureId, size_t focusId)
						{
							computePsfGPU(scene, aberration, result, PsfIndex{ defocusId, horizontalId, verticalId, lambdaId, apertureId, focusId });
						},
						aberration.m_psfParameters.m_evaluatedParameters.m_objectDistances.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_lambdas.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size(),
						aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size());
				}

				// Finish GPU computation
				{
					auto timer = result.m_timers.startComputation("GPU Cleanup", 1);

					finishPsfComputationGpu(scene, aberration, computation, result);
				}
			}

			// Print out some useful stats
			if (aberration.m_psfParameters.m_logStats)
			{
				size_t totalPsfBytes = 0;
				for (size_t psfId = 0; psfId < result.m_psfEntryParameters.num_elements(); ++psfId)
				{
					totalPsfBytes += result.m_psfs.data()[psfId].m_psf.size() * sizeof(ScalarComputation);
				}
				Debug::log_debug() << "Total memory consumed by the PSFs: " << Units::bytesToString(totalPsfBytes) << Debug::end;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void computePSFStack(Scene::Scene& scene, WavefrontAberration& aberration, PsfStackComputation computation)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "PSF Computation");

		PSFStack& result = aberration.m_psfStack;

		Debug::DebugOutputLevel progressLogLevel = (aberration.m_psfParameters.m_logProgress) ? Debug::Info: Debug::Trace;
		Debug::DebugOutputLevel statsLogLevel = (aberration.m_psfParameters.m_logStats) ? Debug::Debug : Debug::Null;

		Debug::log_output(progressLogLevel) << "Computing PSF stack..." << Debug::end;
		Debug::log_debug() << "  - Aberration: " << aberration.m_name << Debug::end;

		// Initialize the timer set
		result.m_timers = DateTime::TimerSet(statsLogLevel, DateTime::Seconds);

		// Perform the stack computation
		{
			auto timer = result.m_timers.startComputation("PSF Stack", getNumPsfsTotal(scene, aberration));

			// Generate the list of parameters to evaluate
			ComputePsfStack::phaseParamRanges(scene, aberration, computation, result);

			// Wait for the GPU
			ComputePsfStack::phaseGPUSync(scene, aberration, computation, result);

			// Compute the eye parameters
			if (computation & PsfStackComputation_EyeParameters) ComputePsfStack::phaseEyeParameters(scene, aberration, computation, result);

			// Derive the parameters
			if (computation & PsfStackComputation_PsfParameters) ComputePsfStack::phasePsfParameters(scene, aberration, computation, result);

			// Produce the original Psfs
			if (computation & PsfStackComputation_Psfs) ComputePsfStack::phasePsfs(scene, aberration, computation, result);
		}

		// Set the computation ID
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
		const size_t currentFrameId = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
		aberration.m_psfStack.m_debugInformationCommon.m_lastComputedFrameId = currentFrameId;
		aberration.m_psfStack.m_debugInformationCommon.m_backend = aberration.m_psfParameters.m_backend;

		Debug::log_output(progressLogLevel) << "PSF stack successfully computed" << Debug::end;

		// Display the overall computation times
		result.m_timers.displaySummary(progressLogLevel, DateTime::Seconds, aberration.m_psfParameters.m_truncateStatsTiming, 
			aberration.m_psfParameters.m_shortenStatsTiming);
	}

	////////////////////////////////////////////////////////////////////////////////
	void freeCacheResources(Scene::Scene& scene, WavefrontAberration& aberration)
	{
		if (aberration.m_psfParameters.m_backend == Aberration::PSFStackParameters::GPU)
		{
			Scene::deleteGPUBuffer(scene, "PsfStack_WklBuffer");
			Scene::deleteGPUBuffer(scene, "PsfStack_CylindricalBesselBuffer");
			Scene::deleteGPUBuffer(scene, "PsfStack_SphericalBesselBuffer");
			Scene::deleteGPUBuffer(scene, "PsfStack_VnmInnerBuffer");
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace PsfStackChartCommon
	{
		////////////////////////////////////////////////////////////////////////////////
		static ImVec4 s_rgbColorsVec4[] =
		{
			ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
			ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
			ImVec4(0.0f, 0.0f, 1.0f, 1.0f),
		};

		////////////////////////////////////////////////////////////////////////////////
		static ImU32 s_rgbColorsU32[] =
		{
			ImGui::Color32FromGlmVector(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)),
			ImGui::Color32FromGlmVector(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)),
			ImGui::Color32FromGlmVector(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)),
		};

		////////////////////////////////////////////////////////////////////////////////
		static const char* s_rgbNames[] =
		{
			"Red", "Green", "Blue"
		};

		////////////////////////////////////////////////////////////////////////////////
		static size_t s_axisId[] =
		{
			0, // Object distance
			1, // Horizontal angle
			2, // Vertical angle
			4, // Aperture diameter
			5  // Focus distance
		};

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		auto& readArray(boost::multi_array<T, 6>& arr, PsfIndex const& coords)
		{
			return arr
				[std::min(coords[0], arr.size() - 1)]
				[std::min(coords[1], arr[0].size() - 1)]
				[std::min(coords[2], arr[0][0].size() - 1)]
				[std::min(coords[3], arr[0][0][0].size() - 1)]
				[std::min(coords[4], arr[0][0][0][0].size() - 1)]
				[std::min(coords[5], arr[0][0][0][0][0].size() - 1)];
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		auto const& readArray(boost::multi_array<T, 6> const& arr, PsfIndex const& coords)
		{
			return arr
				[std::min(coords[0], arr.size() - 1)]
				[std::min(coords[1], arr[0].size() - 1)]
				[std::min(coords[2], arr[0][0].size() - 1)]
				[std::min(coords[3], arr[0][0][0].size() - 1)]
				[std::min(coords[4], arr[0][0][0][0].size() - 1)]
				[std::min(coords[5], arr[0][0][0][0][0].size() - 1)];
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfIndex getCoords(size_t d, size_t h, size_t v, size_t l, size_t a, size_t f)
		{
			return { d, h, v, l, a, f };
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfIndex getCoords(int axis, int index)
		{
			PsfIndex coords = getCoords(0, 0, 0, 0, 0, 0);
			coords[s_axisId[axis]] = index;
			return coords;
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfIndex getCoords(size_t d, size_t h, size_t v, size_t l, size_t a, size_t f, int axis, int index)
		{
			PsfIndex coords = getCoords(d, h, v, l, a, f);
			coords[s_axisId[axis]] = index;
			return coords;
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfIndex getDisplayPsfCoords(WavefrontAberration const& aberration)
		{
			return getCoords
			(
				aberration.m_psfPreview.m_psfId,
				aberration.m_psfPreview.m_horizontalId,
				aberration.m_psfPreview.m_verticalId,
				aberration.m_psfPreview.m_lambdaId,
				aberration.m_psfPreview.m_apertureId,
				aberration.m_psfPreview.m_focusId
			);
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfIndex getDisplayPsfCoords(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			return getDisplayPsfCoords(aberration);
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfStackElements::PsfEntryParams const& getDisplayPsfParametersEntry(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			return getPsfEntryParameters(scene, aberration, getDisplayPsfCoords(scene, aberration));
		}

		////////////////////////////////////////////////////////////////////////////////
		PsfStackElements::PsfEntry const& getDisplayPsf(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			return getPsfEntry(scene, aberration, getDisplayPsfCoords(scene, aberration));
		}

		////////////////////////////////////////////////////////////////////////////////
		float getXAxis(WavefrontAberration const& aberration, int index, PSFStackPreviewParameters::Focus::DisplayAxis axis)
		{
			auto const& units = readArray(aberration.m_psfStack.m_psfEntryParameters, getCoords(axis, index)).m_units;

			switch (axis)
			{
			case PSFStackPreviewParameters::Focus::ObjectDistance:   return units.m_objectDistanceM;
			case PSFStackPreviewParameters::Focus::HorizontalAngle:  return units.m_horizontalAngle;
			case PSFStackPreviewParameters::Focus::VerticalAngle:    return units.m_verticalAngle;
			case PSFStackPreviewParameters::Focus::ApertureDiameter: return units.m_apertureDiameterMM;
			case PSFStackPreviewParameters::Focus::FocusDistance:    return units.m_focusDistanceM;
			}

			// Fall back to 0
			return 0.0f;
		}

		////////////////////////////////////////////////////////////////////////////////
		size_t getDataCount(WavefrontAberration const& aberration, PSFStackPreviewParameters::Focus::DisplayAxis axis)
		{
			switch (axis)
			{
			case PSFStackPreviewParameters::Focus::ObjectDistance:
				return aberration.m_psfStack.m_psfEntryParameters.size();
			case PSFStackPreviewParameters::Focus::HorizontalAngle:
				return aberration.m_psfStack.m_psfEntryParameters[0].size();
			case PSFStackPreviewParameters::Focus::VerticalAngle:
				return aberration.m_psfStack.m_psfEntryParameters[0][0].size();
			case PSFStackPreviewParameters::Focus::ApertureDiameter:
				return aberration.m_psfStack.m_psfEntryParameters[0][0][0][0].size();
			case PSFStackPreviewParameters::Focus::FocusDistance:
				return aberration.m_psfStack.m_psfEntryParameters[0][0][0][0][0].size();
			}
			return 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		const char* getAxisName(WavefrontAberration const& aberration, PSFStackPreviewParameters::Focus::DisplayAxis axis)
		{
			switch (axis)
			{
			case PSFStackPreviewParameters::Focus::ObjectDistance:
				return "Object Distance";
			case PSFStackPreviewParameters::Focus::HorizontalAngle:
				return "Horizontal Angle";
			case PSFStackPreviewParameters::Focus::VerticalAngle:
				return "Vertical Angle";
			case PSFStackPreviewParameters::Focus::ApertureDiameter:
				return "Aperture Diameter";
			case PSFStackPreviewParameters::Focus::FocusDistance:
				return "Focus Distance";
			}
			return "Unknown Axis";
		}

		////////////////////////////////////////////////////////////////////////////////
		const char* getAxisUnits(WavefrontAberration const& aberration, PSFStackPreviewParameters::Focus::DisplayAxis axis)
		{
			switch (axis)
			{
			case PSFStackPreviewParameters::Focus::ObjectDistance:
				return "m";
			case PSFStackPreviewParameters::Focus::HorizontalAngle:
				return " deg";
			case PSFStackPreviewParameters::Focus::VerticalAngle:
				return " deg";
			case PSFStackPreviewParameters::Focus::ApertureDiameter:
				return " mm";
			case PSFStackPreviewParameters::Focus::FocusDistance:
				return " m";
			}
			return "Unknown Units";
		}

		////////////////////////////////////////////////////////////////////////////////
		void makeAxisLabels(WavefrontAberration const& aberration, PSFStackPreviewParameters::Focus::DisplayAxis axis, size_t stepSize,
			std::vector<double>& positions, std::vector<std::string>& labelsStr, std::vector<const char*>& labels)
		{
			const size_t numTotalData = getDataCount(aberration, axis);
			const size_t numData = ((numTotalData - 1) / stepSize) + 1;
			positions = std::vector<double>(numData);
			labelsStr = std::vector<std::string>(numData);
			labels = std::vector<const char*>(numData);

			for (int i = 0, id = 0; i < numData; ++i, id += stepSize)
			{
				labelsStr[i] = std::to_string(getXAxis(aberration, id, axis));
				labels[i] = labelsStr[i].c_str();
				positions[i] = double(id);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void makeAxisLabels(WavefrontAberration const& aberration, PSFStackPreviewParameters::Focus::DisplayAxis axis,
			std::vector<std::string>& labelsStr, std::vector<const char*>& labels)
		{
			std::vector<double> positions;
			makeAxisLabels(aberration, axis, 1, positions, labelsStr, labels);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace CoefficientChart
	{
		////////////////////////////////////////////////////////////////////////////////
		const float BAR_SIZE = 0.15f;

		////////////////////////////////////////////////////////////////////////////////
		size_t getDataCount(WavefrontAberration const& aberration)
		{
			switch (aberration.m_psfPreview.m_coefficient.m_visualizationMethod)
			{
			case PSFStackPreviewParameters::Coefficient::Alpha: return ZernikeCoefficientsAlpha::NUM_COEFFS;
			case PSFStackPreviewParameters::Coefficient::AlphaPhaseCumulative: return ZernikeCoefficientsAlpha::NUM_COEFFS;
			case PSFStackPreviewParameters::Coefficient::AlphaPhaseResidual: return ZernikeCoefficientsAlpha::NUM_COEFFS;
			case PSFStackPreviewParameters::Coefficient::BetaReal: return numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees);
			case PSFStackPreviewParameters::Coefficient::BetaImag: return numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees);
			case PSFStackPreviewParameters::Coefficient::BetaMagnitude: return numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees);
			case PSFStackPreviewParameters::Coefficient::BetaPhase: return numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees);
			}
			return 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		void makeAxisLabels(WavefrontAberration const& aberration, size_t stepSize, 
			std::vector<double>& positions, std::vector<std::string>& labelsStr, std::vector<const char*>& labels)
		{
			const size_t numTotalData = getDataCount(aberration);
			const size_t numData = ((numTotalData - 1) / stepSize) + 1;
			positions = std::vector<double>(numData);
			labelsStr = std::vector<std::string>(numData);
			labels = std::vector<const char*>(numData);

			for (int i = 0, id = 0; i < numData; ++i, id += stepSize)
			{
				const auto [n, m] = ZernikeIndices::single2doubleNoll(id + 1);
				labelsStr[i] = "Z[" + std::to_string(n) + "," + std::to_string(m) + "]";
				labels[i] = labelsStr[i].c_str();
				positions[i] = double(id);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		float getCoefficient(const WavefrontAberration& aberration, int channel, int index,
			const PSFStackPreviewParameters::Coefficient::VisualizationMethod visualizationMethod)
		{
			// Index of the PSF to show the visualization for
			const std::array<size_t, 6> psfIndex = PsfStackChartCommon::getCoords
			(
				aberration.m_psfPreview.m_psfId,
				aberration.m_psfPreview.m_horizontalId,
				aberration.m_psfPreview.m_verticalId,
				channel,
				aberration.m_psfPreview.m_apertureId,
				aberration.m_psfPreview.m_focusId
			);

			// Index of the current coefficient
			const size_t coeffIndex = index + 1;

			// PSF parameters
			auto const& psfParameters = PsfStackChartCommon::readArray(aberration.m_psfStack.m_psfEntryParameters, psfIndex);
			auto const& coefficients = psfParameters.m_coefficients;

			switch (visualizationMethod)
			{
			case PSFStackPreviewParameters::Coefficient::Alpha: return coefficients.m_alpha[coeffIndex];
			case PSFStackPreviewParameters::Coefficient::AlphaPhaseCumulative: return coefficients.m_alphaPhaseCumulative[coeffIndex];
			case PSFStackPreviewParameters::Coefficient::AlphaPhaseResidual: return coefficients.m_alphaPhaseResidual[coeffIndex];
			case PSFStackPreviewParameters::Coefficient::BetaReal: return coefficients.m_beta[coeffIndex].real();
			case PSFStackPreviewParameters::Coefficient::BetaImag: return coefficients.m_beta[coeffIndex].imag();
			case PSFStackPreviewParameters::Coefficient::BetaMagnitude: return std::abs(coefficients.m_beta[coeffIndex]);
			case PSFStackPreviewParameters::Coefficient::BetaPhase: return std::arg(coefficients.m_beta[coeffIndex]);
			}

			// Fall back to 0
			return 0.0f;
		}

		////////////////////////////////////////////////////////////////////////////////
		float getCoefficient(const WavefrontAberration& aberration, int channel, int index)
		{
			return getCoefficient(aberration, channel, index, aberration.m_psfPreview.m_coefficient.m_visualizationMethod);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGenerator(int channel, void* pPayload, int index)
		{
			// Extract the payload
			const WavefrontAberration& aberration = *(const WavefrontAberration*)pPayload;

			// Return the final plot point
			return ImPlotPoint(double(index) + double(channel - 1) * BAR_SIZE, getCoefficient(aberration, channel, index));
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorRed(void* pPayload, int index)
		{
			return dataGenerator(0, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorGreen(void* pPayload, int index)
		{
			return dataGenerator(1, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorBlue(void* pPayload, int index)
		{
			return dataGenerator(2, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		const char* getPlotName(WavefrontAberration& aberration)
		{
			switch (aberration.m_psfPreview.m_coefficient.m_visualizationMethod)
			{
			case PSFStackPreviewParameters::Coefficient::Alpha: return "OPD";
			case PSFStackPreviewParameters::Coefficient::AlphaPhaseCumulative: return "Phase (Cumulative)";
			case PSFStackPreviewParameters::Coefficient::AlphaPhaseResidual: return "Phase (Residual)";
			case PSFStackPreviewParameters::Coefficient::BetaReal: return "Beta (Real)";
			case PSFStackPreviewParameters::Coefficient::BetaImag: return "Beta (Imag)";
			case PSFStackPreviewParameters::Coefficient::BetaMagnitude: return "Beta (Magnitude)";
			case PSFStackPreviewParameters::Coefficient::BetaPhase: return "Beta (Phase)";
			}
			return "Unknown Visualization Mode";
		}

		////////////////////////////////////////////////////////////////////////////////
		using ImPlotGenerator = ImPlotPoint(*)(void* data, int idx);
		static ImPlotGenerator s_generators[3] =
		{
			&dataGeneratorRed, &dataGeneratorGreen, &dataGeneratorBlue
		};

		////////////////////////////////////////////////////////////////////////////////
		void generateChart(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Coefficient Chart");

			// Number of data entries
			const size_t numData = getDataCount(aberration);

			// Generate tick labels
			const int labelStep = (numData / 7);
			std::vector<double> positions; std::vector<std::string> labelsStr; std::vector<const char*> labels;
			makeAxisLabels(aberration, labelStep, positions, labelsStr, labels);

			// X tick marks
			ImPlot::SetNextPlotTicksX(positions.data(), labels.size(), labels.data());

			// Axis flags
			const ImPlotFlags plotFlags = ImPlotFlags_AntiAliased | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend;
			const char* xName = "Coefficient";
			const ImPlotAxisFlags xFlags = ImPlotAxisFlags_AutoFit;
			const ImPlotAxisFlags yFlags = ImPlotAxisFlags_AutoFit;

			// Generate the plot
			if (ImPlot::BeginPlot(getPlotName(aberration), xName, nullptr, ImVec2(-1, 0), plotFlags, xFlags, yFlags))
			{
				// Show the individual bars
				for (int i = 0; i < 3; ++i)
				{
					ImPlot::SetNextLineStyle(PsfStackChartCommon::s_rgbColorsVec4[i]);
					ImPlot::SetNextFillStyle(PsfStackChartCommon::s_rgbColorsVec4[i]);
					ImPlot::PlotBarsG(PsfStackChartCommon::s_rgbNames[i], s_generators[i], &aberration, numData, BAR_SIZE);
				}

				// Handle tooltips
				if (ImPlot::IsPlotHovered())
				{
					// Mouse coordinates in plot coordinates
					const ImPlotPoint mouse = ImPlot::GetPlotMousePos();

					for (size_t i = 0; i < aberration.m_psfParameters.m_lambdas.size(); ++i)
					{
						ImPlot::MouseHitBarResult mouseHit = ImPlot::FindMouseHitBarG(s_generators[i], &aberration, numData, BAR_SIZE, mouse);

						// Generate the tooltip if the index is valid
						if (mouseHit.m_inside)
						{
							// Add the highlighter
							ImPlot::HighlightMouseHitBarG(mouseHit);

							// Generate tooltip
							ImGui::BeginTooltip();
							const auto [n, m] = ZernikeIndices::single2doubleNoll(mouseHit.m_index + 1);
							ImGui::Text("Coefficient[%3d, %3d]", n, m);
							ImGui::BulletText("Channel: %s (%d)", PsfStackChartCommon::s_rgbNames[i], i + 1);
							ImGui::BulletText("%s: %f", getPlotName(aberration), getCoefficient(aberration, i, mouseHit.m_index));

							ImGui::EndTooltip();
						}
					}
				}

				ImPlot::EndPlot();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace FocusChart
	{
		////////////////////////////////////////////////////////////////////////////////
		float getXAxis(WavefrontAberration const& aberration, int index)
		{
			return PsfStackChartCommon::getXAxis(aberration, index, aberration.m_psfPreview.m_focus.m_displayAxis);
		}

		////////////////////////////////////////////////////////////////////////////////
		float getXAxis(void* pPayload, int index)
		{
			const WavefrontAberration& aberration = *(const WavefrontAberration*)pPayload;
			return getXAxis(aberration, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		float getParam(WavefrontAberration const& aberration, PsfStackElements::PsfEntryParams const& parameters, PsfStackElements::PsfEntry const& psf)
		{
			// Return the corresponding parameters
			switch (aberration.m_psfPreview.m_focus.m_visualizationMethod)
			{
			case PSFStackPreviewParameters::Focus::DefocusParam: return parameters.m_focus.m_defocusParam;
			case PSFStackPreviewParameters::Focus::ImageShiftAberration: return parameters.m_focus.m_imageShiftAberrationMuM;
			case PSFStackPreviewParameters::Focus::ImageShiftObjectDistance: return parameters.m_focus.m_imageShiftObjectDepthMuM;
			case PSFStackPreviewParameters::Focus::ImageShift: return parameters.m_focus.m_imageShiftMuM;
			case PSFStackPreviewParameters::Focus::BlurRadius: return blurRadiusPixels(psf, aberration.m_psfPreview.m_resolution, aberration.m_psfPreview.m_fovy);
			}
			return 0.0f;
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGenerator(int channel, const void* pPayload, int index)
		{
			// Extract the payload
			const WavefrontAberration& aberration = *(const WavefrontAberration*)pPayload;

			// Evaluation coordinates
			auto coords = PsfStackChartCommon::getCoords
			(
				aberration.m_psfPreview.m_psfId,
				aberration.m_psfPreview.m_horizontalId,
				aberration.m_psfPreview.m_verticalId,
				channel,
				aberration.m_psfPreview.m_apertureId,
				aberration.m_psfPreview.m_focusId,
				aberration.m_psfPreview.m_focus.m_displayAxis,
				index
			);

			// Extract the PSFs and their parameters
			auto const& parameters = PsfStackChartCommon::readArray(aberration.m_psfStack.m_psfEntryParameters, coords);
			auto const& psf = PsfStackChartCommon::readArray(aberration.m_psfStack.m_psfs, coords);

			// Return the final plot point
			return ImPlotPoint(getXAxis(aberration, index), getParam(aberration, parameters, psf));
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorRed(void* pPayload, int index)
		{
			return dataGenerator(0, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorGreen(void* pPayload, int index)
		{
			return dataGenerator(1, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		ImPlotPoint dataGeneratorBlue(void* pPayload, int index)
		{
			return dataGenerator(2, pPayload, index);
		}

		////////////////////////////////////////////////////////////////////////////////
		const char* getPlotName(WavefrontAberration& aberration)
		{
			switch (aberration.m_psfPreview.m_focus.m_visualizationMethod)
			{
			case PSFStackPreviewParameters::Focus::DefocusParam: return "Defocus Param";
			case PSFStackPreviewParameters::Focus::ImageShiftAberration: return "Image Shift - Aberration (MuM)";
			case PSFStackPreviewParameters::Focus::ImageShiftObjectDistance: return "Image Shift - Object Distance (MuM)";
			case PSFStackPreviewParameters::Focus::ImageShift: return "Image Shift (MuM)";
			case PSFStackPreviewParameters::Focus::BlurRadius: return "Blur Radius (Retina)";
			}
			return "Unknown Visualization Mode";
		}

		////////////////////////////////////////////////////////////////////////////////
		using ImPlotGenerator = ImPlotPoint(*)(void* data, int idx);
		ImPlotGenerator s_generators[3] =
		{
			&dataGeneratorRed, &dataGeneratorGreen, &dataGeneratorBlue
		};

		////////////////////////////////////////////////////////////////////////////////
		bool isLogScale(Scene::Scene& scene, Scene::Object* object, WavefrontAberration& aberration)
		{
			return aberration.m_psfPreview.m_focus.m_displayAxis == PSFStackPreviewParameters::Focus::DisplayAxis::ObjectDistance ||
				aberration.m_psfPreview.m_focus.m_displayAxis == PSFStackPreviewParameters::Focus::DisplayAxis::FocusDistance;
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateChart(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Focus Chart");

			// The displayed axis identifier
			const auto displayAxis = aberration.m_psfPreview.m_focus.m_displayAxis;

			// Number of data entries
			const size_t numData = PsfStackChartCommon::getDataCount(aberration, displayAxis);

			// Axis flags
			const ImPlotFlags plotFlags = ImPlotFlags_AntiAliased | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend;
			const char* xName = PsfStackChartCommon::getAxisName(aberration, displayAxis);
			const ImPlotAxisFlags xFlags = ImPlotAxisFlags_AutoFit | (isLogScale(scene, object, aberration) ? ImPlotAxisFlags_LogScale : 0);
			const ImPlotAxisFlags yFlags = ImPlotAxisFlags_AutoFit;

			// Generate the plot
			if (ImPlot::BeginPlot(getPlotName(aberration), xName, nullptr, ImVec2(-1, 0), plotFlags, xFlags, yFlags))
			{
				// Generate the plots
				for (int i = 0; i < 3; ++i)
				{
					ImPlot::SetNextLineStyle(PsfStackChartCommon::s_rgbColorsVec4[i]);
					ImPlot::PlotLineG(PsfStackChartCommon::s_rgbNames[i], s_generators[i], &aberration, numData);
				}

				// Handle tooltips
				if (ImPlot::IsPlotHovered()) 
				{
					// Mouse coordinates in plot coordinates
					const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
					ImPlot::MouseHitLineResult mouseHit[3];
					for (size_t i = 0; i < aberration.m_psfParameters.m_lambdas.size(); ++i)
						mouseHit[i] = ImPlot::FindMouseHitLineG(s_generators[i], &aberration, numData, mouse);

					// Generate the tooltip if the index is valid
					if (mouseHit[0].m_inside)
					{
						// Add the highlighter
						for (size_t i = 0; i < aberration.m_psfParameters.m_lambdas.size(); ++i)
							ImPlot::HighlightMouseHitLineG(mouseHit[i], PsfStackChartCommon::s_rgbColorsU32[i]);

						// Generate tooltip
						ImGui::BeginTooltip();
						ImGui::Text("%s: %f %s", PsfStackChartCommon::getAxisName(aberration, displayAxis),
							mouse.x, PsfStackChartCommon::getAxisUnits(aberration, displayAxis));
						for (size_t i = 0; i < aberration.m_psfParameters.m_lambdas.size(); ++i)
							ImGui::BulletText("Channel #%d: %f", i, mouseHit[i].m_hit.y);
						ImGui::EndTooltip();
					}
				}

				ImPlot::EndPlot();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void exportData(Scene::Scene& scene, WavefrontAberration& aberration, std::string fileName)
		{
			// Result of the saving
			std::ordered_pairs_in_blocks result;

			// Full file name
			std::string fullFileName = (EnginePaths::generatedFilesFolder() / "Aberration" / fileName).string();

			for (size_t c = 0; c < aberration.m_psfParameters.m_lambdas.size(); ++c)
			{
				auto& block = result["Channel#" + std::to_string(c)];

				block.push_back({ "Lambda", std::to_string(aberration.m_psfParameters.m_lambdas[c]) });

				for (size_t x = 0; x < PsfStackChartCommon::getDataCount(aberration, aberration.m_psfPreview.m_focus.m_displayAxis); ++x)
				{
					block.push_back(
					{
						std::to_string(getXAxis(aberration, x)),
						std::to_string(dataGenerator(c, &aberration, x).y)
					});
				}
			}

			// Save the state
			Asset::savePairsInBlocks(scene, fullFileName, result, false);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace PsfPreview
	{
		////////////////////////////////////////////////////////////////////////////////
		void uploadTexture(Scene::Scene& scene, std::string const& textureName, Aberration::Psf psf)
		{
			// Normalize the psf
			psf = psf / psf.sum();
			psf = psf / psf.maxCoeff();

			// Construct the rgb image
			using MatrixXf3rm = Eigen::Matrix<glm::vec3, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
			MatrixXf3rm const& psfRgb = Eigen::Map<const Aberration::Psf>(psf.data(), psf.rows(), psf.cols()).cwiseAbs().cast<glm::vec3>();

			// Upload the texture data
			Scene::createTexture(scene, textureName, GL_TEXTURE_2D,
				psfRgb.cols(), psfRgb.rows(), 1,
				GL_RGB16F, GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
				GL_FLOAT, psfRgb.data(),
				GL_TEXTURE0);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadDisplayTextures(Scene::Scene& scene, WavefrontAberration& aberration, std::string const& ownerName, 
			Aberration::PSFStackPreviewParameters::PsfTexture& previewOptions, Aberration::PsfStackElements::PsfEntry const& psf)
		{
			// Camera to derive the field of view
			Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
			Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
			Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

			// Resolution and fov-y
			glm::ivec2 renderResolution = aberration.m_psfPreview.m_resolution;
			float fovy = aberration.m_psfPreview.m_fovy;

			// Extract the psf images
			auto const& psfFullRes = psf.m_psf;

			// Target size for the upscaled PSFs
			Eigen::Vector2i targetSize = Eigen::Vector2i(psfFullRes.cols(), psfFullRes.rows());

			// Compute the downscaled images
			float lowResSize = previewOptions.m_radius;
			auto const& psfLowRes = Aberration::resizePsf(scene, aberration, psfFullRes, lowResSize);
			auto const& psfLowResUpscaled = Eigen::resize(psfLowRes, targetSize, cv::INTER_NEAREST);

			// Compute the projected images
			float projectedSize = Aberration::blurRadiusPixels(psf, renderResolution, fovy);
			auto const& psfProjected = Aberration::resizePsf(scene, aberration, psfFullRes, projectedSize);
			auto const& psfProjectedUpscaled = Eigen::resize(psfProjected, targetSize, cv::INTER_NEAREST);

			// Upload the textures
			uploadTexture(scene, ownerName + previewOptions.m_fullRes, psfFullRes);
			uploadTexture(scene, ownerName + previewOptions.m_lowRes, psfLowResUpscaled);
			uploadTexture(scene, ownerName + previewOptions.m_projected, psfProjectedUpscaled);

			// Remember the frame ID of this data upload
			previewOptions.m_lastUploadFrameId = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadDisplayPsfStackTexture(Scene::Scene& scene, WavefrontAberration& aberration, Scene::Object* owner)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "PSF Stack Texture");

			// Extract the corresponding psfs
			Aberration::PsfStackElements::PsfEntry const& psf = aberration.m_psfStack.m_psfs
				[aberration.m_psfPreview.m_psfId]
				[aberration.m_psfPreview.m_horizontalId]
				[aberration.m_psfPreview.m_verticalId]
				[aberration.m_psfPreview.m_lambdaId]
				[aberration.m_psfPreview.m_apertureId]
				[aberration.m_psfPreview.m_focusId];

			// Upload the the texture data
			uploadDisplayTextures(scene, aberration, owner->m_name, aberration.m_psfPreview.m_psfStack.m_texture, psf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadDisplayPsfTexture(Scene::Scene& scene, WavefrontAberration& aberration, Scene::Object* owner)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "PSF Texture");

			// Target defocus and diopter values
			float diopters = std::round_to_digits(1.0f / aberration.m_psfPreview.m_psf.m_value, aberration.m_psfPreview.m_psf.m_dioptresPrecision);
			float defocus = std::round_to_digits(aberration.m_psfPreview.m_psf.m_value, aberration.m_psfPreview.m_psf.m_dioptresPrecision);

			// Local aberration structure
			auto aberrationTmp = aberration;
			auto& psfStack = aberrationTmp.m_psfStack;
			auto const& previewSettings = aberrationTmp.m_psfPreview.m_psf;
			aberrationTmp.m_psfParameters.m_focusDistances = { 1.0f / previewSettings.m_focusDistance, 1.0f / previewSettings.m_focusDistance, 1 };
			aberrationTmp.m_psfParameters.m_incidentAnglesHorizontal = { previewSettings.m_horizontalAngle, previewSettings.m_horizontalAngle, 1 };
			aberrationTmp.m_psfParameters.m_incidentAnglesVertical = { previewSettings.m_verticalAngle, previewSettings.m_verticalAngle, 1 };
			aberrationTmp.m_psfParameters.m_objectDistances = { diopters, diopters, 1 };
			aberrationTmp.m_psfParameters.m_apertureDiameters = { previewSettings.m_aperture, previewSettings.m_aperture, 1 };
			aberrationTmp.m_psfParameters.m_lambdas = { previewSettings.m_lambda };
			aberrationTmp.m_psfParameters.m_manualDefocus = previewSettings.m_valueMode == PSFStackPreviewParameters::Psf::Defocus;
			aberrationTmp.m_psfParameters.m_desiredDefocus = defocus;
			aberrationTmp.m_psfParameters.m_manualCoefficients = previewSettings.m_useManualCoefficients;
			aberrationTmp.m_psfParameters.m_desiredCoefficients = previewSettings.m_manualCoefficients;
			aberrationTmp.m_psfParameters.m_desiredPupilRetinaDistance = previewSettings.m_manualPupilRetinaDistance;


			// Other aberration settings
			aberrationTmp.m_psfParameters.m_logProgress = false;
			aberrationTmp.m_psfParameters.m_logStats = false;
			aberrationTmp.m_psfParameters.m_logDebug = false;

			// Compute the PSF
			computePSFStack(scene, aberrationTmp, PsfStackComputation_Everything);

			// Extract the resulting psfs
			Aberration::PsfStackElements::PsfEntry const& psf = psfStack.m_psfs[0][0][0][0][0][0];

			// Upload the the texture data
			uploadDisplayTextures(scene, aberrationTmp, owner->m_name, aberration.m_psfPreview.m_psf.m_texture, psf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void displayPsf(Scene::Scene& scene, Scene::Object* object, float uvScale, std::string const& description, std::string const& textureName)
		{
			GPU::Texture& texture = scene.m_textures[object->m_name + textureName];

			ImVec2 const previewSize = ImVec2(192, 192);
			ImVec2 const fullSize = ImVec2(1024, 1024);

			ImVec2 const lower = ImVec2(0.5f, 0.5f) + ImVec2(-0.5f * uvScale, 0.5f * uvScale);
			ImVec2 const upper = ImVec2(0.5f, 0.5f) + ImVec2(0.5f * uvScale, -0.5f * uvScale);
			ImColor const tint = ImColor(255, 255, 255, 255);
			ImColor const border = ImColor(255, 255, 255, 255);

			ImGui::Image(&(texture.m_texture), previewSize, lower, upper, tint, border);
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text(description.c_str());
				ImGui::Image(&(texture.m_texture), fullSize, lower, upper, tint, border);
				ImGui::EndTooltip();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void displayPsf(Scene::Scene& scene, WavefrontAberration& aberration, Scene::Object* owner, float uvScale, 
			Aberration::PSFStackPreviewParameters::PsfTexture const& previewOptions)
		{
			displayPsf(scene, owner, uvScale, "Optimized PSF", previewOptions.m_fullRes);
			ImGui::SameLine();
			displayPsf(scene, owner, uvScale, "Optimized PSF (Downscaled)", previewOptions.m_lowRes);
			ImGui::SameLine();
			displayPsf(scene, owner, uvScale, "Optimized PSF (Projected)", previewOptions.m_projected);
		}

		////////////////////////////////////////////////////////////////////////////////
		void propertiesTable(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "PSF Parameters");

			// Extract the visualized PSF's parameters and entries
			auto const& psfParameters = PsfStackChartCommon::getDisplayPsfParametersEntry(scene, aberration);
			auto const& displayPsf = PsfStackChartCommon::getDisplayPsf(scene, aberration);

			#define MAKE_LABEL(L) { ImGui::TableNextColumn(); ImGui::TextDisabled(L); ImGui::TableNextColumn(); }
			#define MAKE_COLUMN(N, V, ...) { ImGui::TableNextColumn(); ImGui::Text(N); ImGui::TableNextColumn(); ImGui::Text(V, __VA_ARGS__); }

			if (ImGui::BeginTable("PsfProperties", 2))
			{
				// PSF parameters
				MAKE_LABEL("PSF Parameters");
				MAKE_COLUMN("Object-space Distance", "%f m", psfParameters.m_units.m_objectDistanceM);
				MAKE_COLUMN("Horizontal Angle", "%f deg", psfParameters.m_units.m_horizontalAngle);
				MAKE_COLUMN("Vertical Angle", "%f deg", psfParameters.m_units.m_verticalAngle);
				MAKE_COLUMN("Lambda", "%f MuM", psfParameters.m_units.m_lambdaMuM);
				MAKE_COLUMN("Aperture Diameter", "%f mm", psfParameters.m_units.m_apertureDiameterMM);
				MAKE_COLUMN("Focus Distance", "%f m", psfParameters.m_units.m_focusDistanceM);

				// Focal lengths
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				MAKE_LABEL("Focal Lengths");
				MAKE_COLUMN("Pupil-retina Distance", "%f MM", psfParameters.m_units.m_pupilRetinaDistanceMuM * 1e-3f);
				MAKE_COLUMN("Defocus", "%f (%f)", psfParameters.m_focus.m_defocusParam, psfParameters.m_focus.m_defocusUnits);
				MAKE_COLUMN("Image shift (aberration)", "%f MuM", psfParameters.m_focus.m_imageShiftAberrationMuM);
				MAKE_COLUMN("Image shift (object depth)", "%f MuM", psfParameters.m_focus.m_imageShiftObjectDepthMuM);
				MAKE_COLUMN("Image shift (total)", "%f MuM", psfParameters.m_focus.m_imageShiftMuM);

				// Units
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				MAKE_LABEL("Units");
				MAKE_COLUMN("s0", "%f", psfParameters.m_units.m_s0);
				MAKE_COLUMN("u0", "%f", psfParameters.m_units.m_u0);
				MAKE_COLUMN("Diffraction Unit", "%f", psfParameters.m_units.m_diffractionUnit);
				MAKE_COLUMN("Axial Diffraction Unit", "%f", psfParameters.m_units.m_axialDiffractionUnit);
				MAKE_COLUMN("Sampling Units", "%f", psfParameters.m_sampling.m_samplingUnits);
				MAKE_COLUMN("Sample Size (MuM)", "%f", psfParameters.m_sampling.m_samplingMuM);
				MAKE_COLUMN("Vnm Sample Size", "%f", psfParameters.m_enzSampling.m_sampling);
				MAKE_COLUMN("Vnm Terms (k)", "%d", psfParameters.m_enzSampling.m_terms);
				MAKE_COLUMN("Vnm Samples", "%d", psfParameters.m_enzSampling.m_samples);

				// Blur sizes
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				MAKE_LABEL("Blur Sizes");
				MAKE_COLUMN("Original Size", "%d px", psfParameters.m_sampling.m_samples);
				MAKE_COLUMN("Blur Size", "%d px, %f MuM, %f deg", displayPsf.m_kernelSizePx, displayPsf.m_blurSizeMuM, displayPsf.m_blurSizeDeg);
				MAKE_COLUMN("Projected Blur Radius", "%f px", blurRadiusPixels(displayPsf, aberration.m_psfPreview.m_resolution, aberration.m_psfPreview.m_fovy));

				ImGui::EndTable();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void copyTableToClipboard(Scene::Scene& scene, std::string const& tableName, ImGui::TableConfig const& conf)
		{
			std::stringstream ss;
			for (size_t colId = 0; colId < conf.cols.count; ++colId)
			{
				ss << conf.cols.headers[colId] << " = [";
				for (size_t rowId = 0; rowId < conf.rows.values.size(); ++rowId)
				{
					if (rowId > 0) ss << ", ";
					ss << conf.rows.values[rowId][colId];
				}
				ss << "]" << std::endl;
			}
			System::copyToClipboard(ss.str());
		}

		////////////////////////////////////////////////////////////////////////////////
		void saveTableToFile(Scene::Scene& scene, std::string const& tableName, ImGui::TableConfig const& conf)
		{
			std::stringstream ss;
			for (size_t colId = 0; colId < conf.cols.count; ++colId)
			{
				if (colId > 0) ss << ";";
				ss << conf.cols.headers[colId];
			}
			ss << std::endl;
			for (size_t rowId = 0; rowId < conf.rows.values.size(); ++rowId)
			{
				for (size_t colId = 0; colId < conf.cols.count; ++colId)
				{
					if (colId > 0) ss << ";";
					ss << conf.rows.values[rowId][colId];
				}
				ss << std::endl;
			}
			std::stringstream filePath;
			filePath << "Generated/";
			filePath << "Tables/";
			filePath << "WavefrontAberration/";
			filePath << EnginePaths::stringToFileName(tableName);
			filePath << ".txt";
			Asset::saveTextFile(scene, filePath.str(), ss);
		}

		////////////////////////////////////////////////////////////////////////////////
		void reconstructionTable(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Reconstruction");

			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Eye Parameters");

				ImGui::PushID("EyeParameters");

				// Table contents
				std::vector<std::vector<std::string>> contents;

				// Go through each row to and compute the text lengths
				for (auto const& param: aberration.m_psfStack.m_debugInformationCommon.m_eyeParameters)
				{
					std::vector<std::string> values;

					ImGui::TableNextColumn(); values.push_back(param.first);
					ImGui::TableNextColumn(); values.push_back(std::to_string(param.second));

					contents.emplace_back(values);
				}

				// Generate the actual table
				ImGui::TableConfig conf;
				conf.cols.count = 2;
				conf.cols.headers = { "Parameter", "Value" };
				conf.rows.values = contents;
				ImGui::Table("Eye Parameters", conf);

				// Also create a button to quickly copy the coefficient data
				// TODO: somehow incorporate this into our custom Tables API
				if (ImGui::Button("Copy")) copyTableToClipboard(scene, "Eye Parameters (Reconstruction)", conf);
				ImGui::SameLine();
				if (ImGui::Button("Save")) saveTableToFile(scene, "Eye Parameters (Reconstruction)", conf);

				ImGui::PopID();
			}

			ImGui::Dummy(ImVec2(0.0f, 15.0f));

			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Coefficients");

				ImGui::PushID("Coefficients");

				// Number of alpha and beta coefficients
				const int numCoefficientsAlpha = ZernikeCoefficientsAlpha::NUM_COEFFS;

				// Table contents
				std::vector<std::vector<std::string>> contents;

				// Go through each row to and compute the text lengths
				for (int i = 1; i <= numCoefficientsAlpha; ++i)
				{
					std::vector<std::string> values;
					#define SHOW_IF(CONDITION, FORMAT, ...) \
						if (CONDITION) { \
							static char s_buffer[128]; \
							sprintf_s(s_buffer, sizeof(s_buffer), FORMAT, __VA_ARGS__); \
							values.push_back(std::string(s_buffer)); \
						} else { \
							values.push_back(std::string()); \
						}
					const float errorNN = aberration.m_psfStack.m_debugInformationCommon.m_alphaTrueNN[i] - aberration.m_aberrationParameters.m_coefficients[i];
					const float errorRT = aberration.m_psfStack.m_debugInformationCommon.m_alphaTrueRT[i] - aberration.m_aberrationParameters.m_coefficients[i];
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%s", ZernikeIndices::s_zernikeNames[i].m_description.c_str());
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_ansi);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_noll);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_degree[0]);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_degree[1]);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", aberration.m_aberrationParameters.m_coefficients[i]);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", aberration.m_psfStack.m_debugInformationCommon.m_alphaTrueRT[i]);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", aberration.m_psfStack.m_debugInformationCommon.m_alphaTrueNN[i]);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", errorRT);
					ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", errorNN);

					contents.emplace_back(values);
				}

				// Generate the actual table
				ImGui::TableConfig conf;
				conf.cols.count = 10;
				conf.cols.headers = { "Coefficient", "ANSI", "Noll", "n", "m", "Target", "True (RT)", "True (NN)", "Error (RT)", "Error (NN)" };
				conf.rows.values = contents;
				ImGui::Table("Coefficients", conf);

				// Also create a button to quickly copy the coefficient data
				// TODO: somehow incorporate this into our custom Tables API
				if (ImGui::Button("Copy")) copyTableToClipboard(scene, "Coefficients (Reconstruction)", conf);
				ImGui::SameLine();
				if (ImGui::Button("Save")) saveTableToFile(scene, "Coefficients (Reconstruction)", conf);

				// Display the error charactestics
				std::pair<EyeEstimation::ErrorMetrics, EyeEstimation::ErrorMetrics> errorMetricsTarget = EyeEstimation::computeErrorMetricsCoefficients(
					aberration.m_aberrationParameters.m_coefficients,
					aberration.m_aberrationParameters.m_coefficients);
				std::pair<EyeEstimation::ErrorMetrics, EyeEstimation::ErrorMetrics> errorMetricsRT = EyeEstimation::computeErrorMetricsCoefficients(
					aberration.m_psfStack.m_debugInformationCommon.m_alphaTrueRT,
					aberration.m_aberrationParameters.m_coefficients);
				std::pair<EyeEstimation::ErrorMetrics, EyeEstimation::ErrorMetrics> errorMetricsNN = EyeEstimation::computeErrorMetricsCoefficients(
					aberration.m_psfStack.m_debugInformationCommon.m_alphaTrueNN,
					aberration.m_aberrationParameters.m_coefficients);

				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				ImGui::TextDisabled("Error statistics (Including Zeros)");
				ImGui::Text("Mean absolute value: %.6f", errorMetricsTarget.first.m_meanAbs);
				ImGui::Text("Mean absolute error: %.6f (RT), %.6f (NN)", errorMetricsRT.first.m_mae, errorMetricsNN.first.m_mae);
				ImGui::Text("Mean absolute percentage error: %.6f (RT), %.6f (NN)", errorMetricsRT.first.m_mape, errorMetricsNN.first.m_mape);

				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				ImGui::TextDisabled("Error statistics (Without Zeros)");
				ImGui::Text("Mean absolute value: %.6f", errorMetricsTarget.second.m_meanAbs);
				ImGui::Text("Mean absolute error: %.6f (RT), %.6f (NN)", errorMetricsRT.second.m_mae, errorMetricsNN.second.m_mae);
				ImGui::Text("Mean absolute percentage error: %.6f (RT), %.6f (NN)", errorMetricsRT.second.m_mape, errorMetricsNN.second.m_mape);

				ImGui::PopID();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void eyeParamsTable(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Eye Parameters");

			// Extract the visualized PSF's parameters
			auto const& psfParameters = PsfStackChartCommon::getDisplayPsfParametersEntry(scene, aberration);

			// Number of alpha and beta coefficients
			const int numCoefficientsAlpha = ZernikeCoefficientsAlpha::NUM_COEFFS;
			const int numCoefficientsBeta = numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees);

			// Table contents
			std::vector<std::vector<std::string>> contents;

			// Go through each row to and compute the text lengths
			for (auto const& param : psfParameters.m_debug.m_eyeParameters)
			{
				std::vector<std::string> values;

				ImGui::TableNextColumn(); values.push_back(param.first);
				ImGui::TableNextColumn(); values.push_back(std::to_string(param.second));

				contents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig conf;
			conf.cols.count = 2;
			conf.cols.headers = { "Parameter", "Value" };
			conf.rows.values = contents;
			ImGui::Table("Eye Parameters", conf);

			// Also create a button to quickly copy the coefficient data
			// TODO: somehow incorporate this into our custom Tables API
			if (ImGui::Button("Copy")) copyTableToClipboard(scene, "Eye Parameters (PSF)", conf);
			ImGui::SameLine();
			if (ImGui::Button("Save")) saveTableToFile(scene, "Eye Parameters (PSF)", conf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void coefficientsTable(Scene::Scene& scene, WavefrontAberration& aberration)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, "Coefficients");

			// Extract the visualized PSF's parameters
			auto const& psfParameters = PsfStackChartCommon::getDisplayPsfParametersEntry(scene, aberration);

			// Number of alpha and beta coefficients
			const int numCoefficientsAlpha = ZernikeCoefficientsAlpha::NUM_COEFFS;
			const int numCoefficientsBeta = numZernikeCoefficients(aberration.m_psfParameters.m_betaDegrees);

			// Table contents
			std::vector<std::vector<std::string>> contents;

			// Go through each row to and compute the text lengths
			for (int i = 1; i <= numCoefficientsBeta; ++i)
			{
				std::vector<std::string> values;
				#define SHOW_IF(CONDITION, FORMAT, ...) \
					if (CONDITION) { \
						static char s_buffer[128]; \
						sprintf_s(s_buffer, sizeof(s_buffer), FORMAT, __VA_ARGS__); \
						values.push_back(std::string(s_buffer)); \
					} else { \
						values.push_back(std::string()); \
					}

				const float error = i <= numCoefficientsAlpha ? psfParameters.m_debug.m_alphaTrue[i] - psfParameters.m_coefficients.m_alpha[i] : 0.0f;
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%s", ZernikeIndices::s_zernikeNames[i].m_description.c_str());
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_ansi);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_noll);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_degree[0]);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%2d", ZernikeIndices::s_zernikeNames[i].m_degree[1]);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", psfParameters.m_debug.m_alphaTrue[i]);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", psfParameters.m_coefficients.m_alpha[i]);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", error);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", psfParameters.m_coefficients.m_alphaPhaseCumulative[i]);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsAlpha, "%7.4f", psfParameters.m_coefficients.m_alphaPhaseResidual[i]);
				ImGui::TableNextColumn(); SHOW_IF(i <= numCoefficientsBeta, "%7.4f+%7.4fi", psfParameters.m_coefficients.m_beta[i].real(), psfParameters.m_coefficients.m_beta[i].imag());

				contents.emplace_back(values);
			}

			// Generate the actual table
			ImGui::TableConfig conf;
			conf.cols.count = 11;
			conf.cols.headers = { "Coefficient", "ANSI", "Noll", "n", "m", "True", "OPD", "Error", "Phase (Cum.)", "Phase (Res.)", "Beta" };
			conf.rows.values = contents;
			ImGui::Table("Coefficients", conf);

			// Also create a button to quickly copy the coefficient data
			// TODO: somehow incorporate this into our custom Tables API
			if (ImGui::Button("Copy")) copyTableToClipboard(scene, "Coefficients (PSF)", conf);
			ImGui::SameLine();
			if (ImGui::Button("Save")) saveTableToFile(scene, "Coefficients (PSF)", conf);

			// Display the error charactestics
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			ImGui::TextDisabled("Error statistics");


			// Display the error charactestics
			std::pair<EyeEstimation::ErrorMetrics, EyeEstimation::ErrorMetrics> errorMetricsCoeffs = EyeEstimation::computeErrorMetricsCoefficients(
				psfParameters.m_debug.m_alphaTrue, psfParameters.m_coefficients.m_alpha);
			EyeEstimation::ErrorMetrics errorMetricsTotal = EyeEstimation::computeErrorMetricsCoefficientsStack(
				scene, aberration);
			const float errorFocus = psfParameters.m_units.m_focusDistanceM - psfParameters.m_debug.m_focusDistance;
			EyeEstimation::ErrorMetrics errorFocusTotal = EyeEstimation::computeErrorMetricsFocusStack(
				scene, aberration);
			EyeEstimation::ErrorMetrics errorFocusParamsTotal = EyeEstimation::computeErrorMetricsFocusParamsStack(
				scene, aberration);
			EyeEstimation::ErrorMetrics errorFocusParamsDeltaTotal = EyeEstimation::computeErrorMetricsFocusParamsDeltaStack(
				scene, aberration);

			//ImGui::Text("Focus distance error: %.6f [stack: %.6f, perc.: %.6f]", errorFocus, errorFocusTotal.m_mae, errorFocusTotal.m_mape);
			ImGui::Text("Focus parameters mean absolute value: %.6f", errorFocusParamsTotal.m_meanAbs);
			ImGui::Text("Focus parameters error: %.6f [stack: %.6f, perc.: %.6f]", 0.0f, errorFocusParamsTotal.m_mae, errorFocusParamsTotal.m_mape);
			ImGui::Text("Delta focus parameters mean absolute value: %.6f", errorFocusParamsDeltaTotal.m_meanAbs);
			ImGui::Text("Delta focus parameters error: %.6f [stack: %.6f, perc.: %.6f]", 0.0f, errorFocusParamsDeltaTotal.m_mae, errorFocusParamsDeltaTotal.m_mape);
			ImGui::Text("Mean absolute value: %.6f [stack: %.6f]", errorMetricsCoeffs.first.m_meanAbs, errorMetricsTotal.m_meanAbs);
			ImGui::Text("Mean absolute error: %.6f [stack: %.6f]", errorMetricsCoeffs.first.m_mae, errorMetricsTotal.m_mae);
			ImGui::Text("Mean absolute percentage error: %.6f [stack: %.6f]", errorMetricsCoeffs.first.m_mape, errorMetricsTotal.m_mape);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool generateGuiParameterRange(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration, const char* label, PSFStackParameters::ParameterRange& range)
	{
		ImGui::PushID(label);
		ImGui::BeginGroup();
		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());

		bool value_changed = false;
		ImGui::DragFloat("##min", &range.m_min, 0.01f, 0.01f, 100.0f, "Min: %.3f"); value_changed |= ImGui::IsItemDeactivatedAfterEdit();
		ImGui::PopItemWidth();
		ImGui::SameLineWithinComposite();

		ImGui::DragFloat("##max", &range.m_max, 0.01f, 0.01f, 100.0f, "Max: %.3f"); value_changed |= ImGui::IsItemDeactivatedAfterEdit();
		ImGui::PopItemWidth();
		ImGui::SameLineWithinComposite();

		ImGui::DragInt("##step", &range.m_numSteps, 1, 1, 100, "Steps: %d"); value_changed |= ImGui::IsItemDeactivatedAfterEdit();
		ImGui::PopItemWidth();
		ImGui::SameLineWithinComposite();

		ImGui::TextEx(label, ImGui::FindRenderedTextEnd(label));

		ImGui::EndGroup();
		ImGui::PopID();

		return value_changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	// TODO: this would probably look better as a TreeView (in a Table)
	bool generateCoefficientEditor(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, ZernikeCoefficientsAlpha& coefficients, bool editable, bool onlyReportCommit)
	{
		bool result = false;
		bool resultCommitted = false;

		// Zernike coeffs
		int currentOrder = -1;
		bool showTreeNode = false;
		for (int i = 1; i <= ZernikeCoefficientsAlpha::NUM_COEFFS; ++i)
		{
			// Properties of the current coeff
			auto const& props = ZernikeIndices::s_zernikeNames[i];

			// Handle separation for the individual Zernike polynomial degrees
			if (props.m_degree[0] != currentOrder)
			{
				if (currentOrder != -1 && showTreeNode)
					ImGui::TreePop();
				currentOrder = props.m_degree[0];
				const std::string name = "Order " + std::to_string(currentOrder);
				showTreeNode = ImGui::TreeNodeEx(name.c_str());
			}

			// If the current order is shown, then show its properties
			if (showTreeNode)
			{
				std::stringstream labelSS;
				labelSS << "Z" << "[" << props.m_degree[0] << ", " << props.m_degree[1] << "]";
				labelSS << " " << "(" << "A: " << props.m_ansi << ", " << "N: " << props.m_noll << ")";
				labelSS << " " << props.m_description;
				std::string label = labelSS.str();

				// If it can be edited, then use a Draggable widget
				if (editable)
				{
					result |= ImGui::DragFloat(label.c_str(), &coefficients[i], 0.005f, -100.0f, 100.0f);
					resultCommitted |= ImGui::IsItemDeactivatedAfterEdit();
				}

				// Otherwise manually show the value
				else
				{
					if (ImGui::BeginTable(props.m_name.c_str(), 2))
					{
						ImGui::TableSetupColumn("Name", 0, ImGui::CalcTextSize("#").x * 40.0f + ImGui::GetStyle().ItemSpacing.x);
						ImGui::TableSetupColumn("Val", 0, ImGui::CalcTextSize("#").x * 12.0f);

						ImGui::TableNextColumn();
						ImGui::Text(label.c_str());
						ImGui::TableNextColumn();
						ImGui::Text("%.5f", coefficients[i]);

						ImGui::EndTable();
					}
				}
			}
		}

		// Close the category if we are currently inside one
		if (currentOrder != -1 && showTreeNode)
			ImGui::TreePop();

		return onlyReportCommit ? resultCommitted : result;
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace DataExport
	{
		////////////////////////////////////////////////////////////////////////////////
		meta_enum(ExportType, int, Coefficients, FocusChart);

		////////////////////////////////////////////////////////////////////////////////
		std::string coefficientsOutputDefault(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration)
		{
			std::stringstream result;
			result << aberration.m_name << "_";
			result << "Coefficients";
			result << ".csv";
			return result.str();
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string focusChartOutputDefault(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration)
		{
			std::stringstream result;
			result << aberration.m_name << "_";
			result << "FocusChart" << "_";
			result << std::to_string(PSFStackPreviewParameters::Focus::DisplayAxis_value_to_string(
				aberration.m_psfPreview.m_focus.m_displayAxis));
			result << ".csv";
			return result.str();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateCoefficientsArea(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration)
		{
			ImGui::InputText("File Name", EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_DataExport_Coefficients_FileName"));

			if (ImGui::ButtonEx("Ok", "|########|"))
			{
				// TODO: actually export
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::ButtonEx("Cancel", "|########|"))
			{
				ImGui::CloseCurrentPopup();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateFocusChartArea(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration)
		{
			ImGui::InputText("File Name", EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_DataExport_FocusChart_FileName"));

			if (ImGui::ButtonEx("Ok", "|########|"))
			{
				FocusChart::exportData(scene, aberration, EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_DataExport_FocusChart_FileName"));
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::ButtonEx("Cancel", "|########|"))
			{
				ImGui::CloseCurrentPopup();
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void generatePopup(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration)
		{
			// The current export type
			int& activeExportType = EditorSettings::editorProperty<int>(scene, owner, "Aberration_DataExport_ActiveExportType");

			// Coefficient selector & default settings
			if (ImGui::RadioButton("Coefficients", &activeExportType, int(ExportType::Coefficients)))
			{
				EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_DataExport_Coefficients_FileName") =
					coefficientsOutputDefault(scene, guiSettings, owner, aberration);
			}

			// Focus chart selector & default settings
			if (ImGui::RadioButton("Focus Chart", &activeExportType, int(ExportType::FocusChart)))
			{
				EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_DataExport_FocusChart_FileName") =
					focusChartOutputDefault(scene, guiSettings, owner, aberration);
			}

			ImGui::Separator();

			// Show the current export label
			std::string label = std::string(ExportType_value_to_string(ExportType(activeExportType)));
			ImGui::TextDisabled("%s settings", label.c_str());

			// Generate the export area
			switch (activeExportType)
			{
			case ExportType::Coefficients:
				generateCoefficientsArea(scene, guiSettings, owner, aberration);
				break;

			case ExportType::FocusChart:
				generateFocusChartArea(scene, guiSettings, owner, aberration);
				break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	std::array<bool, 2> generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* owner, WavefrontAberration& aberration, WavefrontAberrationPresets& presets)
	{
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);

		bool coreChanged = false, aberrationChanged = false;
		bool psfDisplayChanged = 
			aberration.m_psfPreview.m_psf.m_texture.m_lastUploadFrameId <
			aberration.m_psfStack.m_debugInformationCommon.m_lastComputedFrameId;
		bool psfStackDisplayChanged = 
			aberration.m_psfPreview.m_psfStack.m_texture.m_lastUploadFrameId <
			aberration.m_psfStack.m_debugInformationCommon.m_lastComputedFrameId;

		if (!ImGui::BeginTabBar("Aberration")) return { false, false };

		std::string activeTab;
		if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, owner, "Aberration_SelectedTab#Synced"); activeTabSynced.has_value())
			activeTab = activeTabSynced.value();

		if (ImGui::BeginTabItem("Eye Parameters", activeTab.c_str()))
		{
			ImGui::TextDisabled("Measurement Settings");
			ImGui::SliderFloat("Refractive Index", &aberration.m_refractiveIndex, 0.0f, 10.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderFloat("Aperture Diameter", &aberration.m_aberrationParameters.m_apertureDiameter, 3.0f, 6.0f, "%.3f mm"); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderFloat("Measurement Lambda", &aberration.m_aberrationParameters.m_lambda, 400.0f, 800.0f, "%.3f nm"); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::Separator();

			ImGui::TextDisabled("Aberration Coefficients");
			aberrationChanged |= ImGui::Combo("Aberration Type", &aberration.m_aberrationParameters.m_type, AberrationParameters::AberrationType_meta);
			if (aberration.m_aberrationParameters.m_type == AberrationParameters::Spectacle)
			{
				aberration.m_aberrationParameters.m_lambda = 587.56f;
			}

			// Spectacle editor
			if (aberration.m_aberrationParameters.m_type == AberrationParameters::Spectacle)
			{
				ImGui::DragFloat("Sphere", &aberration.m_aberrationParameters.m_spectacleLens.m_sphere, 0.01f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
				ImGui::DragFloat("Cylinder", &aberration.m_aberrationParameters.m_spectacleLens.m_cylinder, 0.01f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
				ImGui::SliderAngle("Axis", &aberration.m_aberrationParameters.m_spectacleLens.m_axis, 0.0f, 180.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;

				if (ImGui::Button("Save"))
				{
					EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SavePreset_Name") = aberration.m_name;
					ImGui::OpenPopup("SaveAberrationPreset");
				}
			}

			// Aberration preset
			if (aberration.m_aberrationParameters.m_type == AberrationParameters::Preset)
			{
				if (ImGui::Combo("Preset", aberration.m_name, presets))
				{
					auto const& aberrationPreset = presets[aberration.m_name];
					aberration.m_aberrationParameters = aberrationPreset.m_aberrationParameters;
					aberrationChanged = true;
				}

				ImGui::SameLine();

				if (ImGui::Button("Save"))
				{
					EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SavePreset_Name") = aberration.m_name;
					ImGui::OpenPopup("SaveAberrationPreset");
				}
			}

			if (ImGui::BeginPopup("SaveAberrationPreset"))
			{
				std::string& presetName = EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SavePreset_Name");
				ImGui::InputText("Preset Name", presetName);

				// Store the preset and save it to disk on save
				if (ImGui::ButtonEx("Ok", "|########|"))
				{
					auto& preset = presets[presetName];
					preset.m_name = presetName;
					preset.m_aberrationParameters.m_type = AberrationParameters::Preset;
					preset.m_aberrationParameters.m_apertureDiameter = aberration.m_aberrationParameters.m_apertureDiameter;
					preset.m_aberrationParameters.m_lambda = aberration.m_aberrationParameters.m_lambda;
					preset.m_aberrationParameters.m_coefficients = aberration.m_aberrationParameters.m_coefficients;
					AberrationPreset::save(scene, presetName, preset);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();

				// Simply close the dialog otherwise
				if (ImGui::ButtonEx("Cancel", "|########|"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			// Zernike coeffs
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
			ImGui::TextDisabled("Coefficients");
			aberrationChanged |= generateCoefficientEditor(scene, guiSettings, owner, aberration.m_aberrationParameters.m_coefficients,
				aberration.m_aberrationParameters.m_type == AberrationParameters::Preset, true);

			ImGui::EndTabItem();
			EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SelectedTab") = ImGui::CurrentTabItemName();
		}

		if (ImGui::BeginTabItem("Reconstruction", activeTab.c_str()))
		{
			ImGui::TextDisabled("Eye Estimation");
			aberrationChanged = ImGui::Combo("Eye Estimation Method", &aberration.m_psfParameters.m_eyeEstimationMethod, Aberration::PSFStackParameters::EyeEstimationMethod_meta) || aberrationChanged;

			if (aberration.m_psfParameters.m_eyeEstimationMethod == Aberration::PSFStackParameters::Matlab)
			{
				ImGui::SliderInt("Number of Rays", &aberration.m_reconstructionParameters.m_numRays, 0, 2048); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
				ImGui::SliderFloat("Time Limit", &aberration.m_reconstructionParameters.m_timeLimit, 0, 1200.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
				ImGui::SliderFloat("Anatomical Weight (Boundary)", &aberration.m_reconstructionParameters.m_anatomicalWeightBoundary, 0, 100.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
				ImGui::SliderFloat("Anatomical Weight (Average)", &aberration.m_reconstructionParameters.m_anatomicalWeightAverage, 0, 100.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
				ImGui::SliderFloat("Functional Weight (Specified)", &aberration.m_reconstructionParameters.m_functionalWeightSpecified, 0, 100.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
				ImGui::SliderFloat("Functional Weight (Unspecified)", &aberration.m_reconstructionParameters.m_functionalWeightUnspecified, 0, 100.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
				coreChanged |= ImGui::Combo("Optimizer", &aberration.m_reconstructionParameters.m_optimizer, Aberration::EyeReconstructionParameters::Optimizer_meta);
				coreChanged |= ImGui::Combo("Solver", &aberration.m_reconstructionParameters.m_solver, Aberration::EyeReconstructionParameters::Solver_meta);
			}
			else if (aberration.m_psfParameters.m_eyeEstimationMethod == Aberration::PSFStackParameters::NeuralNetworks)
			{
				coreChanged |= ImGui::Checkbox("Force On-Axis Network", &aberration.m_psfParameters.m_forceOnAxisNetwork);
			}

			ImGui::EndTabItem();
			EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SelectedTab") = ImGui::CurrentTabItemName();
		}

		if (ImGui::BeginTabItem("Computation", activeTab.c_str()))
		{
			ImGui::TextDisabled("Computation Settings");
			aberrationChanged |= (ImGui::Combo("Backend", &aberration.m_psfParameters.m_backend, Aberration::PSFStackParameters::ComputationBackend_meta));
			ImGui::Checkbox("Log Progress", &aberration.m_psfParameters.m_logProgress);
			ImGui::SameLine();
			ImGui::Checkbox("Log Stats", &aberration.m_psfParameters.m_logStats);
			ImGui::SameLine();
			ImGui::Checkbox("Log Debug", &aberration.m_psfParameters.m_logDebug);

			ImGui::Checkbox("Truncate Timings", &aberration.m_psfParameters.m_truncateStatsTiming);
			ImGui::SameLine();
			ImGui::Checkbox("Shorten Timings", &aberration.m_psfParameters.m_shortenStatsTiming);
			ImGui::SameLine();
			ImGui::Checkbox("Collect Debug Info", &aberration.m_psfParameters.m_collectDebugInfo);

			ImGui::Checkbox("Omit Vnms", &aberration.m_psfParameters.m_omitVnmCalculation);
			ImGui::SameLine();
			ImGui::Checkbox("Omit PSFs", &aberration.m_psfParameters.m_omitPsfCalculation);

			ImGui::Separator();
			ImGui::TextDisabled("Parameter Domains");

			ImGui::SliderFloatN("Wavelengths", aberration.m_psfParameters.m_lambdas.data(), aberration.m_psfParameters.m_lambdas.size(), 400.0f, 800.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			generateGuiParameterRange(scene, guiSettings, owner, aberration, "Object Distances (Dioptres)", aberration.m_psfParameters.m_objectDistances); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			generateGuiParameterRange(scene, guiSettings, owner, aberration, "Horizontal Angles (Deg)", aberration.m_psfParameters.m_incidentAnglesHorizontal); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			generateGuiParameterRange(scene, guiSettings, owner, aberration, "Vertical Angles (Deg)", aberration.m_psfParameters.m_incidentAnglesVertical); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			generateGuiParameterRange(scene, guiSettings, owner, aberration, "Aperture Diameters (mm)", aberration.m_psfParameters.m_apertureDiameters); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			generateGuiParameterRange(scene, guiSettings, owner, aberration, "Focus Distances (Dioptres)", aberration.m_psfParameters.m_focusDistances); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;

			ImGui::EndTabItem();
			EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SelectedTab") = ImGui::CurrentTabItemName();
		}

		if (ImGui::BeginTabItem("Sampling", activeTab.c_str()))
		{
			ImGui::TextDisabled("Alpha to Beta");
			coreChanged |= ImGui::Combo("Source", &aberration.m_psfParameters.m_alphaToBetaCoefficient, Aberration::PSFStackParameters::CoefficientVariation_meta);
			ImGui::SliderInt("Beta Degrees", &aberration.m_psfParameters.m_betaDegrees, 0, 35); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderFloat("Beta Threshold", &aberration.m_psfParameters.m_betaThreshold, 0.0f, 1.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			coreChanged |= ImGui::Combo("L Sampling", &aberration.m_psfParameters.m_alphaToBetaLSampling, Aberration::PSFStackParameters::PupilSampling_meta);
			coreChanged |= ImGui::Combo("K Sampling", &aberration.m_psfParameters.m_alphaToBetaKSampling, Aberration::PSFStackParameters::PupilSampling_meta);
			ImGui::SliderInt("L", &aberration.m_psfParameters.m_alphaToBetaL, 0, 500); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderInt("K", &aberration.m_psfParameters.m_alphaToBetaK, 0, 500); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;

			ImGui::Separator();
			ImGui::TextDisabled("Vnm Approximation");
			
			ImGui::SliderInt("Approximation Samples", &aberration.m_psfParameters.m_maxApproximationSamples, 0, 8192); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderFloat("Approximation Sample Size", &aberration.m_psfParameters.m_approximationSampleSize, 0.01, 0.5f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderInt("Approximation Terms Minimum", &aberration.m_psfParameters.m_approximationTermsMin, 0, 100); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderInt("Approximation Terms Maximum", &aberration.m_psfParameters.m_approximationTermsMax, 0, 1000); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderFloat("Approximation Terms Multiplier", &aberration.m_psfParameters.m_approximationTermsMultiplier, 0.0f, 4.0f); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			ImGui::SliderInt("Bessel Batch Size", &aberration.m_psfParameters.m_besselBatchSize, 1, 400); coreChanged = ImGui::IsItemDeactivatedAfterEdit() || coreChanged;
			coreChanged |= ImGui::Checkbox("Cache Vnm Inner Terms", &aberration.m_psfParameters.m_precomputeVnmLSum);

			ImGui::Separator();

			ImGui::TextDisabled("PSF Sampling");
			ImGui::SliderFloat("Sampling Units Min", &aberration.m_psfParameters.m_minSamplingUnits, 0, 64.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderFloat("Sampling Units Max", &aberration.m_psfParameters.m_maxSamplingUnits, 0, 10000.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderFloat("PSF Sample Size Multiplier", &aberration.m_psfParameters.m_psfSampleSizeMultiplier, 0.0f, 8.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderFloat("PSF Sample Count Multiplier", &aberration.m_psfParameters.m_psfSampleCountMultiplier, 0, 4.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderInt("PSF Samples Min", &aberration.m_psfParameters.m_psfSamplesMin, 1, 1 << 16); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderInt("PSF Samples Max", &aberration.m_psfParameters.m_psfSamplesMax, 1, 1 << 16); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;

			ImGui::Separator();

			ImGui::TextDisabled("PSF Optimization");
			ImGui::SliderFloat("Crop Threshold (Sum)", &aberration.m_psfParameters.m_cropThresholdSum, 0.0f, 1.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			ImGui::SliderFloat("Crop Threshold (Coeff)", &aberration.m_psfParameters.m_cropThresholdCoeff, 0.0f, 1.0f); aberrationChanged = ImGui::IsItemDeactivatedAfterEdit() || aberrationChanged;
			aberrationChanged = ImGui::Combo("Interpolation Method", &aberration.m_psfParameters.m_interpolationType, Aberration::PSFStackParameters::InterpolationType_meta) || aberrationChanged;

			ImGui::EndTabItem();
			EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SelectedTab") = ImGui::CurrentTabItemName();
		}

		if (!aberration.m_psfStack.m_psfEntryParameters.empty() && ImGui::BeginTabItem("Preview", activeTab.c_str()))
		{
			psfStackDisplayChanged |= ImGui::Combo("Render Resolution", &aberration.m_psfPreview.m_resolutionId, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolutionNames);
			aberration.m_psfPreview.m_resolution = RenderSettings::getResolutionById(scene, renderSettings, aberration.m_psfPreview.m_resolutionId);
			psfStackDisplayChanged |= ImGui::SliderAngle("Field of View", &aberration.m_psfPreview.m_fovy, 50.0f, 75.0f);
			psfStackDisplayChanged |= ImGui::SliderInt("Display PSF ID", &aberration.m_psfPreview.m_psfId, 0, aberration.m_psfStack.m_psfs.size() - 1);
			psfStackDisplayChanged |= ImGui::SliderInt("Display Horizontal ID", &aberration.m_psfPreview.m_horizontalId, 0, aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesHorizontal.size() - 1);
			psfStackDisplayChanged |= ImGui::SliderInt("Display Vertical ID", &aberration.m_psfPreview.m_verticalId, 0, aberration.m_psfParameters.m_evaluatedParameters.m_incidentAnglesVertical.size() - 1);
			psfStackDisplayChanged |= ImGui::SliderInt("Display Lambda ID", &aberration.m_psfPreview.m_lambdaId, 0, aberration.m_psfParameters.m_lambdas.size() - 1);
			psfStackDisplayChanged |= ImGui::SliderInt("Display Aperture ID", &aberration.m_psfPreview.m_apertureId, 0, aberration.m_psfParameters.m_evaluatedParameters.m_apertureDiameters.size() - 1);
			psfStackDisplayChanged |= ImGui::SliderInt("Display Focus ID", &aberration.m_psfPreview.m_focusId, 0, aberration.m_psfParameters.m_evaluatedParameters.m_focusDistances.size() - 1);

			ImGui::Dummy(ImVec2(0.0f, 15.0f));

			if (ImGui::BeginChild("VisualizationContentArea", ImVec2(0.0f, 0.0f)))
			{
				if (ImGui::TreeNodeEx("Visualization", &EditorSettings::editorProperty<bool>(scene, owner, "Aberration_PreviewTreesOpen_Visualization")))
				{
					if (ImGui::BeginTabBar("VisualizationTabBar"))
					{
						// Restore the selected tab id
						std::string activeTab;
						if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, owner, "Aberration_PreviewTabBar_SelectedTab#Synced"); activeTabSynced.has_value())
							activeTab = activeTabSynced.value();

						// PSF stack
						if (ImGui::BeginTabItem("PSF Stack", activeTab.c_str()))
						{
							if (ImGui::BeginChild("PsfStackContentArea", ImVec2(0.0f, 200.0f)))
							{
								// UV scale factors
								int psfSize = PsfStackChartCommon::getDisplayPsf(scene, aberration).m_kernelSizePx;
								int targetSize = 0;
								forEachPsfStackIndex(scene, aberration, [&](auto& scene, auto& aberration, PsfIndex const& psfIndex)
								{
									targetSize = glm::max(targetSize, getPsfEntry(scene, aberration, psfIndex).m_kernelSizePx);
								});
								float uvScale = aberration.m_psfPreview.m_psfStack.m_sameScale ? float(targetSize) / float(psfSize) : 1.0f;
								PsfPreview::displayPsf(scene, aberration, owner, uvScale, aberration.m_psfPreview.m_psfStack.m_texture);
							}
							ImGui::EndChild();

							//if (ImGui::BeginChild("PsfStackSettingsContentArea", ImVec2(0.0f, 0.0f)))
							{
								psfStackDisplayChanged |= ImGui::SliderFloat("Display Radius", &aberration.m_psfPreview.m_psfStack.m_texture.m_radius, 0, 63);
								ImGui::Checkbox("Same Scale", &aberration.m_psfPreview.m_psfStack.m_sameScale);
							}
							//ImGui::EndChild();

							EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_PreviewTabBar_SelectedTab") = ImGui::CurrentTabItemName();
							ImGui::EndTabItem();
						}
						else
						{
							psfStackDisplayChanged = false;
						}

						// Custom PSF computation
						if (ImGui::BeginTabItem("PSF", activeTab.c_str()))
						{
							if (ImGui::BeginChild("PsfContentArea", ImVec2(0.0f, 200.0f)))
							{
								PsfPreview::displayPsf(scene, aberration, owner, 1.0f, aberration.m_psfPreview.m_psf.m_texture);
							}
							ImGui::EndChild();

							if (ImGui::BeginChild("PsfSettingsContentArea", ImVec2(0.0f, 0.0f)))
							{
								psfDisplayChanged |= ImGui::SliderFloat("Display Radius", &aberration.m_psfPreview.m_psf.m_texture.m_radius, 0, 63);
								psfDisplayChanged |= ImGui::SliderInt("Dioptres Precision", &aberration.m_psfPreview.m_psf.m_dioptresPrecision, 0, 8);
								psfDisplayChanged |= ImGui::Combo("Value Mode", &aberration.m_psfPreview.m_psf.m_valueMode, PSFStackPreviewParameters::Psf::ValueMode_meta);
								const char* valueLabel, * limitsLabel;
								switch (aberration.m_psfPreview.m_psf.m_valueMode)
								{
								case PSFStackPreviewParameters::Psf::Defocus:
									valueLabel = "Display Defocus";
									limitsLabel = "Display Defocus Range";
									break;

								case PSFStackPreviewParameters::Psf::Depth:
									valueLabel = "Display Depth";
									limitsLabel = "Display Depth Range";
									break;
								}
								psfDisplayChanged |= ImGui::SliderFloat(valueLabel, &aberration.m_psfPreview.m_psf.m_value, aberration.m_psfPreview.m_psf.m_valueLimits.x, aberration.m_psfPreview.m_psf.m_valueLimits.y);
								psfDisplayChanged |= ImGui::SliderFloat("Horizontal Angle", &aberration.m_psfPreview.m_psf.m_horizontalAngle, -60.0f, 60.0f);
								psfDisplayChanged |= ImGui::SliderFloat("Vertical Angle", &aberration.m_psfPreview.m_psf.m_verticalAngle, -45.0f, 45.0f);
								psfDisplayChanged |= ImGui::SliderFloat("Lambda", &aberration.m_psfPreview.m_psf.m_lambda, 400.0f, 700.0f);
								psfDisplayChanged |= ImGui::SliderFloat("Aperture Diameter", &aberration.m_psfPreview.m_psf.m_aperture, 2.0f, 7.0f);
								psfDisplayChanged |= ImGui::SliderFloat("Focus Distance", &aberration.m_psfPreview.m_psf.m_focusDistance, 0.25f, 8.0f);
								ImGui::DragFloatRange2(limitsLabel, glm::value_ptr(aberration.m_psfPreview.m_psf.m_valueLimits), 0.001f, 0.0f); psfDisplayChanged |= ImGui::IsItemDeactivatedAfterEdit();
								psfDisplayChanged |= ImGui::Checkbox("Manual Coefficients", &aberration.m_psfPreview.m_psf.m_useManualCoefficients);

								if (aberration.m_psfPreview.m_psf.m_useManualCoefficients)
								{
									// Focal lengths
									if (ImGui::TreeNode("Focal Lengths"))
									{
										psfDisplayChanged |= ImGui::SliderFloat("Pupil Retina Distance Length", &aberration.m_psfPreview.m_psf.m_manualPupilRetinaDistance, 15, 25.0f);

										ImGui::TreePop();
									}

									// Coefficients
									psfDisplayChanged |= generateCoefficientEditor(scene, guiSettings, owner, 
										aberration.m_psfPreview.m_psf.m_manualCoefficients, true, false);
								}
							}
							ImGui::EndChild();

							EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_PreviewTabBar_SelectedTab") = ImGui::CurrentTabItemName();
							ImGui::EndTabItem();
						}
						else
						{
							psfDisplayChanged = false;
						}

						// Coefficients
						if (ImGui::BeginTabItem("Coefficients", activeTab.c_str()))
						{
							ImGui::Combo("Visualization Method", &aberration.m_psfPreview.m_coefficient.m_visualizationMethod, PSFStackPreviewParameters::Coefficient::VisualizationMethod_meta);

							CoefficientChart::generateChart(scene, guiSettings, owner, aberration);

							EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_PreviewTabBar_SelectedTab") = ImGui::CurrentTabItemName();
							ImGui::EndTabItem();
						}

						// Focus charts
						if (ImGui::BeginTabItem("Focus", activeTab.c_str()))
						{
							ImGui::Combo("Display Axis", &aberration.m_psfPreview.m_focus.m_displayAxis, PSFStackPreviewParameters::Focus::DisplayAxis_meta);
							ImGui::Combo("Visualization Method", &aberration.m_psfPreview.m_focus.m_visualizationMethod, PSFStackPreviewParameters::Focus::VisualizationMethod_meta);

							FocusChart::generateChart(scene, guiSettings, owner, aberration);

							if (ImGui::Button("Export"))
							{
								EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_FocusChart_FileName") =
									aberration.m_name + "_" +
									std::to_string(PSFStackPreviewParameters::Focus::DisplayAxis_value_to_string(aberration.m_psfPreview.m_focus.m_displayAxis)) + ".ini";
								ImGui::OpenPopup("FocusChart_Export");
							}

							if (ImGui::BeginPopup("FocusChart_Export"))
							{
								ImGui::InputText("File Name", EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_FocusChart_FileName"));

								if (ImGui::ButtonEx("Ok", "|########|"))
								{
									FocusChart::exportData(scene, aberration, EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_FocusChart_FileName"));
									ImGui::CloseCurrentPopup();
								}
								ImGui::SameLine();
								if (ImGui::ButtonEx("Cancel", "|########|"))
								{
									ImGui::CloseCurrentPopup();
								}

								ImGui::EndPopup();
							}

							EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_PreviewTabBar_SelectedTab") = ImGui::CurrentTabItemName();
							ImGui::EndTabItem();
						}

						ImGui::EndTabBar();
					}
					ImGui::TreePop();
				}
				else
				{
					psfStackDisplayChanged = false;
					psfDisplayChanged = false;
				}

				if (ImGui::TreeNodeEx("Properties", &EditorSettings::editorProperty<bool>(scene, owner, "Aberration_PreviewTreesOpen_Properties")))
				{
					PsfPreview::propertiesTable(scene, aberration);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Reconstruction", &EditorSettings::editorProperty<bool>(scene, owner, "Aberration_PreviewTreesOpen_Reconstruction")))
				{
					PsfPreview::reconstructionTable(scene, aberration);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Eye Parameters", &EditorSettings::editorProperty<bool>(scene, owner, "Aberration_PreviewTreesOpen_EyeParameters")))
				{
					PsfPreview::eyeParamsTable(scene, aberration);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Coefficients", &EditorSettings::editorProperty<bool>(scene, owner, "Aberration_PreviewTreesOpen_Coefficients")))
				{
					PsfPreview::coefficientsTable(scene, aberration);
					ImGui::TreePop();
				}
			}
			ImGui::EndChild();

			if (ImGui::Button("Export"))
			{
				ImGui::OpenPopup("Aberration_DataExport");
			}

			if (ImGui::BeginPopup("Aberration_DataExport"))
			{
				DataExport::generatePopup(scene, guiSettings, owner, aberration);

				ImGui::EndPopup();
			}

			ImGui::EndTabItem();
			EditorSettings::editorProperty<std::string>(scene, owner, "Aberration_SelectedTab") = ImGui::CurrentTabItemName();
		}
		else
		{
			psfStackDisplayChanged = false;
			psfDisplayChanged = false;
		}

		ImGui::EndTabBar();

		if (psfStackDisplayChanged)
		{
			DelayedJobs::postJob(scene, owner, "PSF Stack Texture Upload", false, 1,
				[&](Scene::Scene& scene, Scene::Object& object)
				{
					PsfPreview::uploadDisplayPsfStackTexture(scene, aberration, &object);
				});
		}

		if (psfDisplayChanged)
		{
			DelayedJobs::postJob(scene, owner, "PSF Texture Upload", false, 1,
				[&](Scene::Scene& scene, Scene::Object& object)
				{
					PsfPreview::uploadDisplayPsfTexture(scene, aberration, &object);
				});
		}

		return { coreChanged, coreChanged || aberrationChanged };
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(Aberration, Aberration, -aberration, healthy, myopia_SphM06, astigmatism_SphM06_CylM03_Ax102, keratoconus)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"aberration", "Aberrations",
			"Which aberration to use by default.",
			"", { "healthy" }, { },
			Config::attribRegexString()
		});
	};
}