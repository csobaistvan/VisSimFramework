#include "PCH.h"
#include "ComplexBlur.h"

namespace ComplexBlur
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(COMPLEX_BLUR);
	DEFINE_OBJECT(COMPLEX_BLUR);
	REGISTER_OBJECT_UPDATE_CALLBACK(COMPLEX_BLUR, AFTER, INPUT);
	REGISTER_OBJECT_RENDER_CALLBACK(COMPLEX_BLUR, "Complex Blur", OpenGL, AFTER, "Effects (LDR) [Begin]", 1, 
		&ComplexBlur::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, 
		&RenderSettings::firstCallObjectCondition, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	namespace Psfs
	{
		////////////////////////////////////////////////////////////////////////////////
		Aberration::WavefrontAberration& getAberration(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<ComplexBlurComponent>().m_aberration;
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::PSFStack& getPsfStack(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<ComplexBlurComponent>().m_aberration.m_psfStack;
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::WavefrontAberrationPresets& getAberrationPresets(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<ComplexBlurComponent>().m_aberrationPresets;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Find the PSF with the smallest defocus
		Aberration::PsfIndex findMinDefocusPsfIndex(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::PsfIndex minPsfIndex;
			float minDefocus = FLT_MAX;
			Aberration::forEachPsfStackIndex(scene, Psfs::getAberration(scene, object),
				[&](auto& scene, auto& aberration, Aberration::PsfIndex const& psfIndex)
				{
					auto const& psfEntry = Aberration::getPsfEntryParameters(scene, aberration, psfIndex);
					const float defocusParam = psfEntry.m_focus.m_defocusParam;
					if (defocusParam < minDefocus)
					{
						minDefocus = defocusParam;
						minPsfIndex = psfIndex;
					}
				});
			return minPsfIndex;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Find the PSF with the largest blur size
		Aberration::PsfIndex findMaxDefocusPsfIndex(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::PsfIndex maxPsfIndex;
			float maxDefocusParam = 0.0f;
			Aberration::forEachPsfStackIndex(scene, Psfs::getAberration(scene, object),
				[&](auto& scene, auto& aberration, Aberration::PsfIndex const& psfIndex)
				{
					auto const& psfEntry = Aberration::getPsfEntryParameters(scene, aberration, psfIndex);
					const float defocusParam = psfEntry.m_focus.m_defocusParam;
					if (defocusParam > maxDefocusParam)
					{
						maxDefocusParam = defocusParam;
						maxPsfIndex = psfIndex;
					}
				});
			return maxPsfIndex;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Find the PSF with the smallest defocus
		Aberration::PsfIndex findMinBlurSizePsfIndex(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::PsfIndex minPsfIndex;
			float minBlurSize = FLT_MAX;
			Aberration::forEachPsfStackIndex(scene, Psfs::getAberration(scene, object),
				[&](auto& scene, auto& aberration, Aberration::PsfIndex const& psfIndex)
				{
					auto const& psfEntry = Aberration::getPsfEntry(scene, aberration, psfIndex);
					const float blurSize = psfEntry.m_blurSizeMuM;
					if (blurSize < minBlurSize)
					{
						minBlurSize = blurSize;
						minPsfIndex = psfIndex;
					}
				});
			return minPsfIndex;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Find the PSF with the largest blur size
		Aberration::PsfIndex findMaxBlurSizePsfIndex(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::PsfIndex maxPsfIndex;
			float maxBlurSize = 0.0f;
			Aberration::forEachPsfStackIndex(scene, Psfs::getAberration(scene, object),
				[&](auto& scene, auto& aberration, Aberration::PsfIndex const& psfIndex)
				{
					auto const& psfEntry = Aberration::getPsfEntry(scene, aberration, psfIndex);
					const float blurSize = psfEntry.m_blurSizeMuM;
					if (blurSize > maxBlurSize)
					{
						maxBlurSize = blurSize;
						maxPsfIndex = psfIndex;
					}
				});
			return maxPsfIndex;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Find the PSF with the closest defocus param
		Aberration::PsfIndex findTargetDefocusPsfIndex(Scene::Scene& scene, Scene::Object* object, const float targetDefocus)
		{
			Aberration::PsfIndex targetPsfIndex;
			float targetDefocusDiff = FLT_MAX;
			Aberration::forEachPsfStackIndex(scene, Psfs::getAberration(scene, object),
				[&](auto& scene, auto& aberration, Aberration::PsfIndex const& psfIndex)
				{
					auto const& psfEntryParameters = Aberration::getPsfEntryParameters(scene, aberration, psfIndex);
					const float defocusDiff = glm::abs(psfEntryParameters.m_focus.m_defocusParam - targetDefocus);
					if (defocusDiff < targetDefocusDiff)
					{
						targetDefocusDiff = defocusDiff;
						targetPsfIndex = psfIndex;
					}
				});
			return targetPsfIndex;
		}

		////////////////////////////////////////////////////////////////////////////////
		void initPsfStack(Scene::Scene& scene, Scene::Object* object)
		{
			const Aberration::PsfStackComputation computationFlags =
				Aberration::PsfStackComputation_RelaxedEyeParameters |
				Aberration::PsfStackComputation_FocusedEyeParameters |
				Aberration::PsfStackComputation_PsfUnits |
				Aberration::PsfStackComputation_PsfBesselTerms |
				Aberration::PsfStackComputation_PsfEnzCoefficients;
			Aberration::computePSFStack(scene, getAberration(scene, object), computationFlags);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computePsfs(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::computePSFStack(scene, getAberration(scene, object), Aberration::PsfStackComputation_Everything);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Dilation
	{
		////////////////////////////////////////////////////////////////////////////////
		int getDilatedTileSize(int searchRadius, int passes)
		{
			return glm::pow(searchRadius, passes);
		}

		////////////////////////////////////////////////////////////////////////////////
		int getDilatedTileSize(Scene::Scene& scene, Scene::Object* object, int passes)
		{
			return getDilatedTileSize(object->component<ComplexBlur::ComplexBlurComponent>().m_dilationSearchRadius, passes);
		}

		////////////////////////////////////////////////////////////////////////////////
		int getDilatedTileSize(Scene::Scene& scene, Scene::Object* object)
		{
			return getDilatedTileSize(
				object->component<ComplexBlur::ComplexBlurComponent>().m_dilationSearchRadius, 
				object->component<ComplexBlur::ComplexBlurComponent>().m_dilationPasses);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::vec2 getDilatedResolution(glm::vec2 renderResolution, int dilatedTileSize)
		{
			return (renderResolution + glm::vec2(dilatedTileSize - 1)) / glm::vec2(dilatedTileSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::vec2 getDilatedResolution(glm::vec2 renderResolution, int searchRadius, int passes)
		{
			return getDilatedResolution(renderResolution, getDilatedTileSize(searchRadius, passes));
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::vec2 getDilatedResolution(Scene::Scene& scene, Scene::Object* object, glm::vec2 renderResolution)
		{
			return getDilatedResolution(renderResolution, getDilatedTileSize(scene, object));
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Kernel
	{
		////////////////////////////////////////////////////////////////////////////////
		ComplexBlurKernelParameters& getKernel(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<ComplexBlurComponent>().m_kernels[object->component<ComplexBlurComponent>().m_numComponents];
		}

		////////////////////////////////////////////////////////////////////////////////
		float getBlurRadiusPixels(Scene::Scene& scene, Scene::Object* object, const glm::ivec2 resolution, const float fovy, const float blurSize)
		{
			return Aberration::blurRadiusPixels(Aberration::blurRadiusAngle(blurSize), resolution, fovy);
		}

		////////////////////////////////////////////////////////////////////////////////
		float getBlurRadiusPixels(Scene::Scene& scene, Scene::Object* object, Scene::Object* renderSettings, Scene::Object* camera, const float blurSize)
		{
			return getBlurRadiusPixels(scene, object, 
				RenderSettings::getResolution(scene, renderSettings), 
				camera->component<Camera::CameraComponent>().m_fovy,
				blurSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		float getBlurRadiusPixels(Scene::Scene& scene, Scene::Object* object, Scene::Object* renderSettings, Scene::Object* camera, const size_t psfId)
		{
			return getBlurRadiusPixels(scene, object,
				RenderSettings::getResolution(scene, renderSettings), 
				camera->component<Camera::CameraComponent>().m_fovy,
				Psfs::getAberration(scene, object).m_psfStack.m_psfs[psfId][0][0][0][0][0].m_blurRadiusMuM);
		}

		////////////////////////////////////////////////////////////////////////////////
		float getBlurRadiusPixels(Scene::Scene& scene, Scene::Object* object, Scene::Object* renderSettings, Scene::Object* camera)
		{
			return getBlurRadiusPixels(scene, object,
				RenderSettings::getResolution(scene, renderSettings),
				camera->component<Camera::CameraComponent>().m_fovy,
				object->component<ComplexBlur::ComplexBlurComponent>().m_blurSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::vec2 getBlurDirection(Scene::Scene& scene, Scene::Object* object, const float offset)
		{
			return glm::vec2(
				glm::cos(offset + object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation),
				glm::sin(offset + object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation));
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::vec2 getBlurDirection(Scene::Scene& scene, Scene::Object* object, const float offset, const glm::ivec2 resolution, const int numTapsRadius)
		{
			return (getBlurDirection(scene, object, offset) / float(numTapsRadius)) / (glm::vec2(resolution));
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::vec2 getBlurDirection(Scene::Scene& scene, Scene::Object* object, Scene::Object* renderSettings, const float offset, const int numTapsRadius)
		{
			return getBlurDirection(scene, object, offset, RenderSettings::getResolution(scene, renderSettings), numTapsRadius);
		}

		////////////////////////////////////////////////////////////////////////////////
		ComplexBlurKernelComponent kernelFunction(ComplexBlurKernelComponent const& component, const float x)
		{
			return
			{
				glm::exp(x * x * -component.m_a) * glm::cos(x * x * component.m_b), // real
				glm::exp(x * x * -component.m_a) * glm::sin(x * x * component.m_b), // imaginary
				component.m_A, // real weight 
				component.m_B, // imaginary weight
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<ComplexBlurKernelComponents> kernelFunction(ComplexBlurKernelParameters const& kernelParameters, const int radius)
		{
			std::vector<ComplexBlurKernelComponents> output(kernelParameters.m_components.size(), ComplexBlurKernelComponents(2 * radius + 1));
			for (int k = 0; k < kernelParameters.m_components.size(); ++k)
			for (int i = -radius, s = 0; i <= radius; ++i, ++s)
			{
				const float x = kernelParameters.m_radius * (i / float(radius));
				output[k][s] = kernelFunction(kernelParameters.m_components[k], x);
			}
			return output;
		}

		////////////////////////////////////////////////////////////////////////////////
		ComplexBlurConvolutionKernel computeKernel(ComplexBlurKernelParameters const& kernelParameters, 
			const int radiusHorizontal, const int radiusVertical)
		{
			// Resulting object
			ComplexBlurConvolutionKernel output;

			// Compute the horizontal and vertical kernel values
			output.m_horizontal = kernelFunction(kernelParameters, radiusHorizontal);
			output.m_vertical = kernelFunction(kernelParameters, radiusVertical);

			// Compute the kernel normalization factor
			float accum = 0.0f;
			for (int k = 0; k < kernelParameters.m_components.size(); ++k)
			for (auto v : output.m_horizontal[k])
			for (auto w : output.m_vertical[k])
			{
				accum += +v.m_A * (v.m_a * w.m_a - v.m_b * w.m_b) + v.m_B * (v.m_a * w.m_b + v.m_b * w.m_a);
			}

			// Normalize the kernel
			float normalizationFactor = 1.0f / glm::sqrt(accum);
			for (int k = 0; k < kernelParameters.m_components.size(); ++k)
			{
				std::transform(output.m_horizontal[k].begin(), output.m_horizontal[k].end(), 
					output.m_horizontal[k].begin(), [=](ComplexBlurKernelComponent c) 
					{ return ComplexBlurKernelComponent{ c.m_a * normalizationFactor, c.m_b * normalizationFactor, 0.0f, 0.0f }; });
				std::transform(output.m_vertical[k].begin(), output.m_vertical[k].end(), 
					output.m_vertical[k].begin(), [=](ComplexBlurKernelComponent c) 
					{ return ComplexBlurKernelComponent{ c.m_a * normalizationFactor, c.m_b * normalizationFactor, 0.0f, 0.0f }; });
			}

			// Compute the offsets for bracketing
			output.m_offsets = std::vector<glm::vec4>(kernelParameters.m_components.size());
			for (size_t k = 0; k < kernelParameters.m_components.size(); ++k)
			{
				glm::vec4 offset{ output.m_horizontal[k][0].m_a, output.m_horizontal[k][0].m_b, output.m_vertical[k][0].m_a, output.m_vertical[k][0].m_b };
				for (auto v : output.m_horizontal[k])
				{
					offset.x = glm::min(offset.x, v.m_a);
					offset.y = glm::min(offset.y, v.m_b);
				}
				for (auto v : output.m_vertical[k])
				{
					offset.z = glm::min(offset.z, v.m_a);
					offset.w = glm::min(offset.w, v.m_b);
				}
				output.m_offsets[k] = offset;
			}

			// Compute the scales for bracketing
			output.m_scales = std::vector<glm::vec4>(kernelParameters.m_components.size());
			for (size_t k = 0; k < kernelParameters.m_components.size(); ++k)
			{
				glm::vec4 scale{ 0.0f, 0.0f, 0.0f, 0.0f };
				for (auto v : output.m_horizontal[k])
				{
					scale.x += v.m_a - output.m_offsets[k].x;
					scale.y += v.m_b - output.m_offsets[k].y;
				}
				for (auto v : output.m_vertical[k])
				{
					scale.z += v.m_a - output.m_offsets[k].z;
					scale.w += v.m_b - output.m_offsets[k].w;
				}
				output.m_scales[k] = scale;
			}

			// Compute the bracketed kernels
			for (size_t k = 0; k < kernelParameters.m_components.size(); ++k)
			{
				for (size_t i = 0; i < output.m_horizontal[k].size(); ++i)
				{
					output.m_horizontal[k][i].m_A = (output.m_horizontal[k][i].m_a - output.m_offsets[k].x) / output.m_scales[k].x;
					output.m_horizontal[k][i].m_B = (output.m_horizontal[k][i].m_b - output.m_offsets[k].y) / output.m_scales[k].y;
				}
				for (size_t i = 0; i < output.m_vertical[k].size(); ++i)
				{
					output.m_vertical[k][i].m_A = (output.m_vertical[k][i].m_a - output.m_offsets[k].z) / output.m_scales[k].z;
					output.m_vertical[k][i].m_B = (output.m_vertical[k][i].m_b - output.m_offsets[k].w) / output.m_scales[k].w;
				}
			}

			// Return the result
			return output;
		}

		////////////////////////////////////////////////////////////////////////////////
		ComplexBlurConvolutionKernel computeKernel(Scene::Scene& scene, Scene::Object* object,
			const int radiusHorizontal, const int radiusVertical)
		{
			return computeKernel(getKernel(scene, object), radiusHorizontal, radiusVertical);
		}

		////////////////////////////////////////////////////////////////////////////////
		ComplexBlurKernelComponent computeGaussianKernelParameters(Scene::Scene& scene, Scene::Object* object, const float sigma)
		{
			return ComplexBlurKernelComponent
			{ 
				/*a*/ 1.0f / (2.0f * sigma * sigma), 
				/*b*/ 1e-5f,
				/*A*/ glm::sqrt(glm::two_pi<float>() * sigma * sigma),
				/*B*/ 0.0f
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadGaussianKernelSettings(Scene::Scene& scene, Scene::Object* object)
		{
			object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents = 1;
			object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelToPsf = false;
			object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelToPsf = false;
			object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation = 0.0f;
			object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRatio = 1.0f;
			object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseContraction = 1.0f;
			object->component<ComplexBlur::ComplexBlurComponent>().m_kernels[1].m_radius = 2.5f;
			object->component<ComplexBlur::ComplexBlurComponent>().m_kernels[1].m_components = { computeGaussianKernelParameters(scene, object, 1.0f) };
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadGarciaKernelSettings(Scene::Scene& scene, Scene::Object* object)
		{
			object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents = 1;
			object->component<ComplexBlur::ComplexBlurComponent>().m_kernels =
			{
				// 0 components
				ComplexBlurKernelParameters
				{
					1.25f,
					ComplexBlurKernelComponents
					{},
				},

				// 1 component
				ComplexBlurKernelParameters
				{
					1.25f,
					ComplexBlurKernelComponents
					{
						ComplexBlurKernelComponent{ 0.862325f, 1.624835f, 0.767583f, 1.862321f },
					},
				},

				// 2 components
				ComplexBlurKernelParameters
				{
					1.25f,
					ComplexBlurKernelComponents
					{
						ComplexBlurKernelComponent{ 0.886528f, 5.268909f, 0.411259f, -0.548794f },
						ComplexBlurKernelComponent{ 1.960518f, 1.558213f, 0.513282f, 4.561110f },
					},
				},

				// 3 components
				ComplexBlurKernelParameters
				{
					1.25f,
					ComplexBlurKernelComponents
					{
						ComplexBlurKernelComponent{ 2.176490f, 5.043495f, 1.621035f, -2.105439f },
						ComplexBlurKernelComponent{ 1.019306f, 9.027613f, -0.280860f, -0.162882f },
						ComplexBlurKernelComponent{ 2.815110f, 1.597273f, -0.366471f, 10.300301f },
					},
				},
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<float> generate2DKernel(ComplexBlurKernelParameters const& kernelParameters, const int radiusHorizontal, const int radiusVertical)
		{
			// Compute the hight resolution kernel
			const ComplexBlurConvolutionKernel kernelHiRes = computeKernel(kernelParameters, radiusHorizontal, radiusVertical);

			// Generate the weights
			std::vector<float> weights(kernelHiRes.m_horizontal[0].size() * kernelHiRes.m_vertical[0].size(), 0.0f);
			for (int row = 0; row < kernelHiRes.m_vertical[0].size(); ++row)
			{
				for (int col = 0; col < kernelHiRes.m_horizontal[0].size(); ++col)
				{
					float weight = 0.0f;
					for (int c = 0; c < kernelParameters.m_components.size(); ++c)
					{
						auto v = kernelHiRes.m_vertical[c][row], w = kernelHiRes.m_horizontal[c][col];
						weight += 
							kernelParameters.m_components[c].m_A * (v.m_a * w.m_a - v.m_b * w.m_b) + 
							kernelParameters.m_components[c].m_B * (v.m_a * w.m_b + v.m_b * w.m_a);
					}

					int pixelId = row * kernelHiRes.m_horizontal[0].size() + col;
					weights[pixelId] = weight;
				}
			}
			return weights;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<float> generate2DKernel(Scene::Scene& scene, Scene::Object* object, const int radiusHorizontal, const int radiusVertical)
		{
			return generate2DKernel(getKernel(scene, object), radiusHorizontal, radiusVertical);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getDebugImageFname(Scene::Scene& scene, Scene::Object* object, std::string const& exportPrefix, std::string const& fname)
		{
			return (exportPrefix + "_" + fname + "_" + DateTime::getDateStringUtf8(DateTime::dateFormatFilename()) + ".png");
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getDebugImageFullpath(Scene::Scene& scene, Scene::Object* object, std::string const& exportPrefix, std::string const& fname)
		{
			return (EnginePaths::generatedFilesFolder() / "ComplexBlur" / getDebugImageFname(scene, object, exportPrefix, fname)).string();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename Img>
		void saveDebugImage(Scene::Scene& scene, Scene::Object* object, std::string const& exportPrefix, std::string const& fname, Img const& image)
		{
			Asset::saveImage(scene, getDebugImageFullpath(scene, object, exportPrefix, fname), image, true);
		}

		////////////////////////////////////////////////////////////////////////////////
		namespace FitEllipse
		{
			////////////////////////////////////////////////////////////////////////////////
			using Ellipse = std::pair<glm::vec2, float>;

			////////////////////////////////////////////////////////////////////////////////
			Ellipse fitEllipseToPsf(Scene::Scene& scene, Scene::Object* object,
				Aberration::Psf const& targetPsf,
				const float threshold, const bool exportImages, std::string const& exportPrefix)
			{
				Debug::log_debug() << "Fitting ellipse to PSF" << Debug::end;
				Debug::log_debug() << " > PSF size: " << targetPsf.rows() << "x" << targetPsf.cols() << Debug::end;

				// Normalize the PSF
				Aberration::Psf psf = targetPsf / targetPsf.maxCoeff();

				// Convert the PSF to binary
				cv::Mat cvPsf;
				cv::eigen2cv(psf, cvPsf);
				cv::Mat psfBinaryF;
				cv::threshold(cvPsf, psfBinaryF, threshold, 1.0f, cv::THRESH_BINARY);
				cv::Mat psfBinary;
				psfBinaryF.convertTo(psfBinary, CV_8UC1);

				// Save the target PSF
				if (exportImages)
				{
					cv::Mat psfSave;
					psfBinary.convertTo(psfSave, CV_32FC1);
					saveDebugImage(scene, object, exportPrefix, "ell_thr", psfSave);
				}

				// Fit a rect around the binary image
				std::vector<cv::Point> pts;
				for (int j = 0; j < psfBinary.rows; ++j)
				for (int i = 0; i < psfBinary.cols; ++i)
					if (psfBinary.at<unsigned char>(j, i))
						pts.push_back(cv::Point(i, j));

				// Find the convex hull of the points
				std::vector<cv::Point> convexHull;
				cv::convexHull(pts, convexHull);

				if (exportImages)
				{
					std::vector<std::vector<cv::Point>> convexHulls(1, convexHull);
					cv::Mat psfSave;
					cvPsf.convertTo(psfSave, CV_32FC1);
					cv::drawContours(psfSave, convexHulls, -1, 0.5f, 2, cv::LINE_8);
					saveDebugImage(scene, object, exportPrefix, "ell_chull", psfSave);
				}

				if (exportImages)
				{
					std::vector<std::vector<cv::Point>> convexHulls(1, convexHull);
					cv::Mat psfSave;
					psfBinary.convertTo(psfSave, CV_32FC1);
					cv::drawContours(psfSave, convexHulls, -1, 0.5f, 2, cv::LINE_8);
					saveDebugImage(scene, object, exportPrefix, "ell_chull_bin", psfSave);
				}

				// Fit an ellipse around the convex hull
				cv::RotatedRect e = cv::fitEllipse(convexHull);

				if (exportImages)
				{
					cv::Mat psfSave;
					cvPsf.convertTo(psfSave, CV_32FC1);
					cv::ellipse(psfSave, e, 0.5f, 2, cv::LINE_8);
					saveDebugImage(scene, object, exportPrefix, "ell_fit", psfSave);
				}

				if (exportImages)
				{
					cv::Mat psfSave;
					psfBinary.convertTo(psfSave, CV_32FC1);
					cv::ellipse(psfSave, e, 0.5f, 2, cv::LINE_8);
					saveDebugImage(scene, object, exportPrefix, "ell_fit_bin", psfSave);
				}

				// Construct the result
				Ellipse result{ glm::vec2(e.size.width, e.size.height), e.angle };

				Debug::log_debug() << " > Fit ellipse" << ": " <<
					"radius[" << result.first << "]" << ", "
					"angle[" << glm::degrees(result.second) << "]" <<
					Debug::end;

				return result;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		namespace KernelAlign
		{
			////////////////////////////////////////////////////////////////////////////////
			Aberration::Psf constructTargetPsf(Scene::Scene& scene, Scene::Object* object,
				ComplexBlurComponent::AlignKernelSettings const& alignSettings, 
				Aberration::PsfStackElements::PsfEntry const& targetPsf)
			{
				// Normalize the PSF
				Aberration::Psf psf = targetPsf.m_psf / targetPsf.m_psf.maxCoeff();

				// Save the target PSF
				if (alignSettings.m_exportPsf)
				{
					saveDebugImage(scene, object, "align_psf", "full", psf);
				}

				// Return the final result
				return psf;
			}

			////////////////////////////////////////////////////////////////////////////////
			void alignKernel(Scene::Scene& scene, Scene::Object* object, ComplexBlurComponent::AlignKernelSettings const& alignSettings)
			{
				// Find the necessary PSFs
				const Aberration::PsfIndex maxPsfIndex = Psfs::findMaxBlurSizePsfIndex(scene, object);
				Aberration::PsfStackElements::PsfEntry const& maxPsf = getPsfEntry(scene, Psfs::getAberration(scene, object), maxPsfIndex);
				const Aberration::PsfIndex targetPsfIndex = Psfs::findTargetDefocusPsfIndex(scene, object, alignSettings.m_targetDefocus);
				Aberration::PsfStackElements::PsfEntry const& targetPsf = getPsfEntry(scene, Psfs::getAberration(scene, object), targetPsfIndex);

				// Construct the target PSF
				Aberration::Psf psf = constructTargetPsf(scene, object, alignSettings, targetPsf);

				// Fit an ellipse over the PSF
				FitEllipse::Ellipse ellipse = FitEllipse::fitEllipseToPsf(scene, object, psf, alignSettings.m_ellipseThreshold,
					alignSettings.m_exportPsf, "align");

				// Set the blur parameters
				object->component<ComplexBlur::ComplexBlurComponent>().m_blurSize = maxPsf.m_blurRadiusMuM;
				object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation = glm::radians(ellipse.second) - glm::half_pi<float>();
				object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRatio = (ellipse.first.x / float(targetPsf.m_kernelSizePx));
				object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseContraction = ellipse.first.x / ellipse.first.y;
			}

			////////////////////////////////////////////////////////////////////////////////
			void alignKernel(Scene::Scene& scene, Scene::Object* object)
			{
				alignKernel(scene, object, object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		namespace KernelFit
		{
			////////////////////////////////////////////////////////////////////////////////
			struct CostFunctor
			{
				////////////////////////////////////////////////////////////////////////////////
				CostFunctor(const Aberration::Psf target, const size_t numComponents, const size_t radiusHorizontal, const size_t radiusVertical) :
					m_target(target),
					m_numComponents(numComponents),
					m_radiusHorizontal(radiusHorizontal),
					m_radiusVertical(radiusVertical)
				{}

				////////////////////////////////////////////////////////////////////////////////
				static ComplexBlurKernelParameters toKernelParameters(const size_t numComponents, const double* x0)
				{
					ComplexBlurKernelParameters result;
					result.m_radius = float(x0[0]);
					result.m_components.resize(numComponents);
					for (size_t i = 0; i < numComponents; ++i)
					{
						result.m_components[i].m_a = float(x0[1 + i * 4 + 0]);
						result.m_components[i].m_b = float(x0[1 + i * 4 + 1]);
						result.m_components[i].m_A = float(x0[1 + i * 4 + 2]);
						result.m_components[i].m_B = float(x0[1 + i * 4 + 3]);
					}
					return result;
				}

				////////////////////////////////////////////////////////////////////////////////
				bool operator()(const double* parameters, double* residuals) const
				{
					// Extract the kernel parameters
					ComplexBlurKernelParameters kernelParameters = toKernelParameters(m_numComponents, parameters);

					// Generate the kernel image
					std::vector<float> kernelImage = generate2DKernel(kernelParameters, m_radiusHorizontal, m_radiusVertical);

					// Turn to the corresponding Eigen object
					using Kernel = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
					Aberration::Psf kernel = Eigen::Map<Kernel>(kernelImage.data(), m_radiusVertical * 2 + 1, m_radiusHorizontal * 2 + 1);
					kernel /= kernel.sum();

					// Calculate the mean difference
					residuals[0] = double((kernel - m_target).cwiseAbs2().mean()) * 1e6f;

					/*
					Debug::log_info() << 
						std::vector<double>(parameters, parameters + m_numComponents * 4 + 1) << ": " <<
						//m_target << ", " <<
						//kernel << ", " <<
						residuals[0] << 
						Debug::end;
					*/

					// Whether the loss was tractable or not
					bool tractable = !isinf(residuals[0]) && !isnan(residuals[0]);
					//for (int c = 0; c < m_numComponents; ++c)
					//	tractable &= (kernelParameters.m_components[c].m_a != 0.0f && kernelParameters.m_components[c].m_b);
					return tractable;
				}

				// The target PSF to estimate
				Aberration::Psf m_target;

				// Kernel settings
				size_t m_numComponents;
				size_t m_radiusHorizontal;
				size_t m_radiusVertical;
			};

			////////////////////////////////////////////////////////////////////////////////
			ceres::CostFunction* makeCostFunction(ComplexBlurComponent::FitKernelSettings const& fitSettings, CostFunctor* cost, const size_t numComponents)
			{
				// Create differentation options
				ceres::NumericDiffOptions diffOptions;
				diffOptions.relative_step_size = fitSettings.m_diffStepSize;

				switch (numComponents)
				{
				case 1: return new ceres::NumericDiffCostFunction<CostFunctor, ceres::CENTRAL, 1, 5>(cost, ceres::TAKE_OWNERSHIP, 1, diffOptions);
				case 2: return new ceres::NumericDiffCostFunction<CostFunctor, ceres::CENTRAL, 1, 9>(cost, ceres::TAKE_OWNERSHIP, 1, diffOptions);
				case 3: return new ceres::NumericDiffCostFunction<CostFunctor, ceres::CENTRAL, 1, 13>(cost, ceres::TAKE_OWNERSHIP, 1, diffOptions);
				}

				Debug::log_error() << "Unsupported number of kernel components: " << numComponents << Debug::end;

				return nullptr;
			}

			////////////////////////////////////////////////////////////////////////////////
			Aberration::Psf constructTargetPsf(Scene::Scene& scene, Scene::Object* object,
				ComplexBlurComponent::FitKernelSettings const& fitSettings,
				Aberration::PsfStackElements::PsfEntry const& targetPsf, 
				const size_t radiusHorizontal, const size_t radiusVertical)
			{
				// Project the PSF
				Aberration::Psf psf = targetPsf.m_psf;
				if (fitSettings.m_projectPsf)
				{
					Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
					Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

					const glm::vec2 renderResolution = RenderSettings::getResolutionById(scene, object->component<ComplexBlur::ComplexBlurComponent>().m_renderResolutionId);
					const float fovy = camera->component<Camera::CameraComponent>().m_fovy;

					psf = Aberration::getProjectedPsf(scene, Psfs::getAberration(scene, object), targetPsf, glm::ivec2(renderResolution), fovy);
				}

				// Fit the ellipse to the PSF
				FitEllipse::Ellipse ellipse = FitEllipse::fitEllipseToPsf(scene, object, psf, fitSettings.m_ellipseThreshold, fitSettings.m_exportPsf, "fit_psf");

				// Normalize the PSF
				psf = psf / psf.maxCoeff();

				// Convert the PSF to CV format
				cv::Mat psfCV;
				cv::eigen2cv(psf, psfCV);

				// Unwarp the ellipse
				cv::Mat psfUnwarped;
				const int ellSize = int(std::ceil(std::max(ellipse.first.x, ellipse.first.y)));
				const cv::Point2f pc(psfCV.cols / 2.0f, psfCV.rows / 2.0f);
				const glm::mat4 t0 = glm::translate(glm::vec3(-pc.x, -pc.y, 0.0f));
				const glm::mat4 r = glm::rotate(-glm::radians(ellipse.second), glm::vec3(0.0f, 0.0f, 1.0f));
				const glm::mat4 s = glm::scale(glm::vec3(1.0f, ellipse.first.x / ellipse.first.y, 1.0f));
				const glm::mat4 t1 = glm::translate(glm::vec3(pc.x, pc.y, 0.0f));
				const glm::mat4 t = t1 * s * r * t0;
				const cv::Mat tcv = cv::Mat(cv::Matx23d(t[0][0], t[1][0], t[3][0], t[0][1], t[1][1], t[3][1]));
				cv::warpAffine(psfCV, psfUnwarped, tcv, psfCV.size());

				// Crop the image
				const int numRowsFull = psfUnwarped.rows;
				const int numRowsEll = int(std::ceil(ellipse.first.x));
				const int startRows = std::max((numRowsFull / 2) - (numRowsEll / 2), 0);
				const int endRows = std::min((numRowsFull / 2) + (numRowsEll / 2) + 1, psfUnwarped.rows);
				cv::Mat psfCropped = psfUnwarped(cv::Range(startRows, endRows), cv::Range(startRows, endRows));

				// Resize the PSF to the final size
				cv::Mat psfResized;
				cv::resize(psfCropped, psfResized, cv::Size(radiusHorizontal * 2 + 1, radiusVertical * 2 + 1), 0, 0, cv::INTER_AREA);
				//cv::resize(psfCropped, psfResized, cv::Size(radiusHorizontal * 2 + 1, radiusVertical * 2 + 1), 0, 0, cv::INTER_LANCZOS4);

				// Save the target PSF
				if (fitSettings.m_exportPsf)
				{
					saveDebugImage(scene, object, "fit_psf", "unwarped", psfUnwarped);
					saveDebugImage(scene, object, "fit_psf", "full", psfCropped);
					saveDebugImage(scene, object, "fit_psf", "resized", psfResized);
				}

				// Convert back to Eigen and return
				Aberration::Psf result;
				cv::cv2eigen(psfResized, result);

				// Normalize the result
				result = result / psf.sum();

				return result;
			}

			////////////////////////////////////////////////////////////////////////////////
			Aberration::Psf constructTargetPsf(Scene::Scene& scene, Scene::Object* object,
				ComplexBlurComponent::FitKernelSettings const& fitSettings,
				Aberration::PsfStackElements::PsfEntry const& targetPsf)
			{
				// Common kernel settings
				const size_t numComponents = object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents;
				const int kernelTaps = object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius * fitSettings.m_fitScale;

				// Construct the target PSF
				return constructTargetPsf(scene, object, fitSettings, targetPsf, kernelTaps, kernelTaps);
			}

			////////////////////////////////////////////////////////////////////////////////
			std::vector<double> createFitResult(Scene::Scene& scene, Scene::Object* object, 
				ComplexBlurComponent::FitKernelSettings const& fitSettings)
			{
				// Common kernel settings
				const size_t numComponents = object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents;

				// Create the resulting structure
				std::vector<double> fitResult(numComponents * 4 + 1, 0.0);

				// Init the radius and the components
				fitResult[0] = 1.5f;
				for (size_t i = 0; i < numComponents; ++i)
				for (size_t c = 0; c < 4; ++c)
					fitResult[1 + i * 4 + c] = fitSettings.m_initialComponents[c];

				// Return the result
				return fitResult;
			}

			////////////////////////////////////////////////////////////////////////////////
			ComplexBlurKernelParameters fitKernel(Scene::Scene& scene, Scene::Object* object,
				Aberration::PsfStackElements::PsfEntry const& psf, 
				ComplexBlurComponent::FitKernelSettings const& fitSettings)
			{
				// Common kernel settings
				const size_t numComponents = object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents;
				const int kernelTaps = object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius * fitSettings.m_fitScale;

				// Result of the fit
				std::vector<double> fitResult = createFitResult(scene, object, fitSettings);

				// Construct the target PSF
				Aberration::Psf targetPsf = constructTargetPsf(scene, object, fitSettings, psf);

				// Create the problem object
				ceres::Problem problem;
				CostFunctor* cost = new CostFunctor(targetPsf, numComponents, kernelTaps, kernelTaps);
				ceres::CostFunction* costFn = makeCostFunction(fitSettings, cost, numComponents);
				problem.AddResidualBlock(costFn, nullptr, fitResult.data());
				problem.SetParameterLowerBound(fitResult.data(), 0, fitSettings.m_radiusLimits.x);
				problem.SetParameterUpperBound(fitResult.data(), 0, fitSettings.m_radiusLimits.y);

				for (size_t i = 0; i < numComponents; ++i)
				{
					problem.SetParameterLowerBound(fitResult.data(), i * 4 + 0, fitSettings.m_aLimits.x); // a
					problem.SetParameterUpperBound(fitResult.data(), i * 4 + 0, fitSettings.m_aLimits.y);
					problem.SetParameterLowerBound(fitResult.data(), i * 4 + 1, fitSettings.m_bLimits.x); // b
					problem.SetParameterUpperBound(fitResult.data(), i * 4 + 1, fitSettings.m_bLimits.y);
					problem.SetParameterLowerBound(fitResult.data(), i * 4 + 2, fitSettings.m_ALimits.x); // A
					problem.SetParameterUpperBound(fitResult.data(), i * 4 + 2, fitSettings.m_ALimits.y);
					problem.SetParameterLowerBound(fitResult.data(), i * 4 + 3, fitSettings.m_BLimits.x); // B
					problem.SetParameterUpperBound(fitResult.data(), i * 4 + 3, fitSettings.m_BLimits.y);
				}

				// Create the solver options
				ceres::Solver::Options options;

				// Common options
				options.num_threads = Threading::numThreads();
				options.logging_type = fitSettings.m_logProgress ? ceres::PER_MINIMIZER_ITERATION : ceres::SILENT;

				// Line-search options
				//options.minimizer_type = ceres::LINE_SEARCH;
				options.line_search_type = ceres::WOLFE;
				options.line_search_direction_type = ceres::BFGS;

				// Trust region options
				options.minimizer_type = ceres::TRUST_REGION;
				//options.trust_region_strategy_type = ceres::LEVENBERG_MARQUARDT;
				options.trust_region_strategy_type = ceres::DOGLEG;
				options.dogleg_type = ceres::TRADITIONAL_DOGLEG;

				// Other optimizer options
				options.linear_solver_type = ceres::DENSE_QR;
				//options.linear_solver_type = ceres::DENSE_SCHUR;
				//options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
				options.preconditioner_type = ceres::SCHUR_JACOBI;
				options.visibility_clustering_type = ceres::SINGLE_LINKAGE;
				options.use_approximate_eigenvalue_bfgs_scaling = true;
				options.use_nonmonotonic_steps = true;
				options.initial_trust_region_radius = 1e-1f;

				// Termination options 
				options.max_num_iterations = fitSettings.m_maxIterations;
				options.max_solver_time_in_seconds = fitSettings.maxMinutes * 60.0f;
				options.function_tolerance = 1e-8f;

				// Solve the problem
				ceres::Solver::Summary summary;
				ceres::Solve(options, &problem, &summary);

				// Extract and return the results
				return CostFunctor::toKernelParameters(numComponents, fitResult.data());
			}

			////////////////////////////////////////////////////////////////////////////////
			ComplexBlurKernelParameters fitKernel(Scene::Scene& scene, Scene::Object* object)
			{
				// Find the necessary PSF
				const Aberration::PsfIndex targetPsfIndex = Psfs::findTargetDefocusPsfIndex(scene, object,
					object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_targetDefocus);
				Aberration::PsfStackElements::PsfEntry const& targetPsf = getPsfEntry(scene, Psfs::getAberration(scene, object), targetPsfIndex);

				// Fit the kernel around it
				return fitKernel(scene, object, targetPsf, object->component<ComplexBlurComponent>().m_fitKernelSettings);
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		void updateKernelSettings(Scene::Scene& scene, Scene::Object* object)
		{
			// Align the kernel to the PSF
			if (object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelToPsf)
			{
				DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, 1, DateTime::Seconds, "Kernel Alignment");

				KernelAlign::alignKernel(scene, object);
			}

			// Fit the kernel to the PSF
			if (object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelToPsf)
			{
				DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, 1, DateTime::Seconds, "Kernel Fit");

				getKernel(scene, object) = KernelFit::fitKernel(scene, object);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getFullTextureName(Scene::Scene& scene, Scene::Object* object, std::string const& textureName)
		{
			return object->m_name + "_" + textureName;
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadPreviewTexture(Scene::Scene& scene, Scene::Object* object, std::string const& textureName, 
			const size_t columns, const size_t rows, const float* pixelData)
		{
			Scene::createTexture(scene, getFullTextureName(scene, object, textureName), GL_TEXTURE_2D,
				columns, rows, 1,
				GL_RGB16F, GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
				GL_FLOAT, pixelData,
				GL_TEXTURE0);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadPreviewTexture(Scene::Scene& scene, Scene::Object* object, std::string const& textureName, Aberration::Psf psf, const bool normalize = true)
		{
			// Normalize the texture
			if (normalize)
			{
				psf = psf / psf.sum();
				psf = psf / psf.maxCoeff();
			}

			// Construct the rgb image
			using MatrixXf3rm = Eigen::Matrix<glm::vec3, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
			MatrixXf3rm const& psfRgb = Eigen::Map<const Aberration::Psf>(psf.data(), psf.rows(), psf.cols()).cwiseAbs().cast<glm::vec3>();

			// Upload the texture data
			uploadPreviewTexture(scene, object, textureName, psfRgb.cols(), psfRgb.rows(), (float*)psfRgb.data());
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::Psf getKernelDisplayImage(Scene::Scene& scene, Scene::Object* object, int radiusHorizontal, int radiusVertical)
		{
			// Column and row sizes
			int maxRadius = glm::max(radiusHorizontal, radiusVertical);
			int numCols = maxRadius * 2 + 1;
			int numRows = numCols;

			// Generate kernel weights
			std::vector<float> kernelWeights = generate2DKernel(scene, object, maxRadius, maxRadius);

			// Generate the image data
			std::vector<float> pixelData(numCols * numRows, 0.0f);
			float max = *std::max_element(kernelWeights.begin(), kernelWeights.end());
			std::transform(kernelWeights.begin(), kernelWeights.end(), pixelData.begin(), [&](float weight) { return weight / max; });

			// Compute the aspect ratios of our kernel
			float blurAspect = 1.0f / object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseContraction;
			if (isinf(blurAspect) || isnan(blurAspect)) blurAspect = 0.1f;

			// Convert the pixel data to an opencv matrix
			cv::Mat pixelDataCV(numRows, numCols, CV_32F, pixelData.data());

			// Rescale the kernel image
			cv::Mat pixelDataCVZoomed;
			if (blurAspect > 1.0f)
			{
				cv::Mat pixelDataCVScaled;
				cv::resize(pixelDataCV, pixelDataCVScaled, cv::Size(pixelDataCV.cols, int(pixelDataCV.rows / blurAspect) / 2 * 2 + 1), 0, 0, cv::INTER_LANCZOS4);
				pixelDataCVZoomed = cv::Mat::zeros(pixelDataCVScaled.cols, pixelDataCVScaled.cols, CV_32F);
				pixelDataCVScaled.copyTo(pixelDataCVZoomed(cv::Rect(0, (pixelDataCVScaled.cols - pixelDataCVScaled.rows) / 2, pixelDataCVScaled.cols, pixelDataCVScaled.rows)));
			}
			else
			{
				cv::Mat pixelDataCVScaled;
				cv::resize(pixelDataCV, pixelDataCVScaled, cv::Size(int(pixelDataCV.cols * blurAspect) / 2 * 2 + 1, pixelDataCV.rows), 0, 0, cv::INTER_LANCZOS4);
				pixelDataCVZoomed = cv::Mat::zeros(pixelDataCVScaled.rows, pixelDataCVScaled.rows, CV_32F);
				pixelDataCVScaled.copyTo(pixelDataCVZoomed(cv::Rect((pixelDataCVScaled.rows - pixelDataCVScaled.cols) / 2, 0, pixelDataCVScaled.cols, pixelDataCVScaled.rows)));
			}

			// Rotate the kernel image
			cv::Mat rotationMatrix = cv::getRotationMatrix2D(
				cv::Point2f(pixelDataCVZoomed.cols / 2, pixelDataCVZoomed.rows / 2),
				glm::degrees(object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation), 1.0f);
			cv::Mat pixelDataCVRotated;
			cv::warpAffine(pixelDataCVZoomed, pixelDataCVRotated, rotationMatrix, cv::Size(pixelDataCVZoomed.cols, pixelDataCVZoomed.rows));

			// Flip the image
			cv::Mat pixelDataCVFlipped;
			cv::flip(pixelDataCVRotated, pixelDataCVFlipped, 0);

			// Convert the CV image to an Eigen PSF and return the result
			Aberration::Psf kernel; 
			cv::cv2eigen(pixelDataCVFlipped, kernel);
			return kernel;
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadKernelTexture(Scene::Scene& scene, Scene::Object* object, std::string const& textureName, int radiusHorizontal, int radiusVertical)
		{
			uploadPreviewTexture(scene, object, textureName, 
				getKernelDisplayImage(scene, object, radiusHorizontal, radiusVertical));
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadKernelTextures(Scene::Scene& scene, Scene::Object* object)
		{
			uploadKernelTexture(scene, object, "Kernel",
				object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius,
				object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadKernelDifferenceTexture(Scene::Scene& scene, Scene::Object* object, std::string const& textureName, int radiusHorizontal, int radiusVertical)
		{
			// Find the necessary PSF
			ComplexBlurComponent::FitKernelSettings const& fitSettings = object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings;
			const Aberration::PsfIndex targetPsfIndex = Psfs::findTargetDefocusPsfIndex(scene, object, fitSettings.m_targetDefocus);
			auto const& targetPsf = getPsfEntry(scene, Psfs::getAberration(scene, object), targetPsfIndex);

			// Construct the target PSF
			Aberration::Psf psf = targetPsf.m_psf;
				Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
			Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

			const glm::vec2 renderResolution = RenderSettings::getResolutionById(scene, object->component<ComplexBlur::ComplexBlurComponent>().m_renderResolutionId);
			const float fovy = camera->component<Camera::CameraComponent>().m_fovy;

			psf = Aberration::getProjectedPsf(scene, Psfs::getAberration(scene, object), targetPsf, glm::ivec2(renderResolution), fovy);
			psf = psf / psf.sum();
			psf = psf / psf.maxCoeff();

			// Generate the target kernel
			Aberration::Psf kernel = getKernelDisplayImage(scene, object, radiusHorizontal, radiusVertical);
			kernel = Eigen::resize(kernel, Eigen::Vector2i(psf.cols(), psf.rows()), cv::INTER_LANCZOS4);

			// Construct the difference
			Aberration::Psf difference = (kernel - psf).cwiseAbs();

			// Upload the PSF data
			uploadPreviewTexture(scene, object, textureName, difference, false);

			// Compute the normalized difference
			Aberration::Psf kernelNorm = kernel / kernel.sum();
			Aberration::Psf psfNorm = psf / psf.sum();
			Aberration::Psf differenceNorm = (kernelNorm - psfNorm).cwiseAbs2();
			object->component<ComplexBlur::ComplexBlurComponent>().m_fitMav = kernelNorm.cwiseAbs().mean();
			object->component<ComplexBlur::ComplexBlurComponent>().m_fitMse = differenceNorm.mean();
			object->component<ComplexBlur::ComplexBlurComponent>().m_fitRmse = glm::sqrt(object->component<ComplexBlur::ComplexBlurComponent>().m_fitMse);
			object->component<ComplexBlur::ComplexBlurComponent>().m_fitPsnr = 10.0f * std::log10f(1.0f / object->component<ComplexBlur::ComplexBlurComponent>().m_fitMse);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadKernelDifferenceTextures(Scene::Scene& scene, Scene::Object* object)
		{
			uploadKernelDifferenceTexture(scene, object, "KernelDiff",
				object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius,
				object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadPsfTexture(Scene::Scene& scene, Scene::Object* object, std::string const& textureName, Aberration::PsfIndex const& psfIndex, const bool normalize = true)
		{
			uploadPreviewTexture(scene, object, textureName, 
				Aberration::getPsfEntry(scene, Psfs::getAberration(scene, object), psfIndex).m_psf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadKernelAlignmentTextures(Scene::Scene& scene, Scene::Object* object)
		{
			// Find the necessary PSFs
			ComplexBlurComponent::AlignKernelSettings const& alignSettings = object->component<ComplexBlurComponent>().m_alignKernelSettings;
			const Aberration::PsfIndex targetPsfIndex = Psfs::findTargetDefocusPsfIndex(scene, object, alignSettings.m_targetDefocus);
			auto const& targetPsf = getPsfEntry(scene, Psfs::getAberration(scene, object), targetPsfIndex);

			// Construct the target PSF
			Aberration::Psf psf = KernelAlign::constructTargetPsf(scene, object, alignSettings, targetPsf);

			// Upload the results
			uploadPreviewTexture(scene, object, "AlignPsfBase", targetPsf.m_psf);
			uploadPreviewTexture(scene, object, "AlignPsfTransformed", psf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadKernelFitTextures(Scene::Scene& scene, Scene::Object* object)
		{
			// Find the necessary PSF
			ComplexBlurComponent::FitKernelSettings const& fitSettings = object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings;
			const Aberration::PsfIndex targetPsfIndex = Psfs::findTargetDefocusPsfIndex(scene, object, fitSettings.m_targetDefocus);
			auto const& targetPsf = getPsfEntry(scene, Psfs::getAberration(scene, object), targetPsfIndex);

			// Construct the target PSF
			Aberration::Psf psf = KernelFit::constructTargetPsf(scene, object, fitSettings, targetPsf);

			// Upload the results
			uploadPreviewTexture(scene, object, "FitPsfBase", targetPsf.m_psf);
			uploadPreviewTexture(scene, object, "FitPsfTransformed", psf);
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeKernel(Scene::Scene& scene, Scene::Object* object)
		{
			// Delegate the work to the other function
			object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel = computeKernel(scene, object,
				object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius,
				object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius);

			// Generate the preview textures
			// - Main kernel
			uploadKernelTextures(scene, object);
			// - PSF vs. kernel difference
			uploadKernelDifferenceTextures(scene, object);
			// - Kernel alignment target
			uploadKernelAlignmentTextures(scene, object);
			// - Kernel fit target
			uploadKernelFitTextures(scene, object);
		}

		////////////////////////////////////////////////////////////////////////////////
		void exportKernelTexture(Scene::Scene& scene, Scene::Object* object, std::filesystem::path const& outFolder, std::string const& textureName)
		{
			auto fullFilePath = outFolder / (EnginePaths::stringToFileName(textureName) + ".png");
			Asset::saveTexture(scene, getFullTextureName(scene, object, textureName), fullFilePath.string());
		}

		////////////////////////////////////////////////////////////////////////////////
		void exportKernels(Scene::Scene& scene, Scene::Object* object)
		{
			auto const& outFolderName = EnginePaths::generateUniqueFilename(Psfs::getAberration(scene, object).m_name + "_");
			auto const& outFolder = EnginePaths::generatedFilesFolder() / "ComplexBlur" / "Kernels" / outFolderName;

			exportKernelTexture(scene, object, outFolder, "Kernel");
			exportKernelTexture(scene, object, outFolder, "KernelDiff");
			exportKernelTexture(scene, object, outFolder, "AlignPsfBase");
			exportKernelTexture(scene, object, outFolder, "AlignPsfTransformed");
			exportKernelTexture(scene, object, outFolder, "FitPsfBase");
			exportKernelTexture(scene, object, outFolder, "FitPsfTransformed");
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Shaders
	{
		////////////////////////////////////////////////////////////////////////////////
		std::vector<std::string> shaderNames(Scene::Scene& scene, Scene::Object* object)
		{
			return std::vector<std::string>
			{
				"complex_blur_downscale"s,
				"complex_blur_coc_dilate"s,
				"complex_blur_horizontal_near"s,
				"complex_blur_horizontal_far"s,
				"complex_blur_vertical"s,
				"complex_blur_composite"s,
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		struct ShaderParameters
		{
			// Constructor that sets the default values
			ShaderParameters(Scene::Scene& scene, Scene::Object* object) :
				m_components(object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents),
				m_radiusHorizontal(object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius),
				m_radiusVertical(object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius),
				m_numTapsHorizontal(object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius * 2 + 1),
				m_numTapsVertical(object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius * 2 + 1),
				m_maxComponents(MAX_COMPONENTS),
				m_maxKernelRadius(MAX_KERNEL_RADIUS),
				m_maxKernelSize(MAX_KERNEL_RADIUS * 2 + 1),
				m_maxObjectDistances(MAX_OBJECT_DISTANCES)
			{}

			// The various parameters
			int m_components;
			int m_radiusHorizontal;
			int m_radiusVertical;
			int m_numTapsHorizontal;
			int m_numTapsVertical;
			int m_maxComponents;
			int m_maxKernelRadius;
			int m_maxKernelSize;
			int m_maxObjectDistances;

			////////////////////////////////////////////////////////////////////////////////
			template<typename Fn>
			void forEachParameter(Fn const& callback) const
			{
				callback("Components", "KERNEL_COMPONENTS", m_components);
				callback("Radius (Horizontal)", "KERNEL_RADIUS_HORIZONTAL", m_radiusHorizontal);
				callback("Radius (Vertical)", "KERNEL_RADIUS_VERTICAL", m_radiusVertical);
				callback("Size (Horizontal)", "KERNEL_SIZE_HORIZONTAL", m_numTapsHorizontal);
				callback("Size (Vertical)", "KERNEL_SIZE_VERTICAL", m_numTapsVertical);
				callback("Max components", "MAX_COMPONENTS", m_maxComponents);
				callback("Max kernel radius", "MAX_KERNEL_RADIUS", m_maxKernelRadius);
				callback("Max kernel size", "MAX_KERNEL_SIZE", m_maxKernelSize);
				callback("Max object distances", "MAX_OBJECT_DISTANCES", m_maxObjectDistances);
			}

			////////////////////////////////////////////////////////////////////////////////
			template<typename It, typename Fn>
			void transformParameters(It outputIt, Fn const& transformFn) const
			{
				forEachParameter([&](std::string const& name, std::string const& id, auto const& value)
					{ *(outputIt++) = transformFn(name, id, value); });
			}

			////////////////////////////////////////////////////////////////////////////////
			inline size_t numParameters() const
			{
				size_t count = 0;
				forEachParameter([&](std::string const& name, std::string const& id, auto const& value)
					{ ++count; });
				return count;
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		std::string shaderParametersToString(ShaderParameters const& parameters)
		{
			std::stringstream result;

			parameters.forEachParameter([&](std::string const& name, std::string const& id, auto const& value)
				{
					result << name << ": " << value << std::endl;
				});

			return result.str();
		}

		////////////////////////////////////////////////////////////////////////////////
		void emitProfilerShaderParameters(Scene::Scene& scene, ShaderParameters const& parameters)
		{
			parameters.forEachParameter([&](std::string const& name, std::string const& id, auto const& value)
				{
					Profiler::storeData(scene, { "Shader Parameters", name }, value);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string shaderNameSuffix(Scene::Scene& scene, Scene::Object* object, ShaderParameters parameters)
		{
			std::stringstream ss;
			parameters.forEachParameter([&](std::string const& name, std::string const& id, auto const& value)
				{
					ss << "_" << value;
				});
			return ss.str();
		}

		////////////////////////////////////////////////////////////////////////////////
		Asset::ShaderParameters shaderDefines(Scene::Scene& scene, Scene::Object* object, ShaderParameters parameters)
		{
			Asset::ShaderParameters result;

			// Append the shader parameters
			parameters.transformParameters(std::back_inserter(result.m_defines), [](std::string const& name, std::string const& id, auto const& value)
				{
					std::stringstream ss;
					ss << id << " " << value;
					return ss.str();
				});

			// Add the meta enums
			result.m_enums = Asset::generateMetaEnumDefines
			(
				ComplexBlurComponent::OutputMode_meta
			);

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getFullShaderName(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName, std::string const& suffix)
		{
			return Asset::getShaderName("Aberration/ComplexBlur", shaderName, suffix);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getFullShaderName(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName, ShaderParameters const& parameters)
		{
			return getFullShaderName(scene, object, shaderName, shaderNameSuffix(scene, object, parameters));
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getShaderName(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName)
		{
			return "ComplexBlur/" + object->m_name + "_" + shaderName;
		}

		////////////////////////////////////////////////////////////////////////////////
		void bindShader(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName, ShaderParameters const& shaderParameters)
		{
			std::string const& fullShaderName = getFullShaderName(scene, object, shaderName, shaderParameters);

			Debug::log_trace() << "Binding shader: " << shaderName << " (" << fullShaderName << ")" << Debug::end;

			Scene::bindShader(scene, fullShaderName);
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadShader(Scene::Scene& scene, Scene::Object* object, std::string const& name, std::string const& fullName, Asset::ShaderParameters const& defines)
		{
			if (scene.m_shaders.find(fullName) == scene.m_shaders.end() || scene.m_shaders[fullName].m_program == 0)
				Asset::loadShader(scene, "Aberration/ComplexBlur", name, fullName, defines);
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadShaders(Scene::Scene& scene, Scene::Object* object)
		{
			Debug::log_trace() << "Loading shaders for " << object->m_name << Debug::end;

			// Various shader properties necessary
			const ShaderParameters parameters(scene, object);
			const std::string nameSuffix = shaderNameSuffix(scene, object, parameters);
			const Asset::ShaderParameters defines = shaderDefines(scene, object, parameters);
			const std::vector<std::string> names = shaderNames(scene, object);

			// Load the the shaders, if needed
			for (auto const& name : names)
				loadShader(scene, object, name, getFullShaderName(scene, object, name, nameSuffix), defines);

			Debug::log_trace() << "Shaders successfully loaded for " << object->m_name << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace ResourceLoading
	{
		////////////////////////////////////////////////////////////////////////////////
		void initFramebuffers(Scene::Scene& scene, Scene::Object* = nullptr)
		{
			auto maxResolution = GPU::maxResolution();
			Scene::createTexture(scene, "ComplexBlur_Downscaled", GL_TEXTURE_2D,
				maxResolution.x, maxResolution.y, 1, GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			Scene::createTexture(scene, "ComplexBlur_Dilated0", GL_TEXTURE_2D,
				maxResolution.x, maxResolution.y, 1, GL_RG16F, GL_RG, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_2_ENUM);
			Scene::createTexture(scene, "ComplexBlur_Dilated1", GL_TEXTURE_2D,
				maxResolution.x, maxResolution.y, 1, GL_RG16F, GL_RG, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_2_ENUM);
			Scene::createTexture(scene, "ComplexBlur_Result", GL_TEXTURE_2D,
				maxResolution.x, maxResolution.y, 1, GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_3_ENUM);
			Scene::createTexture(scene, "ComplexBlur_Buffer_Near", GL_TEXTURE_2D_ARRAY, 
				maxResolution.x, maxResolution.y, MAX_TEXTURES, GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_4_ENUM);
			Scene::createTexture(scene, "ComplexBlur_Buffer_Far", GL_TEXTURE_2D_ARRAY,
				maxResolution.x, maxResolution.y, MAX_TEXTURES, GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_5_ENUM);
		}

		////////////////////////////////////////////////////////////////////////////////
		void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
		{
			Scene::createGPUBuffer(scene, "ComplexBlurCommon", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
			Scene::createGPUBuffer(scene, "ComplexBlurWeights", GL_SHADER_STORAGE_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_2);
		}

		////////////////////////////////////////////////////////////////////////////////
		void initAberrationPresets(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::initAberrationStructure(scene, Psfs::getAberration(scene, object), Psfs::getAberrationPresets(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadNeuralNetworks(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::loadNeuralNetworks(scene, Psfs::getAberration(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadShaders(Scene::Scene& scene, Scene::Object* object)
		{
			// Load common shaders for the aberration structure (needed for PSF stack computation)
			Aberration::loadShaders(scene, Psfs::getAberration(scene, object));

			// Load the shaders of this object
			Shaders::loadShaders(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Computations
	{
		////////////////////////////////////////////////////////////////////////////////
		void initPsfStack(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "PSF Computation", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Psfs::initPsfStack(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void computePsfs(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "PSF Computation", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Psfs::computePsfs(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeKernel(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "Kernel Computation", 
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Kernel::computeKernel(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadGaussianKernelSettings(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "Kernel Computation", 
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Kernel::loadGaussianKernelSettings(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadGarciaKernelSettings(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "Kernel Computation",
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Kernel::loadGarciaKernelSettings(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void computeKernelSettings(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "Kernel Settings", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Kernel::updateKernelSettings(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadShaders(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "Shader Loading", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					ResourceLoading::loadShaders(scene, &object);
				});
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, ResourceLoading::loadShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::NeuralNetwork, ResourceLoading::loadNeuralNetworks, "Neural Networks");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Texture, ResourceLoading::initFramebuffers, "FrameBuffers");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, ResourceLoading::initGPUBuffers, "Generic GPU Buffers");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Custom, ResourceLoading::initAberrationPresets, "Aberration Presets");

		// Init everything
		Computations::initPsfStack(scene, &object);
		Computations::computePsfs(scene, &object);
		Computations::computeKernelSettings(scene, &object);
		Computations::computeKernel(scene, &object);
		Computations::loadShaders(scene, &object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Various numbers needed later
		auto& aberration = Psfs::getAberration(scene, object);
		auto& psfStack = Psfs::getPsfStack(scene, object);
		const size_t numDefocuses = psfStack.m_psfs.size();
		auto const& psfParameters = aberration.m_psfParameters.m_evaluatedParameters;
		const size_t focusedPsfId = Psfs::findMinDefocusPsfIndex(scene, object)[0];

		// Generate the shader parameters
		const Shaders::ShaderParameters shaderParameters = Shaders::ShaderParameters(scene, object);

		// Render resolution
		const glm::vec2 renderResolution = RenderSettings::getResolutionById(scene, object->component<ComplexBlur::ComplexBlurComponent>().m_renderResolutionId);
		const glm::vec2 sceneResolution = RenderSettings::getResolutionById(scene, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId);
		const glm::vec2 maxResolution = RenderSettings::getResolutionById(scene, 0);
		const glm::vec2 dilatedResolution = Dilation::getDilatedResolution(scene, object, renderResolution);

		// Set the OpenGL state
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// Bind the scene texture
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings);

		// Upload the uniform parameters
		UniformDataCommon blurDataCommon;
		blurDataCommon.m_resolution = renderResolution;
		blurDataCommon.m_dilatedResolution = dilatedResolution;
		blurDataCommon.m_uvScale = renderResolution / maxResolution;
		blurDataCommon.m_kernelTaps = object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius;
		blurDataCommon.m_horizontalDir = Kernel::getBlurDirection(scene, object, renderSettings, 0.0f, blurDataCommon.m_kernelTaps);
		blurDataCommon.m_verticalDir = Kernel::getBlurDirection(scene, object, renderSettings, glm::half_pi<float>(), blurDataCommon.m_kernelTaps);
		blurDataCommon.m_outputMode = object->component<ComplexBlur::ComplexBlurComponent>().m_outputMode;
		blurDataCommon.m_maxBlurSize = Kernel::getBlurRadiusPixels(scene, object, renderSettings, camera);
		blurDataCommon.m_sampleSize = blurDataCommon.m_maxBlurSize / blurDataCommon.m_kernelTaps;
		blurDataCommon.m_dilationPasses = object->component<ComplexBlur::ComplexBlurComponent>().m_dilationPasses;
		blurDataCommon.m_dilationSearchRadius = object->component<ComplexBlur::ComplexBlurComponent>().m_dilationSearchRadius;
		blurDataCommon.m_dilatedTileSize = Dilation::getDilatedTileSize(scene, object);
		blurDataCommon.m_ellipseRotation = object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation;
		blurDataCommon.m_ellipseRatio = object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRatio;
		blurDataCommon.m_ellipseContraction = object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseContraction;
		blurDataCommon.m_numObjectDistances = numDefocuses;
		blurDataCommon.m_objectDistancesMin = psfParameters.m_objectDioptres.front();
		blurDataCommon.m_objectDistancesMax = psfParameters.m_objectDioptres.back();
		blurDataCommon.m_objectDistancesStep = psfParameters.m_objectDistancesRange.m_step;
		for (size_t i = 0; i < numDefocuses; ++i)
		{
			// - negative: far-field
			// - positive: near-field
			// 
			// - focusedPsfId: index of the focus distance PSF slice
			// - in meters: positive -> focusedPsfId -> negative, but must reverse order in dioptres
			const float multiplier = i <= focusedPsfId ? -1.0f : 1.0f;
			blurDataCommon.m_blurSizes[i].x = multiplier * Kernel::getBlurRadiusPixels(scene, object, renderSettings, camera, i);
		}
		uploadBufferData(scene, "ComplexBlurCommon", blurDataCommon);

		// Upload the kernel weights
		auto const& kernel = Kernel::getKernel(scene, object);
		auto const& convolutionKernel = object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel;
		UniformDataWeights blurDataWeights;
		for (size_t i = 0; i < object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents; ++i)
		{
			// Brackets
			blurDataWeights.m_bracketsHorizontal[i] = glm::vec4
			(
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_offsets[i].x,
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_offsets[i].y,
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_scales[i].x,
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_scales[i].y
			);
			blurDataWeights.m_bracketsVertical[i] = glm::vec4
			(
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_offsets[i].z,
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_offsets[i].w,
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_scales[i].z,
				object->component<ComplexBlur::ComplexBlurComponent>().m_convolutionKernel.m_scales[i].w
			);

			// Component weights
			blurDataWeights.m_weights[i] = glm::vec4(kernel.m_components[i].m_A, kernel.m_components[i].m_B, 0.0f, 0.0f);

			// Horizontal components
			auto const& kernelHorizontal = convolutionKernel.m_horizontal[i];
			for (size_t j = 0; j < kernelHorizontal.size(); ++j)
				blurDataWeights.m_componentsHorizontal[i * kernelHorizontal.size() + j] = glm::vec4(
					kernelHorizontal[j].m_a, kernelHorizontal[j].m_b,  // original
					kernelHorizontal[j].m_A, kernelHorizontal[j].m_B); // bracketed

			// Vertical components
			auto const& kernelVertical = convolutionKernel.m_vertical[i];
			for (size_t j = 0; j < kernelVertical.size(); ++j)
				blurDataWeights.m_componentsVertical[i * kernelVertical.size() + j] = glm::vec4(
					kernelVertical[j].m_a, kernelVertical[j].m_b,   // original
					kernelVertical[j].m_A, kernelVertical[j].m_B);  // bracketed
		}
		uploadBufferData(scene, "ComplexBlurWeights", blurDataWeights);

		// Bind the necessary buffers
		Scene::bindBuffer(scene, "ComplexBlurCommon");
		Scene::bindBuffer(scene, "ComplexBlurWeights");

		// Downscale
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Downscale");

			// Bind the output FBO
			Scene::bindFramebuffer(scene, "ComplexBlur_Downscaled");
			glViewport(0, 0, renderResolution.x, renderResolution.y);

			// Bind the shader
			Shaders::bindShader(scene, object, "complex_blur_downscale", shaderParameters);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);

			// Bind the downscaled texture
			Scene::bindTexture(scene, "ComplexBlur_Downscaled");
		}

		// circle-of-confusion dilation
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "CoC Dilate");

			// Bind the shader
			Shaders::bindShader(scene, object, "complex_blur_coc_dilate", shaderParameters);

			for (size_t dilatePassId = 0; dilatePassId < object->component<ComplexBlur::ComplexBlurComponent>().m_dilationPasses; ++dilatePassId)
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Pass #" + std::to_string(dilatePassId));

				const int dilatedTileSizePrev = Dilation::getDilatedTileSize(scene, object, dilatePassId);
				const int dilatedTileSizeCurr = Dilation::getDilatedTileSize(scene, object, dilatePassId + 1);
				const glm::vec2 dilatedResolutionPrev = Dilation::getDilatedResolution(renderResolution, dilatedTileSizePrev);
				const glm::vec2 dilatedResolutionCurr = Dilation::getDilatedResolution(renderResolution, dilatedTileSizeCurr);

				// Bind the output FBO
				Scene::bindFramebuffer(scene, "ComplexBlur_Dilated" + std::to_string(dilatePassId % 2));
				glViewport(0, 0, dilatedResolutionCurr.x, dilatedResolutionCurr.y);

				// Upload the current pass ID
				glUniform1ui(0, dilatePassId);
				glUniform1ui(1, dilatedTileSizePrev);
				glUniform1ui(2, dilatedTileSizeCurr);
				glUniform2f(3, dilatedResolutionPrev.x, dilatedResolutionPrev.y);
				glUniform2f(4, dilatedResolutionCurr.x, dilatedResolutionCurr.y);

				// Render the fullscreen quad
				RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);

				// Bind the dilated CoC texture
				Scene::bindTexture(scene, "ComplexBlur_Dilated" + std::to_string(dilatePassId % 2));
			}
		}

		// Horizontal pass - near
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Horizontal (Near)");

			// Bind the output FBO
			Scene::bindFramebuffer(scene, "ComplexBlur_Buffer_Near");
			glViewport(0, 0, renderResolution.x, renderResolution.y);

			// Bind the shader
			Shaders::bindShader(scene, object, "complex_blur_horizontal_near", shaderParameters);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Horizontal pass
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Horizontal (Far)");

			// Bind the output FBO
			Scene::bindFramebuffer(scene, "ComplexBlur_Buffer_Far");
			glViewport(0, 0, renderResolution.x, renderResolution.y);

			// Bind the shader
			Shaders::bindShader(scene, object, "complex_blur_horizontal_far", shaderParameters);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Vertical pass
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Vertical");

			Scene::bindFramebuffer(scene, "ComplexBlur_Result");
			glViewport(0, 0, renderResolution.x, renderResolution.y);

			// Bind the buffer texture
			Scene::bindTexture(scene, "ComplexBlur_Buffer_Near");
			Scene::bindTexture(scene, "ComplexBlur_Buffer_Far");

			// Bind the shader
			Shaders::bindShader(scene, object, "complex_blur_vertical", shaderParameters);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);

			// Bind the downscaled texture
			Scene::bindTexture(scene, "ComplexBlur_Result");
		}

		// Final composite
		{
			// Bind the output FBO
			RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings);
			RenderSettings::setupViewportOpenGL(scene, simulationSettings, renderSettings);

			// Bind the shader
			Shaders::bindShader(scene, object, "complex_blur_composite", shaderParameters);

			// Render the fullscreen quad
			RenderSettings::renderFullscreenPlaneOpenGL(scene, simulationSettings, renderSettings);
		}

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);

		// Reset color mask
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	}

	////////////////////////////////////////////////////////////////////////////////
	auto generateGuiPreviewImage(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, std::string const& tooltip, std::string const& textureSuffix)
	{
		ImGui::Image(&(scene.m_textures[object->m_name + "_" + textureSuffix].m_texture), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text(tooltip.c_str());
			ImGui::Image(&(scene.m_textures[object->m_name + "_" + textureSuffix].m_texture), ImVec2(1024, 1024), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			ImGui::EndTooltip();
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

		bool aberrationChanged = false;
		bool kernelChanged = false;
		bool fitChanged = false;

		if (ImGui::BeginTabBar(object->m_name.c_str()) == false)
			return;

		// Restore the selected tab id
		std::string activeTab;
		if (auto activeTabSynced = EditorSettings::consumeEditorProperty<std::string>(scene, object, "MainTabBar_SelectedTab#Synced"); activeTabSynced.has_value())
			activeTab = activeTabSynced.value();

		// Aberration settings
		if (ImGui::BeginTabItem("Aberration", activeTab.c_str()))
		{
			auto [coreChangedTmp, aberrationChangedTmp] = Aberration::generateGui(scene, guiSettings, object,
				Psfs::getAberration(scene, object), Psfs::getAberrationPresets(scene, object));
			aberrationChanged |= coreChangedTmp;
			aberrationChanged |= aberrationChangedTmp;

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// Convolution settings
		if (ImGui::BeginTabItem("Convolution", activeTab.c_str()))
		{
			ImGui::Combo("Render Resolution", &object->component<ComplexBlur::ComplexBlurComponent>().m_renderResolutionId, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolutionNames);
			ImGui::Combo("Output Mode", &object->component<ComplexBlur::ComplexBlurComponent>().m_outputMode, ComplexBlur::ComplexBlurComponent::OutputMode_meta);
			ImGui::SliderInt("Dilation Steps", &object->component<ComplexBlur::ComplexBlurComponent>().m_dilationPasses, 1, 8);
			ImGui::SliderInt("Dilation Search Radius", &object->component<ComplexBlur::ComplexBlurComponent>().m_dilationSearchRadius, 1, 32);
			kernelChanged |= ImGui::SliderInt("Components", &object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents, 1, 3);
			kernelChanged |= ImGui::SliderInt("Taps", &object->component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius, 2, 32);
			kernelChanged |= ImGui::SliderFloat("Blur Size (mum)", &object->component<ComplexBlur::ComplexBlurComponent>().m_blurSize, 0.01f, 1600.0f);
			kernelChanged |= ImGui::SliderAngle("Ellipse Rotation", &object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation, 0.0f, 360.0f);
			kernelChanged |= ImGui::SliderFloat("Ellipse Ratio", &object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseRatio, 0.0f, 1.0f);
			kernelChanged |= ImGui::SliderFloat("Ellipse Contraction", &object->component<ComplexBlur::ComplexBlurComponent>().m_ellipseContraction, 0.0f, 1.0f);

			fitChanged |= ImGui::Checkbox("Align Kernel to PSF", &object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelToPsf);
			ImGui::SameLine();
			fitChanged |= ImGui::Checkbox("Fit Kernel to PSF", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelToPsf);
			
			if (ImGui::Button("Load Gaussian"))
			{
				Computations::loadGaussianKernelSettings(scene, object);
				kernelChanged = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Garcia"))
			{
				Computations::loadGarciaKernelSettings(scene, object);
				kernelChanged = true;
			}

			if (object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelToPsf)
			{
				if (ImGui::TreeNodeEx("Align Kernel"))
				{
					ImGui::PushID("AlignKernelSettings");
					//ImGui::Dummy(ImVec2(0.0f, 15.0f));
					//ImGui::TextDisabled("Align Kernel");
					fitChanged |= ImGui::SliderFloat("Ellipse Threshold", &object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_ellipseThreshold, 0.0001f, 0.1f);
					fitChanged |= ImGui::SliderFloat("Target Defocus", &object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_targetDefocus, 0.0f, 1000.0f);
					fitChanged |= ImGui::Checkbox("Export PSF", &object->component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_exportPsf);

					if (ImGui::TreeNodeEx("Preview"))
					{
						//ImGui::Dummy(ImVec2(0.0f, 15.0f));
						//ImGui::TextDisabled("Alignment Target");
						generateGuiPreviewImage(scene, guiSettings, object, "Target PSF", "AlignPsfBase");
						ImGui::SameLine();
						generateGuiPreviewImage(scene, guiSettings, object, "Transformed PSF", "AlignPsfTransformed");

						ImGui::TreePop();
					}

					ImGui::PopID();
					ImGui::TreePop();
				}
			}

			if (object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelToPsf)
			{
				if (ImGui::TreeNodeEx("Fit Kernel"))
				{
					ImGui::PushID("FitKernelSettings");

					//ImGui::Dummy(ImVec2(0.0f, 15.0f));
					//ImGui::TextDisabled("Fit Kernel");
					ImGui::SliderInt("Max Iterations", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_maxIterations, 1, 1e5); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat("Max Minutes", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.maxMinutes, 0.0f, 60.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat("Diff Step Size", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_diffStepSize, 0.0f, 1.0f, "%.6f"); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderInt("Kernel Scale", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_fitScale, 1, 64); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat("Target Defocus", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_targetDefocus, 0.0f, 100.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat("Ellipse Threshold", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_ellipseThreshold, 0.005f, 1.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();

					ImGui::DragFloat4("Initial Component", glm::value_ptr(object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_initialComponents), -10.0f, 10.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat2("Limits - R", glm::value_ptr(object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_radiusLimits), 0.0f, 5.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat2("Limits - a", glm::value_ptr(object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_aLimits), -10.0f, 10.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat2("Limits - b", glm::value_ptr(object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_bLimits), -10.0f, 10.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat2("Limits - A", glm::value_ptr(object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_ALimits), -10.0f, 10.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();
					ImGui::SliderFloat2("Limits - B", glm::value_ptr(object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_BLimits), -10.0f, 10.0f); fitChanged |= ImGui::IsItemDeactivatedAfterEdit();

					fitChanged |= ImGui::Checkbox("Project PSF", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_projectPsf);
					ImGui::SameLine();
					fitChanged |= ImGui::Checkbox("Log Progress", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_logProgress);

					fitChanged |= ImGui::Checkbox("Export PSF", &object->component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_exportPsf);

					if (ImGui::TreeNodeEx("Preview"))
					{
						//ImGui::Dummy(ImVec2(0.0f, 15.0f));
						//ImGui::TextDisabled("Fit Target");
						generateGuiPreviewImage(scene, guiSettings, object, "Target PSF", "FitPsfBase");
						ImGui::SameLine();
						generateGuiPreviewImage(scene, guiSettings, object, "Transformed PSF", "FitPsfTransformed");

						ImGui::TreePop();
					}

					ImGui::PopID();
					ImGui::TreePop();
				}
			}

			if (ImGui::TreeNodeEx("Kernel"))
			{
				auto& kernel = Kernel::getKernel(scene, object);
				kernelChanged |= ImGui::SliderFloat("Kernel Radius", &kernel.m_radius, 0.0f, 10.0f);
				for (int i = 0; i < object->component<ComplexBlur::ComplexBlurComponent>().m_numComponents; ++i)
				{					
					if (ImGui::TreeNodeEx(&kernel.m_components[i], 0, "Component #%d", i + 1))
					{
						ImGui::PushID(i);

						kernelChanged |= ImGui::SliderFloat("a", &kernel.m_components[i].m_a, -20.0f, 20.0f);
						kernelChanged |= ImGui::SliderFloat("b", &kernel.m_components[i].m_b, -20.0f, 20.0f);
						kernelChanged |= ImGui::SliderFloat("A", &kernel.m_components[i].m_A, -20.0f, 20.0f);
						kernelChanged |= ImGui::SliderFloat("B", &kernel.m_components[i].m_B, -20.0f, 20.0f);

						ImGui::PopID();
						ImGui::TreePop();
					}
				}

				if (ImGui::TreeNodeEx("Preview"))
				{
					// ImGui::Dummy(ImVec2(0.0f, 15.0f));
					// ImGui::TextDisabled("Kernel Preview");
					generateGuiPreviewImage(scene, guiSettings, object, "Low Res Kernel", "Kernel");
					ImGui::SameLine();
					generateGuiPreviewImage(scene, guiSettings, object, "PSF vs. Kernel Difference", "KernelDiff");

					ImGui::Text("Kernel MAV: %.10f", object->component<ComplexBlur::ComplexBlurComponent>().m_fitMav);
					ImGui::Text("Fit MSE: %.10f", object->component<ComplexBlur::ComplexBlurComponent>().m_fitMse);
					ImGui::Text("Fit RMSE: %.10f", object->component<ComplexBlur::ComplexBlurComponent>().m_fitRmse);
					ImGui::Text("Fit PSNR: %.10f", object->component<ComplexBlur::ComplexBlurComponent>().m_fitPsnr);

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (ImGui::Button("Export"))
			{
				Kernel::exportKernels(scene, object);
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// End the tab bar
		ImGui::EndTabBar();

		// Updates
		if (aberrationChanged)
		{
			Computations::computePsfs(scene, object);
		}

		if (aberrationChanged || fitChanged)
		{
			Computations::computeKernelSettings(scene, object);
		}

		if (aberrationChanged || kernelChanged || fitChanged)
		{
			Computations::computeKernel(scene, object);
		}

		if (kernelChanged)
		{
			Computations::loadShaders(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void demoSetup(Scene::Scene& scene)
	{
		// @CONSOLE_VAR(Scene, Object Groups, -object_group, Aberration_ComplexBlur)
		SimulationSettings::createGroup(scene, "Aberration", "Aberration_ComplexBlur");

		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

		// Add the debug visualization object.
		auto& complexBlur = createObject(scene, Scene::OBJECT_TYPE_COMPLEX_BLUR, Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, "Aberration_ComplexBlur");

			// Aberration parameters
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_name = Config::AttribValue("aberration").get<std::string>();
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_aberrationParameters.m_type = Aberration::AberrationParameters::Spectacle;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_aberrationParameters.m_spectacleLens.m_sphere = -2.0f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_aberrationParameters.m_spectacleLens.m_cylinder = -0.5f;

			// Aberration computation settings
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_backend = Aberration::PSFStackParameters::ComputationBackend::GPU;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_precomputeVnmLSum = false;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_collectDebugInfo = false;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_logProgress = true;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_logStats = true;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_logDebug = false;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_cropThresholdCoeff = 0.00f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_cropThresholdSum = 0.99f;

			// PSF settings
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_alphaToBetaK = 40;
			
			// PSF parameter sampling
			const float cameraAperture = camera->component<Camera::CameraComponent>().m_fixedAperture;
			const float cameraFocus = camera->component<Camera::CameraComponent>().m_focusDistance;
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_objectDistances = { 0.125f, 8.125f, 81 };
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_lambdas = { 575.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesHorizontal = { 0.0f, 0.0f, 1 };
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesVertical = { 0.0f, 0.0f, 1 };
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_apertureDiameters = { cameraAperture, cameraAperture, 1 };
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_psfParameters.m_focusDistances = { 1.0f / cameraFocus, 1.0f / cameraFocus, 1 };

			// Convolution settings
			object.component<ComplexBlur::ComplexBlurComponent>().m_numComponents = 2;
			object.component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius = 16;
			object.component<ComplexBlur::ComplexBlurComponent>().m_dilationPasses = 1;
			object.component<ComplexBlur::ComplexBlurComponent>().m_dilationSearchRadius = 1;
			object.component<ComplexBlur::ComplexBlurComponent>().m_blurSize = 62.0f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_ellipseRotation = 0.0f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_ellipseRatio = 1.0f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_ellipseContraction = 1.0f;

			// Kernel fit settings
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelToPsf = true;
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_ellipseThreshold = 0.05f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_targetDefocus = 100.0f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_exportPsf = false;

			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelToPsf = false;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_maxIterations = 500;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.maxMinutes = 10.f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_fitScale = 1;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_diffStepSize = 1e-5f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_targetDefocus = 100.0f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_ellipseThreshold = 0.010f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_radiusLimits = glm::vec2{ 0.25f, 5.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_initialComponents = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_aLimits = glm::vec2{ -10.0f, 10.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_bLimits = glm::vec2{ -10.0f, 10.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_ALimits = glm::vec2{ -10.0f, 10.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_BLimits = glm::vec2{ -10.0f, 10.0f };
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_logProgress = true;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_projectPsf = true;
			object.component<ComplexBlur::ComplexBlurComponent>().m_fitKernelSettings.m_exportPsf = false;

			// Default kernels
			Kernel::loadGarciaKernelSettings(scene, &object);

			// Dissertation
			object.component<ComplexBlur::ComplexBlurComponent>().m_aberration.m_aberrationParameters.m_type = Aberration::AberrationParameters::Preset;
			object.component<ComplexBlur::ComplexBlurComponent>().m_kernelTapsRadius = 8;
			// - Myopia
			//    - ellipse ratio: 1.0
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_ellipseThreshold = 0.001f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_targetDefocus = 200.0f;
			// - Astigmatism
			//    - ellipse ratio: 0.92
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_ellipseThreshold = 0.002f;
			object.component<ComplexBlur::ComplexBlurComponent>().m_alignKernelSettings.m_targetDefocus = 200.0f;
		}));
	}
}