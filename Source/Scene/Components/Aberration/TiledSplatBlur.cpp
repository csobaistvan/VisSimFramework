#include "PCH.h"
#include "TiledSplatBlur.h"

namespace TiledSplatBlur
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(TILED_SPLAT_BLUR);
	DEFINE_OBJECT(TILED_SPLAT_BLUR);
	REGISTER_OBJECT_UPDATE_CALLBACK(TILED_SPLAT_BLUR, AFTER, INPUT);
	REGISTER_OBJECT_RENDER_CALLBACK(TILED_SPLAT_BLUR, "Tiled Splat Blur [HDR]", OpenGL, AFTER, "Effects (HDR) [Begin]", 1, 
		&TiledSplatBlur::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, 
		&TiledSplatBlur::renderObjectPreconditionHDROpenGL, nullptr, nullptr);
	REGISTER_OBJECT_RENDER_CALLBACK(TILED_SPLAT_BLUR, "Tiled Splat Blur [LDR]", OpenGL, AFTER, "Effects (LDR) [Begin]", 1, 
		&TiledSplatBlur::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, 
		&TiledSplatBlur::renderObjectPreconditionLDROpenGL, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	namespace Tiling
	{
		////////////////////////////////////////////////////////////////////////////////
		/** Compute the max render resolution. */
		glm::ivec2 computeMaxRenderResolution(Scene::Scene& scene, Scene::Object* object)
		{
			return RenderSettings::getResolutionById(scene, 0);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the normal render resolution. */
		glm::ivec2 computeRenderResolution(Scene::Scene& scene, Scene::Object* object)
		{
			return RenderSettings::getResolutionById(scene, object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_renderResolutionId);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the min and max fovy settings. */
		std::array<float, 1> computeFovy(Scene::Scene& scene, Scene::Object* object)
		{
			Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
			Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);
			return { Camera::getFieldOfView(renderSettings, camera).y };
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the min and max fovy settings. */
		std::array<float, 2> computeFovyLimits(Scene::Scene& scene, Scene::Object* object)
		{
			Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
			Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);
			return Camera::getFieldOfViewLimits(renderSettings, camera);
		}

		////////////////////////////////////////////////////////////////////////////////
		int fragmentBlockSize(Scene::Scene& scene, Scene::Object* object, int mergeSteps)
		{
			return mergeSteps == 0 ? 1 : object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[mergeSteps - 1].m_blockSize;
		}

		////////////////////////////////////////////////////////////////////////////////
		int fragmentBlockSize(Scene::Scene& scene, Scene::Object* object)
		{
			return fragmentBlockSize(scene, object, object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_fragmentMergeSteps);
		}

		////////////////////////////////////////////////////////////////////////////////
		int maxFragmentBlockSize(Scene::Scene& scene, Scene::Object* object)
		{
			return fragmentBlockSize(scene, object, object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets.size() - 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the number of groups corresponding to the parameter resolution and group size */
		glm::ivec2 computeNumGroups(glm::ivec2 resolution, int tileSize, int blockSize = 1)
		{
			return ((resolution + blockSize - 1) / blockSize + tileSize - 1) / tileSize;
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the number of tiles corresponding to the parameter resolution */
		glm::ivec2 computeNumTiles(Scene::Scene& scene, Scene::Object* object, glm::ivec2 resolution, int blockSize = 1)
		{
			return computeNumGroups(resolution, object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize, blockSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the number of tiles corresponding to the render resolution of the object */
		glm::ivec2 computeNumTiles(Scene::Scene& scene, Scene::Object* object, int blockSize = 1)
		{
			return computeNumTiles(scene, object, computeRenderResolution(scene, object), blockSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the padded resolution (size of the area spanned by the spawned tiles) corresponding to the parameter resolution */
		glm::ivec2 computePaddedResolution(Scene::Scene& scene, Scene::Object* object, glm::ivec2 resolution, int blockSize = 1)
		{
			return computeNumTiles(scene, object, resolution, blockSize) * blockSize * object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize;
		}

		////////////////////////////////////////////////////////////////////////////////
		/** Compute the padded resolution (size of the area spanned by the spawned tiles) corresponding to the render resolution of the object */
		glm::ivec2 computePaddedResolution(Scene::Scene& scene, Scene::Object* object)
		{
			return computePaddedResolution(scene, object, computeRenderResolution(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		int fragmentBufferMaxFragmentsPerEntry(Scene::Scene& scene, Scene::Object* object, int layers)
		{
			return layers;
		}

		////////////////////////////////////////////////////////////////////////////////
		float tileBufferLayerMultiplier(int layers)
		{
			return float(layers);
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxCenterFragmentsPerEntry(int layers, int tileSize)
		{
			// Number of entries in the center zone
			return int(tileBufferLayerMultiplier(layers) * tileSize * tileSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxCenterFragmentsPerEntry(Scene::Scene& scene, Scene::Object* object, int layers)
		{
			return tileBufferMaxCenterFragmentsPerEntry(
				layers,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize);
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxFragmentsPerEntry(int layers, int tileSize, int maxCoc)
		{
			// Number of entries in the center zone
			int center = tileSize * tileSize;

			// Number of entries in the neighbor zones (top, bottom, left, right)
			int side = 4.0f * tileSize * maxCoc;

			// Number of entries in the neighbor zones (top, bottom, left, right)
			int corner = 4.0f * maxCoc * maxCoc;

			// Multiply by the number of layers and return
			return int(tileBufferLayerMultiplier(layers) * (center + side + corner));
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxFragmentsPerEntry(Scene::Scene& scene, Scene::Object* object, int layers)
		{
			return tileBufferMaxFragmentsPerEntry(
				layers,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxCoC);
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferNumNeighborTilesSplat(int tileSize, int maxCoc)
		{
			return (maxCoc + tileSize - 1) / tileSize;
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferNumNeighborTilesSplat(Scene::Scene& scene, Scene::Object* object)
		{
			return tileBufferNumNeighborTilesSplat(
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxCoC);
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxSortElements(int maxFragmentsPerEntry)
		{
			return std::next_pow2(maxFragmentsPerEntry);
		}

		////////////////////////////////////////////////////////////////////////////////
		int sortGroupId(int groupSize, int maxSharedIndices)
		{
			return int(glm::log2((float)groupSize)) - int(glm::log2((float)maxSharedIndices));
		}

		////////////////////////////////////////////////////////////////////////////////
		int dispatchElementIndex(int groupSize, int maxSharedIndices)
		{
			return (sortGroupId(groupSize, maxSharedIndices) * (sortGroupId(groupSize, maxSharedIndices) + 1)) / 2;
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxSortSharedElements(int maxSortElements, int maxFragmentsPerEntry)
		{
			return glm::min(tileBufferMaxSortElements(maxFragmentsPerEntry), maxSortElements);
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxSortSharedElements(Scene::Scene& scene, Scene::Object* object, int layers)
		{
			return tileBufferMaxSortSharedElements(
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxSortElements,
				tileBufferMaxFragmentsPerEntry(scene, object, layers));
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxSortIterations(int maxFragmentsPerEntry, int tileBufferMaxSharedSortIndices)
		{
			// Maximum number of fragments in a tile buffer entry
			int maxSortElements = tileBufferMaxSortElements(maxFragmentsPerEntry);

			// Maximum number of total iterations needed
			int maxTotalIterations = (int)glm::log2((float)glm::max(maxSortElements, tileBufferMaxSharedSortIndices));

			// From which this many iterations are inner ones
			int numInnerIterations = (int)glm::log2((float)tileBufferMaxSharedSortIndices);

			// Maximum number of iterations necessary to sort a tile buffer entry
			return maxTotalIterations - numInnerIterations + 1;
		}

		////////////////////////////////////////////////////////////////////////////////
		int tileBufferMaxSortIterations(Scene::Scene& scene, Scene::Object* object, int layers)
		{
			return tileBufferMaxSortIterations(
				tileBufferMaxFragmentsPerEntry(scene, object, layers),
				tileBufferMaxSortSharedElements(scene, object, layers));
		}

		////////////////////////////////////////////////////////////////////////////////
		int dispatchBufferMaxDispatchPerEntry(int maxSortIterations)
		{
			return (maxSortIterations * (maxSortIterations + 1)) / 2;
		}

		////////////////////////////////////////////////////////////////////////////////
		int dispatchBufferMaxDispatchPerEntry(int maxFragmentsPerEntry, int tileBufferMaxSharedSortIndices)
		{
			return dispatchBufferMaxDispatchPerEntry(tileBufferMaxSortIterations(maxFragmentsPerEntry, tileBufferMaxSharedSortIndices));
		}

		////////////////////////////////////////////////////////////////////////////////
		int dispatchBufferMaxDispatchPerEntry(Scene::Scene& scene, Scene::Object* object, int layers)
		{
			return dispatchBufferMaxDispatchPerEntry(
				tileBufferMaxFragmentsPerEntry(scene, object, layers),
				tileBufferMaxSortSharedElements(scene, object, layers));
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Psfs
	{
		////////////////////////////////////////////////////////////////////////////////
		Aberration::WavefrontAberration& getAberration(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<TiledSplatBlurComponent>().m_aberration;
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::PSFStack& getPsfStack(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<TiledSplatBlurComponent>().m_aberration.m_psfStack;
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::WavefrontAberrationPresets& getAberrationPresets(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<TiledSplatBlurComponent>().m_aberrationPresets;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool hasOffAxisPsfs(Scene::Scene& scene, Scene::Object* object)
		{
			return Aberration::getNumHorizontalAngles(scene, getAberration(scene, object)) > 1 ||
				Aberration::getNumVerticalAngles(scene, getAberration(scene, object)) > 1;
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::PsfIndexIterator stackBegin(Scene::Scene& scene, Scene::Object* object)
		{
			return Aberration::psfStackBegin(scene, getAberration(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::PsfIndexIterator stackEnd(Scene::Scene& scene, Scene::Object* object)
		{
			return Aberration::psfStackEnd(scene, getAberration(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::PsfIndex getPsfIndex(Scene::Scene& scene, Scene::Object* object, const size_t psfIndex)
		{
			return Aberration::getPsfIndex(scene, getAberration(scene, object), psfIndex);
		}

		////////////////////////////////////////////////////////////////////////////////
		Aberration::PsfStackElements::PsfEntry& selectEntry(Scene::Scene& scene, Scene::Object* object, Aberration::PsfIndex const& psfIndex)
		{
			return Aberration::getPsfEntry(scene, getAberration(scene, object), psfIndex);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool isIndexNeighbor(const size_t a, const size_t b)
		{
			return ((a - b) <= 1 || (b - a) <= 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool arePsfsNeighbors(Aberration::PsfIndex const& a, Aberration::PsfIndex const& b, const bool offAxis)
		{
			bool result = a != b;
			result &= isIndexNeighbor(a[0], b[0]);
			result &= ((offAxis && isIndexNeighbor(a[1], b[1])) || (!offAxis && a[1] == b[1]));
			result &= ((offAxis && isIndexNeighbor(a[2], b[2])) || (!offAxis && a[2] == b[2]));
			result &= (a[3] == b[3]);
			result &= isIndexNeighbor(a[4], b[4]);
			result &= isIndexNeighbor(a[5], b[5]);
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool arePsfsNeighbors(Scene::Scene& scene, Scene::Object* object, Aberration::PsfIndex const& a, Aberration::PsfIndex const& b)
		{
			return arePsfsNeighbors(a, b, object->component<TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlurComponent::OffAxis);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool arePsfsSame(Aberration::PsfIndex const& a, Aberration::PsfIndex const& b)
		{
			return a == b;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool arePsfsSame(Scene::Scene& scene, Scene::Object* object, Aberration::PsfIndex const& a, Aberration::PsfIndex const& b)
		{
			return arePsfsSame(a, b);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool arePsfsSameOrNeighbors(Aberration::PsfIndex const& a, Aberration::PsfIndex const& b, const bool offAxis)
		{
			return arePsfsSame(a, b) || arePsfsNeighbors(a, b, offAxis);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool arePsfsSameOrNeighbors(Scene::Scene& scene, Scene::Object* object, Aberration::PsfIndex const& a, Aberration::PsfIndex const& b)
		{
			return arePsfsSameOrNeighbors(a, b, object->component<TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlurComponent::OffAxis);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the blur radius (in pixels) for the parameter PSF index at the input resolution and fovy setting
		float blurRadius(Scene::Scene& scene, Scene::Object* object, glm::ivec2 resolution, float fovy,
			Aberration::PsfIndex const& psfIndex)
		{
			return Aberration::blurRadiusPixels(selectEntry(scene, object, psfIndex), resolution, fovy);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the blur radius (in pixels) for the parameter PSF indices at the input resolution and fovy settings
		template<size_t N, size_t M>
		std::array<float, N * M> blurRadius(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 const& resolution, std::array<float, N> const& fovys, std::array<Aberration::PsfIndex, M> const& psfIndices)
		{
			std::array<float, N * M> result;
			for (size_t psfId = 0; psfId < M; ++psfId)
			{
				auto const& psf = selectEntry(scene, object, psfIndices[psfId]);
				for (size_t fovyId = 0; fovyId < N; ++fovyId)
				{
					result[psfId * N + fovyId] = Aberration::blurRadiusPixels(psf, resolution, fovys[fovyId]);
				}
			}
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the blur radius (in pixels) for the parameter PSF index at the input resolution and fovy settings
		template<size_t N>
		std::array<float, N> blurRadius(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 const& resolution, std::array<float, N> const& fovys, Aberration::PsfIndex const& psfIndex)
		{
			return blurRadius(scene, object, resolution, fovys, std::array<Aberration::PsfIndex, 1>{ psfIndex });
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the minimum blur radius (in pixels) for the parameter PSF index given the input resolution and fovy settings
		template<size_t N, size_t M>
		size_t minBlurRadius(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 const& resolution, std::array<float, N> const& fovys, std::array<Aberration::PsfIndex, M> const& psfIndices)
		{
			auto const& radii = blurRadius(scene, object, resolution, fovys, psfIndices);
			return size_t(glm::floor(*std::min_element(radii.begin(), radii.end())));
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the maximum blur radius (in pixels) for the parameter PSF index given the input resolution and fovy settings
		template<size_t N, size_t M>
		size_t maxBlurRadius(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 const& resolution, std::array<float, N> const& fovys, std::array<Aberration::PsfIndex, M> const& psfIndices)
		{
			auto const& radii = blurRadius(scene, object, resolution, fovys, psfIndices);
			return size_t(glm::ceil(*std::max_element(radii.begin(), radii.end())));
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the absolute blur radius difference (in pixels) for the parameter PSF index given the input resolution, 
		// for each neighboring fovy setting
		template<size_t N, size_t M>
		std::array<float, N * (M - 1)> blurRadiusDifference(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 const& resolution, std::array<float, N> const& fovys, std::array<Aberration::PsfIndex, M> const& psfIndices)
		{
			std::array<float, N * (M - 1)> result;
			for (size_t psfId = 0; psfId < (M - 1); ++psfId)
			{
				auto const& psf0 = selectEntry(scene, object, psfIndices[psfId]);
				auto const& psf1 = selectEntry(scene, object, psfIndices[psfId + 1]);
				for (size_t fovyId = 0; fovyId < N; ++fovyId)
				{
					const float r0 = Aberration::blurRadiusPixels(psf0, resolution, fovys[fovyId]);
					const float r1 = Aberration::blurRadiusPixels(psf1, resolution, fovys[fovyId]);
					result[psfId * N + fovyId] = glm::abs(r1 - r0);
				}
			}
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Returns the maximum absolute blur radius difference (in pixels) for the parameter PSF index given the 
		// input resolution and each neighboring fovy setting
		template<size_t N, size_t M>
		size_t maxBlurRadiusDifference(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 const& resolution, std::array<float, N> const& fovys, std::array<Aberration::PsfIndex, M> const& psfIndices)
		{
			auto const& differences = blurRadiusDifference(scene, object, resolution, fovys, psfIndices);
			return size_t(glm::ceil(*std::max_element(differences.begin(), differences.end())));
		}

		////////////////////////////////////////////////////////////////////////////////
		// Predicates for the min and max blur sizes
		static auto const& s_minPred = [](const int p, const float r) { return glm::min(p, int(glm::floor(r))); };
		static auto const& s_maxPred = [](const int p, const float r) { return glm::max(p, int(glm::ceil(r))); };

		////////////////////////////////////////////////////////////////////////////////
		// Predicate to calculate the min and max blur radii, given the previous min and max values and a set of blur radii
		template<size_t N, typename Fp>
		glm::ivec2 minMaxPred(Scene::Scene& scene, Scene::Object* object, glm::ivec2 resolution, std::array<float, N> const& fovys,
			glm::ivec2 const& prev, Fp const& filterPred, Aberration::PsfIndex const& psfIndex)
		{
			if (!filterPred(psfIndex)) return prev;

			std::array<float, N> const& radii = blurRadius(scene, object, resolution, fovys, psfIndex);
			return glm::ivec2
			(
				std::accumulate(radii.begin(), radii.end(), prev[0], s_minPred),
				std::accumulate(radii.begin(), radii.end(), prev[1], s_maxPred)
			);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<size_t N, typename Fp>
		glm::ivec2 blurRadiusLimits(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 resolution, std::array<float, N> const& fovys, Fp const& filterPred)
		{
			return std::accumulate(stackBegin(scene, object), stackEnd(scene, object), glm::ivec2(INT_MAX, -INT_MAX),
				[&](glm::ivec2 prev, auto const& psfIndex)
				{ return minMaxPred(scene, object, resolution, fovys, prev, filterPred, psfIndex); });
		}

		////////////////////////////////////////////////////////////////////////////////
		template<size_t N>
		glm::ivec2 blurRadiusLimits(Scene::Scene& scene, Scene::Object* object,
			glm::ivec2 resolution, std::array<float, N> const& fovys)
		{
			return blurRadiusLimits(scene, object, resolution, fovys, [](auto const&) { return true; });
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec2 blurRadiusLimitsCurrent(Scene::Scene& scene, Scene::Object* object)
		{
			return blurRadiusLimits(scene, object,
				Tiling::computeRenderResolution(scene, object),
				Tiling::computeFovy(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec2 blurRadiusLimitsGlobal(Scene::Scene& scene, Scene::Object* object)
		{
			return blurRadiusLimits(scene, object,
				Tiling::computeMaxRenderResolution(scene, object),
				Tiling::computeFovyLimits(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec2 blurRadiusLimitsEntry(Scene::Scene& scene, Scene::Object* object,
			Aberration::PsfIndex const& psfIndex)
		{
			return blurRadiusLimits(scene, object,
				Tiling::computeMaxRenderResolution(scene, object),
				Tiling::computeFovyLimits(scene, object),
				[&](auto const& neighborIndex) { return arePsfsSameOrNeighbors(scene, object, psfIndex, neighborIndex); });
		}

		////////////////////////////////////////////////////////////////////////////////
		int weightsPerEntrySingleChannel(const int minRadius, const int maxRadius)
		{
			// FullSimplify[Sum[(i * 2 + 1) * (i * 2 + 1), { i, n0, n1 }]]
			return (minRadius - 4 * minRadius * minRadius * minRadius + (1 + maxRadius) * (1 + 2 * maxRadius) * (3 + 2 * maxRadius)) / 3;
		}

		////////////////////////////////////////////////////////////////////////////////
		int weightsPerEntrySingleChannel(const int maxRadius)
		{
			// FullSimplify[Sum[(i * 2 + 1) * (i * 2 + 1), { i, 0, n }]]
			return ((1 + maxRadius) * (1 + 2 * maxRadius) * (3 + 2 * maxRadius)) / 3;
		}

		////////////////////////////////////////////////////////////////////////////////
		int weightsPerEntry(const int maxRadius, const int maxChannels)
		{
			return maxChannels * weightsPerEntrySingleChannel(maxRadius);
		}

		////////////////////////////////////////////////////////////////////////////////
		int weightsPerEntry(const int minRadius, const int maxRadius, const int maxChannels)
		{
			return maxChannels * weightsPerEntrySingleChannel(minRadius, maxRadius);
		}

		////////////////////////////////////////////////////////////////////////////////
		int weightsPerEntry(const glm::ivec2 radii, const int maxChannels)
		{
			return weightsPerEntry(radii[0], radii[1], maxChannels);
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

		////////////////////////////////////////////////////////////////////////////////
		void clearPsfCache(Scene::Scene& scene, Scene::Object* object)
		{
			Aberration::freeCacheResources(scene, getAberration(scene, object));
		}

		////////////////////////////////////////////////////////////////////////////////
		using PsfGpu = Eigen::Matrix<GLfloat, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
		PsfGpu computeGpuPsf(Scene::Scene& scene, Scene::Object* object,
			Aberration::PsfStackElements::PsfEntry const& psfEntry, const size_t radius)
		{
			return Aberration::resizePsfNormalized(scene, getAberration(scene, object), psfEntry.m_psf, radius).cast<GLfloat>();
		}

		////////////////////////////////////////////////////////////////////////////////
		void updateDerivedParameters(Scene::Scene& scene, Scene::Object* object)
		{
			// Extract the total number of PSFs and weights
			const size_t numTotalPsfs = getPsfStack(scene, object).m_psfs.num_elements();

			DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, numTotalPsfs, DateTime::Seconds, "Derived PSF Params");

			// Resize the derived parameters vector
			object->component<TiledSplatBlurComponent>().m_derivedPsfParameters.resize(numTotalPsfs);

			Threading::threadedExecuteIndices(Threading::numThreads(),
				[&](Threading::ThreadedExecuteEnvironment const& environment, size_t psfId)
				{
					// Extract the psf and calculate the needed properties
					const Aberration::PsfIndex psfIndex = Psfs::getPsfIndex(scene, object, psfId);
					Aberration::PsfStackElements::PsfEntry const& psfEntry = Psfs::selectEntry(scene, object, psfIndex);
					const glm::ivec2 blurRadii = Psfs::blurRadiusLimitsEntry(scene, object, psfIndex);

					// Remember the total number of weights for this PSF entry
					const size_t numPsfWeights = Psfs::weightsPerEntry(blurRadii[0], blurRadii[1], 1);

					// Store the relevant props in the output structure
					TiledSplatBlurComponent::DerivedPsfParameters& derivedPsfParameters = 
						object->component<TiledSplatBlurComponent>().m_derivedPsfParameters[psfId];
					derivedPsfParameters.m_minBlurRadius = blurRadii[0];
					derivedPsfParameters.m_maxBlurRadius = blurRadii[1];
					derivedPsfParameters.m_numPsfWeights = numPsfWeights;
					derivedPsfParameters.m_blurRadiusDeg = psfEntry.m_blurRadiusDeg;
				},
				numTotalPsfs);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool isEyeStateFixed(Scene::Scene& scene, Scene::Object* object)
		{
			return getPsfStack(scene, object).m_psfEntryParameters[0][0][0][0].size() == 1 &&
				getPsfStack(scene, object).m_psfEntryParameters[0][0][0][0][0].size() == 1;
		}

		////////////////////////////////////////////////////////////////////////////////
		size_t numTotalPsfs(Scene::Scene& scene, Scene::Object* object)
		{
			return object->component<TiledSplatBlurComponent>().m_derivedPsfParameters.size();
		}

		////////////////////////////////////////////////////////////////////////////////
		size_t numTotalWeights(Scene::Scene& scene, Scene::Object* object)
		{
			size_t result = 0;
			for (auto const& psfParameters : object->component<TiledSplatBlurComponent>().m_derivedPsfParameters)
				result += psfParameters.m_numPsfWeights;
			return result;
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
				"psf_cache_command"s,
				"psf_cache_params"s,
				"psf_cache_texture"s,
				"psf_texture_command_params"s,
				"psf_texture_command_texture"s,
				"psf_texture_params"s,
				"psf_texture_texture"s,
				"fragment_buffer_build"s,
				"tile_buffer_build"s,
				"fragment_buffer_merge"s,
				"tile_buffer_splat"s,
				"tile_buffer_splat_command"s,
				"tile_buffer_sort_params"s,
				"tile_buffer_sort_presort"s,
				"tile_buffer_sort_inner"s,
				"tile_buffer_sort_outer"s,
				"convolution"s,
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		bool isOutputModified(TiledSplatBlurComponent::OutputMode outputMode, TiledSplatBlurComponent::OverlayMode overlayMode)
		{
			return (outputMode != TiledSplatBlurComponent::Convolution || overlayMode != TiledSplatBlurComponent::None);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldUseDebugShader(TiledSplatBlurComponent::ShaderType shaderType, TiledSplatBlurComponent::OutputMode outputMode, TiledSplatBlurComponent::OverlayMode overlayMode)
		{
			return shaderType == TiledSplatBlurComponent::Debug || (shaderType == TiledSplatBlurComponent::Auto && isOutputModified(outputMode, overlayMode));
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldUseDebugShader(Scene::Scene& scene, Scene::Object* object)
		{
			return shouldUseDebugShader(
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_shaderType,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_outputMode,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_overlayMode);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string psfTextureFormatString(TiledSplatBlurComponent::PsfTextureFormat format)
		{
			switch (format)
			{
			case TiledSplatBlurComponent::F11: return "r11f_g11f_b10f";;
			case TiledSplatBlurComponent::F16: return "rgba16f";
			case TiledSplatBlurComponent::F32: return "rgba32f";
			}
			return "";
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string psfTextureFormatString(Scene::Scene& scene, Scene::Object* object)
		{
			return psfTextureFormatString(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat);
		}

		////////////////////////////////////////////////////////////////////////////////
		/** The set of parameters needed for generating a shader. */
		struct ShaderParameters
		{
			// Constructor that sets the default values
			ShaderParameters(Scene::Scene& scene, Scene::Object* object) :
				m_maxLayers(GPU::numLayers()),
				m_tileSize(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize),
				m_maxCoc(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxCoC),
				m_mergeSteps(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_fragmentMergeSteps),
				m_mergedBlockSize(Tiling::fragmentBlockSize(scene, object, m_mergeSteps)),
				m_mergedTileSize((m_tileSize / Tiling::fragmentBlockSize(scene, object, m_mergeSteps))),
				m_interpolationGroupSize(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_interpolation),
				m_mergeGroupSize(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_merge),
				m_splatGroupSize(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_splat),
				m_sortGroupSize(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_sort),
				m_convolutionGroupSize(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_convolution),
				m_isDebugging(shouldUseDebugShader(scene, object) ? 1 : 0),
				m_maxSortIndices(Tiling::tileBufferMaxSortSharedElements(scene, object, m_maxLayers)),
				m_numNeighborTilesSplat(Tiling::tileBufferNumNeighborTilesSplat(m_tileSize, m_maxCoc)),
				m_numSortIterations(Tiling::tileBufferMaxSortIterations(scene, object, m_maxLayers)),
				m_sortElementsPerThread(glm::max(m_maxSortIndices / m_sortGroupSize / 2, 1)),
				m_isEyeStateFixed(Psfs::isEyeStateFixed(scene, object)),
				m_psfAxisMethod(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod),
				m_psfTextureFormat(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat),
				m_psfTextureDepthLayout(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout),
				m_psfTextureAngleLayout(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureAngleLayout)
			{}

			// The various parameters
			int m_maxLayers;
			int m_tileSize;
			int m_maxCoc;
			int m_mergeSteps;
			int m_mergedBlockSize;
			int m_mergedTileSize;
			int m_interpolationGroupSize;
			int m_mergeGroupSize;
			int m_splatGroupSize;
			int m_sortGroupSize;
			int m_convolutionGroupSize;
			int m_isDebugging;
			int m_maxSortIndices;
			int m_numNeighborTilesSplat;
			int m_numSortIterations;
			int m_sortElementsPerThread;
			int m_isEyeStateFixed;
			TiledSplatBlurComponent::PsfAxisMethod m_psfAxisMethod;
			TiledSplatBlurComponent::PsfTextureFormat m_psfTextureFormat;
			TiledSplatBlurComponent::PsfTextureDepthLayout m_psfTextureDepthLayout;
			TiledSplatBlurComponent::PsfTextureAngleLayout m_psfTextureAngleLayout;

			////////////////////////////////////////////////////////////////////////////////
			template<typename Fn>
			void forEachParameter(Fn const& callback) const
			{
				callback("Layers",                      "MAX_LAYERS", m_maxLayers);
				callback("Tile Size",                   "TILE_SIZE", m_tileSize);
				callback("Merge Steps",                 "MERGE_STEPS", m_mergeSteps);
				callback("Merged Tile Size",            "MERGED_TILE_SIZE", m_mergedTileSize);
				callback("Merged Block Size",           "MERGED_BLOCK_SIZE", m_mergedBlockSize);
				callback("Interpolation Group Size",    "INTERPOLATION_GROUP_SIZE", m_interpolationGroupSize);
				callback("Merged Group Size",           "FRAGMENT_MERGE_GROUP_SIZE", m_mergeGroupSize);
				callback("Splat Group Size",            "SPLAT_GROUP_SIZE", m_splatGroupSize);
				callback("Sort Group Size",             "SORT_GROUP_SIZE", m_sortGroupSize);
				callback("Convolution Group Size",      "CONVOLUTION_GROUP_SIZE", m_convolutionGroupSize);
				callback("Max Sort Indices",            "SORT_SHARED_ARRAY_SIZE", m_maxSortIndices);
				callback("Num Splat Neighbor Tiles",    "NUM_NEIGHBOR_TILES_SPLAT", m_numNeighborTilesSplat);
				callback("Num Sort Iterations",         "NUM_SORT_ITERATIONS", m_numSortIterations);
				callback("Sort Elements Per Thread",    "SORT_ELEMENTS_PER_THREAD", m_sortElementsPerThread);
				callback("Is Debugging",                "DEBUG_OUTPUT", m_isDebugging);
				callback("Max PSF Radius",              "MAX_PSF_RADIUS", m_maxCoc);
				callback("Max PSF Diameter",            "MAX_PSF_DIAMETER", m_maxCoc * 2 + 1);
				callback("Fixed Eye State",             "FIXED_EYE_STATE", m_isEyeStateFixed ? 1 : 0);
				callback("PSF Texture Format",          "PSF_TEXTURE_FORMAT", std::string(TiledSplatBlurComponent::PsfTextureFormat_value_to_string(m_psfTextureFormat)));
				callback("PSF Texture Format",          "PSF_TEXTURE_FORMAT_ID", m_psfTextureFormat);
				callback("PSF Texture Format String",   "PSF_TEXTURE_TYPE", psfTextureFormatString(m_psfTextureFormat));
				callback("PSF Axis Method",             "PSF_AXIS_METHOD", std::string(TiledSplatBlurComponent::PsfAxisMethod_value_to_string(m_psfAxisMethod)));
				callback("PSF Texture Layout",          "PSF_TEXTURE_DEPTH_LAYOUT", std::string(TiledSplatBlurComponent::PsfTextureDepthLayout_value_to_string(m_psfTextureDepthLayout)));
				callback("PSF Texture Layout",          "PSF_TEXTURE_ANGLE_LAYOUT", std::string(TiledSplatBlurComponent::PsfTextureAngleLayout_value_to_string(m_psfTextureAngleLayout)));
				callback("PSF Axis Method",             "PSF_AXIS_METHOD_ID", m_psfAxisMethod);
				callback("PSF Texture Layout",          "PSF_TEXTURE_DEPTH_LAYOUT_ID", m_psfTextureDepthLayout);
				callback("PSF Texture Layout",          "PSF_TEXTURE_ANGLE_LAYOUT_ID", m_psfTextureAngleLayout);
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
				TiledSplatBlurComponent::OutputMode_meta,
				TiledSplatBlurComponent::OverlayMode_meta,
				TiledSplatBlurComponent::AccumulationMethod_meta,
				TiledSplatBlurComponent::PsfAxisMethod_meta,
				TiledSplatBlurComponent::WeightScaleMethod_meta,
				TiledSplatBlurComponent::WeightRescaleMethod_meta,
				TiledSplatBlurComponent::CoefficientLerpMethod_meta,
				TiledSplatBlurComponent::PsfTextureDepthLayout_meta,
				TiledSplatBlurComponent::PsfTextureAngleLayout_meta
			);

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getFullShaderName(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName, std::string const& suffix)
		{
			return Asset::getShaderName("Aberration/TiledSplatBlur", shaderName, suffix);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string getFullShaderName(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName, ShaderParameters const& parameters)
		{
			return getFullShaderName(scene, object, shaderName, shaderNameSuffix(scene, object, parameters));
		}

		////////////////////////////////////////////////////////////////////////////////
		void loadShader(Scene::Scene& scene, Scene::Object* object, std::string const& name, std::string const& fullName, Asset::ShaderParameters const& defines)
		{
			if (scene.m_shaders.find(fullName) == scene.m_shaders.end() || scene.m_shaders[fullName].m_program == 0)
				Asset::loadShader(scene, "Aberration/TiledSplatBlur", name, fullName, defines);
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

		////////////////////////////////////////////////////////////////////////////////
		void bindShader(Scene::Scene& scene, Scene::Object* object, std::string const& shaderName, ShaderParameters const& shaderParameters)
		{
			std::string const& fullShaderName = getFullShaderName(scene, object, shaderName, shaderParameters);

			Debug::log_trace() << "Binding shader: " << shaderName << " (" << fullShaderName << ")" << Debug::end;

			Scene::bindShader(scene, fullShaderName);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace PsfTexture
	{
		////////////////////////////////////////////////////////////////////////////////
		std::string textureName(Scene::Scene& scene, Scene::Object* object)
		{
			return object->m_name + "_" + "BasePsfs";
		}

		////////////////////////////////////////////////////////////////////////////////
		std::string textureNameCache(Scene::Scene& scene, Scene::Object* object, const size_t a, const size_t f)
		{
			return object->m_name + "_" + "PsfCache" + std::to_string(a) + "_" + std::to_string(f);
		}

		////////////////////////////////////////////////////////////////////////////////
		std::vector<std::string> textureNames(Scene::Scene& scene, Scene::Object* object)
		{
			return std::vector<std::string>
			{
				textureName(scene, object)
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		GLenum textureFormatEnum(Scene::Scene& scene, Scene::Object* object)
		{
			switch (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat)
			{
			case TiledSplatBlurComponent::F11: return GL_R11F_G11F_B10F;
			case TiledSplatBlurComponent::F16: return GL_RGBA16F;
			case TiledSplatBlurComponent::F32: return GL_RGBA32F;
			}
			return GL_R11F_G11F_B10F;
		}

		////////////////////////////////////////////////////////////////////////////////
		GLenum imageFormatEnum(Scene::Scene& scene, Scene::Object* object)
		{
			switch (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat)
			{
			case TiledSplatBlurComponent::F11: return GL_R11F_G11F_B10F;
			case TiledSplatBlurComponent::F16: return GL_RGBA16F;
			case TiledSplatBlurComponent::F32: return GL_RGBA32F;
			}
			return GL_RGBA8;
		}

		////////////////////////////////////////////////////////////////////////////////
		size_t textureSize(Scene::Scene& scene, Scene::Object* object, const glm::ivec3 dimensions)
		{
			return GPU::textureSizeBytes(dimensions, textureFormatEnum(scene, object), 0, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		float calcNumSlicesReduction(const size_t maxRadius, const size_t maxDiff, const float s, const float p)
		{
			return 1.0f / glm::max(glm::pow(maxDiff * s, p), 1.0f);
		}

		////////////////////////////////////////////////////////////////////////////////
		size_t calcNumReducedSlices(const size_t maxRadius, const size_t maxDiff, const float s, const float p)
		{
			return std::max(size_t(1), size_t(float(maxDiff) * calcNumSlicesReduction(maxRadius, maxDiff, s, p)));
		}

		////////////////////////////////////////////////////////////////////////////////
		struct SliceInfo
		{
			struct PerSliceInfo
			{
				size_t m_numSlices = 0;
				size_t m_maxDiff = 0;
				float m_numLayerReduction = 0.0f;
				std::array<size_t, 2> m_blurRadii = { 999, 0 };
				Aberration::PsfIndex m_psfIndex;
			};

			std::array<float, 6> m_s;
			std::array<float, 6> m_p;
			std::array<size_t, 6> m_numSlices;
			std::array<size_t, 6> m_numSlicesUnreduced;
			std::array<Aberration::PsfIndex, 6> m_selectedPsfIndex;
			std::array<std::array<size_t, 2>, 6> m_psfBurRadii;
			std::array<float, 6> m_numLayerReduction;
			std::array<std::vector<PerSliceInfo>, 6> m_slices;

			void logStats() const
			{
				Debug::log_debug() << std::string(80, '=') << Debug::end;
				Debug::log_debug() << "Number of texture layers needed for each axis: " << m_numSlices << Debug::end;
				Debug::log_debug() << " - Without reductions: " << m_numSlicesUnreduced << Debug::end;
				for (size_t axisId = 0; axisId < 6; ++axisId)
				{
					Debug::log_debug() << std::string(80, '-') << Debug::end;
					Debug::log_debug() << "Axis #" << axisId << Debug::end;
					//Debug::log_debug() << std::string(80, '-') << Debug::end;
					Debug::log_debug() << "- s: " << m_s[axisId] << ", p: " << m_p[axisId] << Debug::end;
					Debug::log_debug() << "- PSF#: " << m_selectedPsfIndex[axisId] << ", "
						<< "radii: " << m_psfBurRadii[axisId] << ", "
						<< "reduction: " << m_numLayerReduction[axisId]
						<< Debug::end;
					Debug::log_debug() << std::string(80, '-') << Debug::end;
					for (size_t sliceId = 0; sliceId < m_slices[axisId].size(); ++sliceId)
					{
						Debug::log_debug()
							<< "  - " << sliceId << ": " << m_slices[axisId][sliceId].m_numSlices << " - "
							<< "Blur radii: " << m_slices[axisId][sliceId].m_blurRadii << ", "
							<< "Max diff: " << m_slices[axisId][sliceId].m_maxDiff << ", "
							<< "Reduction: " << m_slices[axisId][sliceId].m_numLayerReduction << ", "
							<< "Index: " << m_slices[axisId][sliceId].m_psfIndex
							<< Debug::end;
					}
				}

				Debug::log_debug() << std::string(80, '=') << Debug::end;
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		SliceInfo slicesPerAxis(Scene::Scene& scene, Scene::Object* object,
			Aberration::PsfIndex const& allAxisStarts, Aberration::PsfIndex const& allAxisCounts,
			std::array<float, 6> const& s, std::array<float, 6> const& p)
		{
			// Render parameters
			const glm::ivec2 maxResolution = Tiling::computeMaxRenderResolution(scene, object);
			const std::array<float, 2> fovyLimits = Tiling::computeFovyLimits(scene, object);

			// The number of layers needed for each axis
			SliceInfo result;
			result.m_s = s;
			result.m_p = p;
			result.m_numSlices = { 1, 1, 1, 1, 1, 1 }; // start from 1, which is for the last PSF
			result.m_numSlicesUnreduced = { 1, 1, 1, 1, 1, 1 };

			// Go through all three of the interpolated PSF axes
			for (size_t axisId = 0; axisId < 6; ++axisId)
			{
				// Construct the axis start IDs and axis counters
				Aberration::PsfIndex axisStarts;
				Aberration::PsfIndex axisCounts;
				for (size_t id = 0; id < 6; ++id)
				{
					axisStarts[id] = allAxisStarts[(axisId + id) % 6];
					axisCounts[id] = allAxisCounts[(axisId + id) % 6];
				}

				//Debug::log_debug() << "axis: " << axisId << Debug::end;

				// Find out the number of texture layers needed for the horizontal axis
				Aberration::PsfIndex loopIndices;
				for (loopIndices[0] = axisStarts[0]; loopIndices[0] < (axisStarts[0] + axisCounts[0] - 1); ++loopIndices[0]) // again, ignore the last PSF
				{
					//Debug::log_debug() << " - slice: " << loopIndices[0] << Debug::end;

					// Necessary information for the current slice
					SliceInfo::PerSliceInfo perSliceInfo;

					// Figure out how many layers are needed to transition to the next PSF
					for (loopIndices[1] = axisStarts[1]; loopIndices[1] < (axisStarts[1] + axisCounts[1]); ++loopIndices[1])
					for (loopIndices[2] = axisStarts[2]; loopIndices[2] < (axisStarts[2] + axisCounts[2]); ++loopIndices[2])
					for (loopIndices[3] = axisStarts[3]; loopIndices[3] < (axisStarts[3] + axisCounts[3]); ++loopIndices[3])
					for (loopIndices[4] = axisStarts[4]; loopIndices[4] < (axisStarts[4] + axisCounts[4]); ++loopIndices[4])
					for (loopIndices[5] = axisStarts[5]; loopIndices[5] < (axisStarts[5] + axisCounts[5]); ++loopIndices[5])
					{
						// Calculate the two neighboring PSF indices
						std::array<Aberration::PsfIndex, 2> psfIndices;
						for (size_t id = 0; id < 6; ++id)
						{
							psfIndices[0][(axisId + id) % 6] = loopIndices[id];
							psfIndices[1][(axisId + id) % 6] = loopIndices[id] + (id == 0 ? 1 : 0);
						}

						// Update the number of necessary layers needed from the max. blur radius diff.
						const size_t minRadius = Psfs::minBlurRadius(scene, object, maxResolution, fovyLimits, psfIndices);
						const size_t maxRadius = Psfs::maxBlurRadius(scene, object, maxResolution, fovyLimits, psfIndices);
						const size_t maxDiff = Psfs::maxBlurRadiusDifference(scene, object, maxResolution, fovyLimits, psfIndices);
						const size_t numLayersNeeded = calcNumReducedSlices(maxRadius, maxDiff, s[axisId], p[axisId]);
						const float numLayersReduction = calcNumSlicesReduction(maxRadius, maxDiff, s[axisId], p[axisId]);

						if (perSliceInfo.m_numSlices < numLayersNeeded || (perSliceInfo.m_numSlices == numLayersNeeded && perSliceInfo.m_blurRadii[1] < maxRadius))
						{
							result.m_selectedPsfIndex[axisId] = psfIndices[0];
							result.m_psfBurRadii[axisId] = { minRadius, maxRadius };
							result.m_numLayerReduction[axisId] = numLayersReduction;

							perSliceInfo.m_maxDiff = maxDiff;
							perSliceInfo.m_numLayerReduction = numLayersReduction;
							perSliceInfo.m_numSlices = numLayersNeeded;
							perSliceInfo.m_blurRadii[0] = minRadius;
							perSliceInfo.m_blurRadii[1] = maxRadius;
							perSliceInfo.m_psfIndex = psfIndices[0];
						}
					}

					// Increment the layer counter
					result.m_numSlices[axisId] += perSliceInfo.m_numSlices;
					result.m_numSlicesUnreduced[axisId] += perSliceInfo.m_maxDiff;

					// Store the per-slice info for each axis
					result.m_slices[axisId].push_back(perSliceInfo);
				}
			}

			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		SliceInfo slicesPerAxis(Scene::Scene& scene, Scene::Object* object,
			Aberration::PsfIndex const& allAxisStarts, Aberration::PsfIndex const& allAxisCounts,
			glm::vec3 s, glm::vec3 p)
		{
			// Extend them to all 4 axes
			std::array<float, 6> ss{ s[2], s[0], s[1], 1.0f, 1.0f, 1.0f };
			std::array<float, 6> ps{ p[2], p[0], p[1], 1.0f, 1.0f, 1.0f };

			return slicesPerAxis(scene, object, allAxisStarts, allAxisCounts, ss, ps);
		}

		////////////////////////////////////////////////////////////////////////////////
		SliceInfo slicesPerAxis(Scene::Scene& scene, Scene::Object* object,
			Aberration::PsfIndex const& allAxisStarts, Aberration::PsfIndex const& allAxisCounts)
		{
			return slicesPerAxis(scene, object, allAxisStarts, allAxisCounts,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP);
		}

		////////////////////////////////////////////////////////////////////////////////
		void textureDimensionsComparisonOffAxis(Scene::Scene& scene, Scene::Object* object)
		{
			// Various numbers needed later
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Find the minimum and maximum blur radius
			const glm::ivec2 minMaxRadius = Psfs::blurRadiusLimitsGlobal(scene, object);
			const size_t maxDiameter = minMaxRadius[1] * 2 + 1;

			Debug::log_debug() << std::string(80, '=') << Debug::end;
			Debug::log_debug() << "Comparing texture dimensions" << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;

			// Calculate the number of layers needed for each axis
			const SliceInfo sliceInfoBase = slicesPerAxis(scene, object,
				Aberration::PsfIndex{ 0, 0, 0, 0, 0, 0 },
				Aberration::PsfIndex{ numDefocuses, numHorizontal, numVertical, numChannels, numApertures, numFocuses },
				glm::vec3(0.0f), glm::vec3(0.0f));

			const SliceInfo sliceInfoReduced = slicesPerAxis(scene, object,
				Aberration::PsfIndex{ 0, 0, 0, 0, 0, 0 },
				Aberration::PsfIndex{ numDefocuses, numHorizontal, numVertical, numChannels, numApertures, numFocuses });

			Debug::log_debug() << std::string(80, '-') << Debug::end;

			// Return the final texture dimensions
			glm::ivec3 dimensionsBase = glm::ivec3(maxDiameter, maxDiameter, 1) * glm::ivec3(sliceInfoBase.m_numSlices[1],
				sliceInfoBase.m_numSlices[2], sliceInfoBase.m_numSlices[0]);
			glm::ivec3 dimensionsReduced = glm::ivec3(maxDiameter, maxDiameter, 1) * glm::ivec3(sliceInfoReduced.m_numSlices[1],
				sliceInfoReduced.m_numSlices[2], sliceInfoReduced.m_numSlices[0]);

			Debug::log_debug() << "Texture data comparison:" << Debug::end;
			Debug::log_debug() << "Max global blur radius: " << minMaxRadius[1] << Debug::end;
			Debug::log_debug() << " - Base: " << dimensionsBase << ", " << Units::bytesToString(PsfTexture::textureSize(scene, object, dimensionsBase)) << Debug::end;
			Debug::log_debug() << " - Reduced: " << dimensionsReduced << ", " << Units::bytesToString(PsfTexture::textureSize(scene, object, dimensionsReduced)) << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << "Layer number comparison:" << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << " - Base:" << Debug::end;
			sliceInfoBase.logStats();
			Debug::log_debug() << " - Reduced:" << Debug::end;
			sliceInfoReduced.logStats();
			Debug::log_debug() << std::string(80, '=') << Debug::end;
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec3 textureCacheSize(Scene::Scene& scene, Scene::Object* object, const size_t a, const size_t f)
		{
			// Various numbers needed later
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Find the minimum and maximum blur radius
			const glm::ivec2 minMaxRadius = Psfs::blurRadiusLimitsGlobal(scene, object);
			const size_t maxDiameter = minMaxRadius[1] * 2 + 1;

			// Calculate the number of layers needed for each axis
			const SliceInfo sliceInfo = slicesPerAxis(scene, object,
				Aberration::PsfIndex{ 0, 0, 0, 0, a, f },
				Aberration::PsfIndex{ numDefocuses, numHorizontal, numVertical, numChannels, 1, 1 });
			sliceInfo.logStats();

			// Return the final texture dimensions
			return glm::ivec3(sliceInfo.m_numSlices[1], sliceInfo.m_numSlices[2], sliceInfo.m_numSlices[0]) * glm::ivec3(maxDiameter, maxDiameter, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec3 textureDimensionsDiopterBasedOnAxis(Scene::Scene& scene, Scene::Object* object)
		{
			// Various numbers needed later
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Find the minimum and maximum blur radius
			const glm::ivec2 minMaxRadius = Psfs::blurRadiusLimitsGlobal(scene, object);
			const size_t maxDiameter = minMaxRadius[1] * 2 + 1;

			// Calculate the number of layers needed for each axis
			const SliceInfo sliceInfo = slicesPerAxis(scene, object,
				Aberration::PsfIndex{ 0, numHorizontal / 2, numVertical / 2, 0, 0, 0 },
				Aberration::PsfIndex{ numDefocuses, 1, 1, numChannels, numApertures, numFocuses });

			sliceInfo.logStats();
			Debug::log_debug() << "Max global blur radius: " << minMaxRadius[1] << Debug::end;

			// Return the final texture dimensions
			return glm::ivec3(1, 1, sliceInfo.m_numSlices[0]) * glm::ivec3(maxDiameter, maxDiameter, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec3 textureDimensionsDiopterBasedOffAxis(Scene::Scene& scene, Scene::Object* object)
		{
			// Various numbers needed later
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Find the minimum and maximum blur radius
			const glm::ivec2 minMaxRadius = Psfs::blurRadiusLimitsGlobal(scene, object);
			const size_t maxDiameter = minMaxRadius[1] * 2 + 1;

			// Calculate the number of layers needed for each axis
			const SliceInfo sliceInfo = slicesPerAxis(scene, object,
				Aberration::PsfIndex{ 0, 0, 0, 0, 0, 0 },
				Aberration::PsfIndex{ numDefocuses, numHorizontal, numVertical, numChannels, numApertures, numFocuses });
			sliceInfo.logStats();

			// Calculate the number of layers needed for each axis
			const glm::ivec3 maxNumSlices = glm::ivec3(sliceInfo.m_numSlices[1], sliceInfo.m_numSlices[2], sliceInfo.m_numSlices[0]);

			Debug::log_debug() << "Max global blur radius: " << minMaxRadius[1] << Debug::end;

			// Return the final texture dimensions
			return maxNumSlices * glm::ivec3(maxDiameter, maxDiameter, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec3 textureDimensionsRadiusBased(Scene::Scene& scene, Scene::Object* object)
		{
			// Various numbers needed later
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Find the minimum and maximum blur radius
			const glm::ivec2 minMaxRadius = Psfs::blurRadiusLimitsGlobal(scene, object);
			const size_t maxRadius = minMaxRadius[1];
			const size_t maxDiameter = maxRadius * 2 + 1;

			// Return the final texture dimensions
			return glm::ivec3(1, 1, numDefocuses * (maxRadius + 1)) * glm::ivec3(maxDiameter, maxDiameter, 1);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec3 textureDimensionsDiopterBased(Scene::Scene& scene, Scene::Object* object)
		{
			switch (object->component<TiledSplatBlurComponent>().m_psfAxisMethod)
			{
			case TiledSplatBlurComponent::PsfAxisMethod::OnAxis: return textureDimensionsDiopterBasedOnAxis(scene, object);
			case TiledSplatBlurComponent::PsfAxisMethod::OffAxis: return textureDimensionsDiopterBasedOffAxis(scene, object);
			}
			return glm::ivec3(0);
		}

		////////////////////////////////////////////////////////////////////////////////
		glm::ivec3 textureDimensions(Scene::Scene& scene, Scene::Object* object)
		{
			switch (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout)
			{
			case TiledSplatBlurComponent::RadiusBased:  return textureDimensionsRadiusBased(scene, object);
			case TiledSplatBlurComponent::DiopterBased: return textureDimensionsDiopterBased(scene, object);
			}
			return glm::ivec3(0);
		}

		////////////////////////////////////////////////////////////////////////////////
		void updateTexture(Scene::Scene& scene, Scene::Object* object)
		{
			DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, 1, DateTime::Seconds, "PSF Texture Update");

			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Generate the base texture
			const std::string baseName = textureName(scene, object);
			const glm::ivec3 baseSize = textureDimensions(scene, object);
			Scene::createTexture(scene, baseName, GL_TEXTURE_3D,
				baseSize.x, baseSize.y, baseSize.z,
				textureFormatEnum(scene, object), GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
				GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);

			// Clear the textures
			glm::vec3 clearData(0.0f);
			glClearTexImage(scene.m_textures[baseName].m_texture, 0, GL_RGB, GL_FLOAT, glm::value_ptr(clearData));

			// Log stats regarding the PSF texture
			Debug::log_debug() << std::string(80, '=') << Debug::end;
			Debug::log_debug() << "Base texture dimensions: " << baseSize << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << "Base texture data size: " << Units::bytesToString(PsfTexture::textureSize(scene, object, baseSize)) << Debug::end;
			Debug::log_debug() << std::string(80, '=') << Debug::end;

			// Create the cache textures
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis &&
				(numApertures > 1 || numFocuses > 1))
			{

				Debug::log_debug() << std::string(80, '=') << Debug::end;

				size_t totalBytes = 0;
				for (size_t a = 0; a < numApertures; ++a)
				for (size_t f = 0; f < numFocuses; ++f)
				{
					// Name and size
					const std::string cacheName = textureNameCache(scene, object, a, f);
					const glm::ivec3 cacheSize = textureCacheSize(scene, object, a, f);

					// Create the texture
					Scene::createTexture(scene, cacheName, GL_TEXTURE_3D,
						cacheSize.x, cacheSize.y, cacheSize.z,
						textureFormatEnum(scene, object), GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
						GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);

					// Clear the textures
					glm::vec3 clearData(0.0f);
					glClearTexImage(scene.m_textures[cacheName].m_texture, 0, GL_RGB, GL_FLOAT, glm::value_ptr(clearData));

					Debug::log_debug() << "Cache size for " << a << ", " << f << ": " << cacheSize << ", "
						<< Units::bytesToString(PsfTexture::textureSize(scene, object, cacheSize)) << Debug::end;
					totalBytes += PsfTexture::textureSize(scene, object, cacheSize);
				}

				Debug::log_debug() << std::string(80, '=') << Debug::end;
				Debug::log_debug() << "Total cache size: " << Units::bytesToString(totalBytes) << Debug::end;
				Debug::log_debug() << std::string(80, '=') << Debug::end;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace Buffers
	{
		////////////////////////////////////////////////////////////////////////////////
		std::vector<std::string> bufferNames(Scene::Scene& scene, Scene::Object* object)
		{
			return
			{
				// Common buffers
				"TiledSplatBlur_Fragments"s,
				"TiledSplatBlur_DispatchIndirect"s,
				"TiledSplatBlur_SplatIndex"s,
				"TiledSplatBlur_SortIndex"s,
				"TiledSplatBlur_TileParameters"s,

				// PSF buffers
				"TiledSplatBlur_PsfParams"s,
				"TiledSplatBlur_PsfWeights"s,
				"TiledSplatBlur_CachePsfParams"s,
				"TiledSplatBlur_PsfInterpolation"s,
				"TiledSplatBlur_InterpolatedPsfParams"s,
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		void updateBuffers(Scene::Scene& scene, Scene::Object* object)
		{
			DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, 1, DateTime::Seconds, "PSF Buffer Update");

			// Extract the number of fragments and tiles
			const int blockSize = Tiling::maxFragmentBlockSize(scene, object);
			const glm::ivec2 numFragments = Tiling::computePaddedResolution(scene, object);
			const glm::ivec2 numTiles = Tiling::computeNumTiles(scene, object, blockSize) * blockSize;

			// Number of psfs and other parameters
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();
			const size_t maxRadius = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxCoC;

			// Compute the total number of entries in the various buffers
			const size_t numEntriesFragmentData = numFragments.x * numFragments.y;
			const size_t numEntriesTileData = numTiles.x * numTiles.y;
			const size_t numEntriesTileParameters = numTiles.x * numTiles.y;
			const size_t numEntriesTileDispatch = numTiles.x * numTiles.y;
			const size_t numEntriesTileSplat = numTiles.x * numTiles.y;
			const size_t numEntriesTileSort = numTiles.x * numTiles.y;
			const size_t numEntriesPsfParam = Psfs::numTotalPsfs(scene, object);
			const size_t numEntriesCachePsfParam = numDefocuses * numHorizontal * numVertical * numChannels * numApertures * numFocuses;
			const size_t numEntriesPsfInterpolation = 1 << 10;
			const size_t numEntriesLerpedPsfParam = numDefocuses * numHorizontal * numVertical * numChannels;

			// Compute the maximum number of sub-entries one entry may hold in the each buffer
			const size_t numSubEntriesFragmentData = Tiling::fragmentBufferMaxFragmentsPerEntry(scene, object, GPU::numLayers());
			const size_t numSubEntriesTileDispatch = Tiling::dispatchBufferMaxDispatchPerEntry(scene, object, GPU::numLayers());
			const size_t numSubEntriesTileParameters = 1;
			const size_t numSubEntriesTileSplat = Tiling::tileBufferMaxCenterFragmentsPerEntry(scene, object, GPU::numLayers());
			const size_t numSubEntriesTileSort = Tiling::tileBufferMaxFragmentsPerEntry(scene, object, GPU::numLayers());
			const size_t numSubEntriesPsfParam = 1;
			const size_t numSubEntriesCachePsfParam = 1;
			const size_t numSubEntriesPsfInterpolation = 1;
			const size_t numSubEntriesLerpedPsfParam = 1;

			// Compute the data size for a single entry in each buffer
			const GLsizeiptr fragmentDataSize = 4 * sizeof(GLuint);
			const GLsizeiptr tileParametersDataSize = 4 * sizeof(GLuint);
			const GLsizeiptr tileDispatchIndirectSize = 4 * sizeof(GLuint);
			const GLsizeiptr tileSplatIndexSize = 8 * sizeof(GLuint);
			const GLsizeiptr tileSortIndexSize = 2 * sizeof(GLuint);
			const GLsizeiptr psfParamDataSize = sizeof(UniformDataPsfParam);
			const GLsizeiptr cachePsfParamDataSize = 8 * sizeof(GLuint);
			const GLsizeiptr psfInterpolationDataSize = 4 * sizeof(GLuint);
			const GLsizeiptr lerpedPsfParamDataSize = 8 * sizeof(GLuint);

			auto printEntries = [](std::string const& entryName, std::string const& bufferName, const size_t numEntries, const size_t numSubEntries, const size_t dataSize)
			{
				Debug::log_debug() << "Buffer properties (" << bufferName << "):" << Debug::end;
				Debug::log_debug() << " > number of entries: " << numEntries << Debug::end;
				Debug::log_debug() << " > number of sub-entries per " << entryName << ": " << numSubEntries << Debug::end;
				Debug::log_debug() << " > number of total entries: " << numEntries * numSubEntries << Debug::end;
				Debug::log_debug() << " > size of one sub-entry: " << Units::bytesToString(dataSize) << Debug::end;
				Debug::log_debug() << " > size of one whole entry: " << Units::bytesToString(numSubEntries * dataSize) << Debug::end;
				Debug::log_debug() << " > total size: " << Units::bytesToString(numEntries * numSubEntries * dataSize) << Debug::end;
				Debug::log_debug() << std::string(80, '-') << Debug::end;
			};

			Debug::log_debug() << std::string(80, '=') << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << "Number of fragments: " << numFragments << " (" << numEntriesFragmentData << ")" << Debug::end;
			Debug::log_debug() << "Number of tiles: " << numTiles << " (" << numEntriesTileData << ")" << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;

			printEntries("fragment", "fragment buffer", numEntriesFragmentData, numSubEntriesFragmentData, fragmentDataSize);
			printEntries("tile", "tile parameter buffer", numEntriesTileParameters, numSubEntriesTileParameters, tileParametersDataSize);
			printEntries("tile", "dispatch buffer", numEntriesTileDispatch, numSubEntriesTileDispatch, tileDispatchIndirectSize);
			printEntries("tile", "splat index buffer", numEntriesTileSplat, numSubEntriesTileSplat, tileSplatIndexSize);
			printEntries("tile", "sort index buffer", numEntriesTileSort, numSubEntriesTileSort, tileSortIndexSize);

			Debug::log_debug() << "Number of PSFs: ("
				<< numDefocuses << ", " << numHorizontal << ", " << numVertical << ", "
				<< numChannels << ", " << numApertures << ", " << numFocuses << ")" << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			printEntries("PSF", "PSF parameter buffer", numEntriesPsfParam, numSubEntriesPsfParam, psfParamDataSize);
			printEntries("PSF", "PSF cache param buffer", numEntriesCachePsfParam, numSubEntriesCachePsfParam, cachePsfParamDataSize);
			printEntries("PSF", "PSF interpolation buffer", numEntriesPsfInterpolation, numSubEntriesPsfInterpolation, psfInterpolationDataSize);
			printEntries("PSF", "lerped PSF param buffer", numEntriesLerpedPsfParam, numSubEntriesLerpedPsfParam, lerpedPsfParamDataSize);

			// Compute the new sizes of the GPU buffers
			std::unordered_map<std::string, GLsizeiptr> newBufferSizes =
			{
				// Compute the new sizes of the common buffers
				std::make_pair("TiledSplatBlur_Fragments"s, numEntriesFragmentData * numSubEntriesFragmentData * fragmentDataSize),
				std::make_pair("TiledSplatBlur_TileParameters"s, numEntriesTileParameters * numSubEntriesTileParameters * tileParametersDataSize),
				std::make_pair("TiledSplatBlur_DispatchIndirect"s, numEntriesTileDispatch * numSubEntriesTileDispatch * tileDispatchIndirectSize),
				std::make_pair("TiledSplatBlur_SplatIndex"s, numEntriesTileSplat * numSubEntriesTileSplat * tileSplatIndexSize),
				std::make_pair("TiledSplatBlur_SortIndex"s, numEntriesTileSort * numSubEntriesTileSort * tileSortIndexSize),

				// Compute the new sizes of the PSF-related buffers
				std::make_pair("TiledSplatBlur_PsfParams",  numEntriesPsfParam * numSubEntriesPsfParam * psfParamDataSize),
				std::make_pair("TiledSplatBlur_CachePsfParams", numEntriesCachePsfParam * numSubEntriesCachePsfParam * cachePsfParamDataSize),
				std::make_pair("TiledSplatBlur_PsfInterpolation", numEntriesPsfInterpolation * numSubEntriesPsfInterpolation * psfInterpolationDataSize),
				std::make_pair("TiledSplatBlur_InterpolatedPsfParams", numEntriesLerpedPsfParam * numSubEntriesLerpedPsfParam * lerpedPsfParamDataSize),
			};

			Debug::log_debug() << "Buffer sizes:" << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;

			// Actually resize the buffers
			size_t totalSize = 0;
			for (auto const& newBufferSize : newBufferSizes)
			{
				auto const& [bufferName, bufferSize] = newBufferSize;
				Scene::resizeGPUBuffer(scene, bufferName, bufferSize, true);
				Scene::bindBuffer(scene, bufferName);
				glm::uvec4 emptyBufferData = glm::uvec4(0);
				glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, glm::value_ptr(emptyBufferData));
				totalSize += bufferSize;

				Debug::log_debug() << bufferName << ": " << Units::bytesToString(bufferSize) << Debug::end;
			}

			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << "Total memory consumption of all the buffers: " << Units::bytesToString(totalSize) << "." << Debug::end;
			Debug::log_debug() << std::string(80, '=') << Debug::end;
			Debug::log_debug() << "Updating buffers finished." << Debug::end;
		}

		////////////////////////////////////////////////////////////////////////////////
		bool shouldUpdatePsf(Aberration::PsfIndex const& psfIndex, Aberration::PsfIndex const& startIndex, Aberration::PsfIndex const& numIndices)
		{
			bool result = true;
			for (size_t id = 0; id < 6; ++id)
				result &= (psfIndex[id] >= startIndex[id] && psfIndex[id] < (startIndex[id] + numIndices[id]));
			return result;
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadPsfData(Scene::Scene& scene, Scene::Object* object, const bool uploadParams, const bool uploadWeights,
			Aberration::PsfIndex const& startIndex, Aberration::PsfIndex const& numIndices)
		{
			// Various numbers needed later
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();

			// Extract the total number of PSFs and weights
			const size_t numTotalPsfs = Psfs::numTotalPsfs(scene, object);
			size_t numTotalWeights = 0;

			// Go through each PSF and upload the PSF parameters
			std::vector<UniformDataPsfParam> psfParamBuffer(numTotalPsfs); // Parameters of the individual PSFs
			{
				DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, numTotalPsfs, DateTime::Seconds, "PSF Params");

				for (size_t psfId = 0; psfId < numTotalPsfs; ++psfId)
				{
					// Extract the psf and calculate the needed properties
					const Aberration::PsfIndex psfIndex = Psfs::getPsfIndex(scene, object, psfId);

					// Extract the correponding derived PSF parameters
					TiledSplatBlurComponent::DerivedPsfParameters& derivedPsfParameters =
						object->component<TiledSplatBlurComponent>().m_derivedPsfParameters[psfId];

					// Store the relevant props in the output structure
					UniformDataPsfParam& psfParameters = psfParamBuffer[psfId];
					psfParameters.m_minBlurRadius = derivedPsfParameters.m_minBlurRadius;
					psfParameters.m_maxBlurRadius = derivedPsfParameters.m_maxBlurRadius;
					psfParameters.m_weightStartId = numTotalWeights;
					psfParameters.m_blurRadiusDeg = derivedPsfParameters.m_blurRadiusDeg;

					// Increment the weight start pointer
					if (shouldUpdatePsf(psfIndex, startIndex, numIndices))
						numTotalWeights += derivedPsfParameters.m_numPsfWeights;
				}
			}

			// Upload the generated data
			if (uploadParams)
				Scene::uploadBufferData(scene, "TiledSplatBlur_PsfParams", psfParamBuffer);

			// Skip the weight upload if not requested
			if (!uploadWeights)
				return;

			// Now actually upload the weights
			std::vector<GLfloat> psfWeightBuffer(numTotalWeights);
			{
				DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, numTotalPsfs, DateTime::Seconds, "PSF Weights");

				Threading::threadedExecuteIndices(Threading::numThreads(),
					[&](Threading::ThreadedExecuteEnvironment const& environment, size_t psfId)
				{
					// Extract the psf and calculate the needed properties
					const Aberration::PsfIndex psfIndex = Psfs::getPsfIndex(scene, object, psfId);
					Aberration::PsfStackElements::PsfEntry const& psfEntry = Psfs::selectEntry(scene, object, psfIndex);

					// Ignore PSFs that we shouldn't upload
					if (!shouldUpdatePsf(psfIndex, startIndex, numIndices)) return;

					// Extract the PSF properties
					UniformDataPsfParam const& psfParameters = psfParamBuffer[psfId];

					// Store the downscaled PSF in all the relevant radii
					GLfloat* writePtr = &psfWeightBuffer[psfParameters.m_weightStartId];
					for (size_t radius = psfParameters.m_minBlurRadius; radius <= psfParameters.m_maxBlurRadius; ++radius)
					{
						const size_t diameter = radius * 2 + 1;
						const size_t numWeights = diameter * diameter;
						Psfs::PsfGpu::Map(writePtr, diameter, diameter) = Psfs::computeGpuPsf(scene, object, psfEntry, radius);
						writePtr += numWeights;
					}
				},
				numTotalPsfs);
			}

			// Upload the generated data
			Scene::resizeGPUBuffer(scene, "TiledSplatBlur_PsfWeights", numTotalWeights * sizeof(GLfloat), true);
			Scene::uploadBufferData(scene, "TiledSplatBlur_PsfWeights", psfWeightBuffer);
		}

		////////////////////////////////////////////////////////////////////////////////
		void uploadPsfData(Scene::Scene& scene, Scene::Object* object)
		{
			// Various numbers needed later
			auto& aberration = Psfs::getAberration(scene, object);
			auto& psfStack = Psfs::getPsfStack(scene, object);
			const size_t numDefocuses = psfStack.m_psfs.size();
			const size_t numHorizontal = psfStack.m_psfs[0].size();
			const size_t numVertical = psfStack.m_psfs[0][0].size();
			const size_t numChannels = psfStack.m_psfs[0][0][0].size();
			const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
			const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();
			auto const& psfParameters = aberration.m_psfParameters.m_evaluatedParameters;

			// Generate on-axis PSF data
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis)
			{
				DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, 1, DateTime::Seconds, "PSF Data Upload");

				// Upload all the weights in one go
				uploadPsfData(scene, object, true, true,
					Aberration::PsfIndex{ 0, 0, 0, 0, 0, 0 },
					Aberration::PsfIndex{ numDefocuses, numHorizontal, numVertical, numChannels, numApertures, numFocuses });
			}

			// Generate texture parameters with off-axis angles
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
			{
				// Generate the shader parameters
				const Shaders::ShaderParameters shaderParameters = Shaders::ShaderParameters(scene, object);

				// Some necessary objects
				Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
				Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);
				const glm::ivec2 minMaxRadiusGlobal = Psfs::blurRadiusLimitsGlobal(scene, object);

				DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, numApertures * numFocuses, DateTime::Seconds, "PSF Data Upload");

				// Generate each slice
				for (size_t a = 0; a < numApertures; ++a)
				for (size_t f = 0; f < numFocuses; ++f)
				{
					// Upload the relevant PSF weights
					uploadPsfData(scene, object, true, true,
						Aberration::PsfIndex{ 0, 0, 0, 0, a, f },
						Aberration::PsfIndex{ numDefocuses, numHorizontal, numVertical, numChannels, 1, 1 });

					// Upload the necessary uniforms	
					TiledSplatBlur::UniformDataCommon blurDataCommon;
					blurDataCommon.m_renderResolution = Tiling::computeRenderResolution(scene, object);
					blurDataCommon.m_paddedResolution = Tiling::computePaddedResolution(scene, object);
					blurDataCommon.m_cameraFov = glm::degrees(Camera::getFieldOfView(renderSettings, camera));
					blurDataCommon.m_psfLayersS = glm::vec4(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS, 0.0f);
					blurDataCommon.m_psfLayersP = glm::vec4(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP, 0.0f);
					blurDataCommon.m_minBlurRadiusGlobal = minMaxRadiusGlobal[0];
					blurDataCommon.m_maxBlurRadiusGlobal = minMaxRadiusGlobal[1];
					blurDataCommon.m_numDefocuses = numDefocuses;
					blurDataCommon.m_numHorizontalAngles = numHorizontal;
					blurDataCommon.m_numVerticalAngles = numVertical;
					blurDataCommon.m_numChannels = numChannels;
					blurDataCommon.m_numApertures = numApertures;
					blurDataCommon.m_numFocuses = numFocuses;
					blurDataCommon.m_objectDistancesMin = psfParameters.m_objectDioptres.front();
					blurDataCommon.m_objectDistancesMax = psfParameters.m_objectDioptres.back();
					blurDataCommon.m_objectDistancesStep = psfParameters.m_objectDistancesRange.m_step;
					blurDataCommon.m_apertureMin = psfParameters.m_apertureDiameters.front();
					blurDataCommon.m_apertureMax = psfParameters.m_apertureDiameters.back();
					blurDataCommon.m_apertureStep = psfParameters.m_apertureDiametersRange.m_step;
					blurDataCommon.m_focusDistanceMin = psfParameters.m_focusDioptres.front();
					blurDataCommon.m_focusDistanceMax = psfParameters.m_focusDioptres.back();
					blurDataCommon.m_focusDistanceStep = psfParameters.m_focusDistancesRange.m_step;
					blurDataCommon.m_incidentAnglesHorMin = psfParameters.m_incidentAnglesHorizontal.front();
					blurDataCommon.m_incidentAnglesHorMax = psfParameters.m_incidentAnglesHorizontal.back();
					blurDataCommon.m_incidentAnglesHorStep = psfParameters.m_incidentAnglesHorizontalRange.m_step;
					blurDataCommon.m_incidentAnglesVertMin = psfParameters.m_incidentAnglesVertical.front();
					blurDataCommon.m_incidentAnglesVertMax = psfParameters.m_incidentAnglesVertical.back();
					blurDataCommon.m_incidentAnglesVertStep = psfParameters.m_incidentAnglesVerticalRange.m_step;
					uploadBufferData(scene, "TiledSplatBlurCommon", blurDataCommon);

					// Bind all the necessary buffers
					for (auto const& bufferName : Buffers::bufferNames(scene, object))
						bindBuffer(scene, bufferName);
					glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, scene.m_genericBuffers["TiledSplatBlur_DispatchIndirect"].m_buffer);

					// Generate dispatch command
					bindShader(scene, object, "psf_cache_command", shaderParameters);
					glUniform1ui(0, a);
					glUniform1ui(1, f);
					glDispatchCompute(1, 1, 1);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

					// Bind the cache texture as image
					if (numApertures > 1 || numFocuses > 1)
						glBindImageTexture(1, scene.m_textures[PsfTexture::textureNameCache(scene, object, a, f)].m_texture, 0,
							GL_FALSE, 0, GL_WRITE_ONLY, PsfTexture::imageFormatEnum(scene, object));

					// Bind the final PSF texture as image
					else
						glBindImageTexture(1, scene.m_textures[PsfTexture::textureName(scene, object)].m_texture, 0,
							GL_FALSE, 0, GL_WRITE_ONLY, PsfTexture::imageFormatEnum(scene, object));

					// Generate the cache
					bindShader(scene, object, "psf_cache_texture", shaderParameters);
					glUniform1ui(0, a);
					glUniform1ui(1, f);
					glDispatchComputeIndirect(0);
					glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
				}

				// Generate PSF parameters
				bindShader(scene, object, "psf_cache_params", shaderParameters);
				glDispatchCompute(1, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);				

				// Get rid of the temporary PSF weights buffer by resizing it
				Scene::resizeGPUBuffer(scene, "TiledSplatBlur_PsfWeights", sizeof(GLfloat), false);

				// Log the total raw PSF memory
				const size_t totalRawMemory = Psfs::numTotalWeights(scene, object) * sizeof(GLfloat);
				Debug::log_debug() << "Number of raw PSF data: " << Units::bytesToString(totalRawMemory) << Debug::end;

				// Also do a PSF texture size comparison
				PsfTexture::textureDimensionsComparisonOffAxis(scene, object);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace ResourceLoading
	{
		////////////////////////////////////////////////////////////////////////////////
		void initGPUBuffers(Scene::Scene& scene, Scene::Object* = nullptr)
		{
			Scene::createGPUBuffer(scene, "TiledSplatBlurCommon", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_1);
			Scene::createGPUBuffer(scene, "TiledSplatBlurPass", GL_UNIFORM_BUFFER, false, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_2);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_Fragments", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_3);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_TileParameters", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_4);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_DispatchIndirect", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_5);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_SplatIndex", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_6);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_SortIndex", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_7);

			Scene::createGPUBuffer(scene, "TiledSplatBlur_PsfParams", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_8, GL_DYNAMIC_STORAGE_BIT);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_PsfWeights", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_9, GL_DYNAMIC_STORAGE_BIT);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_PsfInterpolation", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_10);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_CachePsfParams", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_11);
			Scene::createGPUBuffer(scene, "TiledSplatBlur_InterpolatedPsfParams", GL_SHADER_STORAGE_BUFFER, true, true, GPU::UniformBufferIndices::UNIFORM_BUFFER_GENERIC_12);
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

			// Load shaders for this component
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
		void clearPsfCache(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "PSF Cache Clear", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Psfs::clearPsfCache(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void updateBuffers(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "Buffer Update", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Buffers::updateBuffers(scene, &object);
					PsfTexture::updateTexture(scene, &object);
				});
		}

		////////////////////////////////////////////////////////////////////////////////
		void updateDerivedParameters(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "PSF Data Upload", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Psfs::updateDerivedParameters(scene, &object);
				});
		};

		////////////////////////////////////////////////////////////////////////////////
		void uploadPsfData(Scene::Scene& scene, Scene::Object* object, int delayFrames = 1)
		{
			DelayedJobs::postJob(scene, object, "PSF Data Upload", false, delayFrames,
				[](Scene::Scene& scene, Scene::Object& object)
				{
					Buffers::uploadPsfData(scene, &object);
				});
		};

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
		// Store the initializers
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, ResourceLoading::loadShaders, "Shaders");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::NeuralNetwork, ResourceLoading::loadNeuralNetworks, "Neural Networks");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::GenericBuffer, ResourceLoading::initGPUBuffers, "Generic GPU Buffers");
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Custom, ResourceLoading::initAberrationPresets, "Aberration Presets");

		// Invoke the delayed computations
		Computations::initPsfStack(scene, &object);
		Computations::computePsfs(scene, &object);
		if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearAberrationCache)
			Computations::clearPsfCache(scene, &object);
		Computations::updateDerivedParameters(scene, &object);
		Computations::loadShaders(scene, &object);
		Computations::updateBuffers(scene, &object);
		Computations::uploadPsfData(scene, &object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Store the shader initializer
		Scene::removeResourceInitializer(scene, object.m_name, Scene::Shader);
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionLDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, 
		Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_inputDynamicRange == TiledSplatBlurComponent::LDR &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderObjectPreconditionHDROpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, 
		Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_inputDynamicRange == TiledSplatBlurComponent::HDR &&
			RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, 
		Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Aberration and PSF stack
		auto& aberration = Psfs::getAberration(scene, object);
		auto& psfStack = Psfs::getPsfStack(scene, object);
		const size_t numDefocuses = psfStack.m_psfs.size();
		const size_t numHorizontal = psfStack.m_psfs[0].size();
		const size_t numVertical = psfStack.m_psfs[0][0].size();
		const size_t numChannels = psfStack.m_psfs[0][0][0].size();
		const size_t numApertures = psfStack.m_psfs[0][0][0][0].size();
		const size_t numFocuses = psfStack.m_psfs[0][0][0][0][0].size();
		auto const& psfParameters = aberration.m_psfParameters.m_evaluatedParameters;

		// Compute the number of work groups
		const glm::ivec2 numTiles = Tiling::computeNumTiles(scene, object);

		// Resolution to render at
		const glm::ivec2 outputResolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		const glm::ivec2 renderResolution = Tiling::computeRenderResolution(scene, object);

		// Downscale, if needed
		if (renderResolution != outputResolution)
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Downscale");

			// Blit the fbo contents
			RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings,
				RenderSettings::GB_WriteBuffer, RenderSettings::GB_ReadBuffer, 0, GL_READ_FRAMEBUFFER);
			RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings,
				RenderSettings::GB_WriteBuffer, RenderSettings::GB_WriteBuffer, 0, GL_DRAW_FRAMEBUFFER);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, outputResolution.x, outputResolution.y, 0, 0, renderResolution.x, renderResolution.y, 
				GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			// Swap read buffers
			RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
		}

		// Compute the padded resolution (size of the area spanned by the spawned tiles)
		const glm::ivec2 paddedResolution = Tiling::computePaddedResolution(scene, object);

		// Merged block size
		const size_t mergedFragmentSize = Tiling::fragmentBlockSize(scene, object);

		// Find the minimum and maximum blur radius
		// TODO: consider the current aperture and focus settings to determine this
		const glm::ivec2 minMaxRadiusCurrent = Psfs::blurRadiusLimitsCurrent(scene, object);
		const glm::ivec2 minMaxRadiusGlobal = Psfs::blurRadiusLimitsGlobal(scene, object);

		// Maximum buffer sizes with the current configuration
		const size_t tileSize = object->component<TiledSplatBlurComponent>().m_tileSize;
		const size_t numLayers = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers;
		const size_t maxDiameter = size_t(minMaxRadiusCurrent[1]) * 2 + 1;
		const size_t fragmentBufferMaxSubentries = Tiling::fragmentBufferMaxFragmentsPerEntry(scene, object, numLayers);
		const size_t tileBufferMaxCenterSubentries = Tiling::tileBufferMaxCenterFragmentsPerEntry(numLayers, tileSize);
		const size_t tileBufferMaxSubentries = Tiling::tileBufferMaxFragmentsPerEntry(numLayers, tileSize, minMaxRadiusCurrent[1]);

		// Maximum sort values
		const size_t maxSharedIndices = Tiling::tileBufferMaxSortSharedElements(scene, object, tileBufferMaxSubentries);
		const size_t maxSortElements = Tiling::tileBufferMaxSortElements(tileBufferMaxSubentries);
		const size_t numSortIterations = Tiling::tileBufferMaxSortIterations(tileBufferMaxSubentries, maxSharedIndices);
		const size_t dispatchBufferMaxSubentries = Tiling::dispatchBufferMaxDispatchPerEntry(tileBufferMaxSubentries, maxSharedIndices);

		// Distance between consecutive dispatch commands
		const size_t dispatchBufferElementStride = 4 * sizeof(GLuint);

		// Generate the shader parameters
		const Shaders::ShaderParameters shaderParameters = Shaders::ShaderParameters(scene, object);

		// Bind the GBuffer textures
		RenderSettings::bindGbufferTextureLayersOpenGL(scene, simulationSettings, renderSettings);

		// Bind the output color image
		RenderSettings::bindGbufferLayerImageOpenGL(scene, simulationSettings, renderSettings);

		// Emit the shader parameters
		if (Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS)->component<DebugSettings::DebugSettingsComponent>().m_profileGpu)
		{
			Profiler::ScopedCategory category(scene, "Shader Parameters");

			emitProfilerShaderParameters(scene, shaderParameters);
		}

		// Upload the necessary uniforms
		TiledSplatBlur::UniformDataCommon blurDataCommon;
		{
			Profiler::ScopedGpuPerfCounter category(scene, "Uniforms");

			blurDataCommon.m_numTiles = numTiles;
			blurDataCommon.m_renderResolution = renderResolution;
			blurDataCommon.m_paddedResolution = paddedResolution;
			blurDataCommon.m_cameraFov = glm::degrees(Camera::getFieldOfView(renderSettings, camera));
			blurDataCommon.m_fragmentBufferSubentries = fragmentBufferMaxSubentries;
			blurDataCommon.m_tileBufferCenterSubentries = tileBufferMaxCenterSubentries;
			blurDataCommon.m_tileBufferTotalSubentries = tileBufferMaxSubentries;
			blurDataCommon.m_numSortIterations = numSortIterations;

			blurDataCommon.m_psfAxisMethod = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod;
			blurDataCommon.m_psfTextureFormat = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat;
			blurDataCommon.m_psfTextureDepthLayout = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout;
			blurDataCommon.m_psfTextureAngleLayout = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureAngleLayout;
			blurDataCommon.m_weightScaleMethod = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_weightScaleMethod;
			blurDataCommon.m_weightRescaleMethod = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_weightRescaleMethod;
			blurDataCommon.m_outputMode = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_outputMode;
			blurDataCommon.m_overlayMode = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_overlayMode;
			blurDataCommon.m_accumulationMethod = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_accumulationMethod;

			blurDataCommon.m_psfLayersS = glm::vec4(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS, 0.0f);
			blurDataCommon.m_psfLayersP = glm::vec4(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP, 0.0f);

			blurDataCommon.m_numMergeSteps = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_fragmentMergeSteps;
			blurDataCommon.m_mergedFragmentSize = mergedFragmentSize;

			blurDataCommon.m_sortOffsetConstant = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortDepthOffset;
			blurDataCommon.m_sortOffsetScale = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortDepthScale;

			blurDataCommon.m_renderChannels = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_numWavelengths;
			blurDataCommon.m_renderLayers = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_layers.m_numLayers;
			blurDataCommon.m_depthOffset = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_depthOffset;
			blurDataCommon.m_alphaThreshold = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_alphaThreshold;
			blurDataCommon.m_normalizeResult = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_normalizeResult ? 1.0f : 0.0f;

			blurDataCommon.m_minBlurRadiusCurrent = minMaxRadiusCurrent[0];
			blurDataCommon.m_maxBlurRadiusCurrent = minMaxRadiusCurrent[1];
			blurDataCommon.m_minBlurRadiusGlobal = minMaxRadiusGlobal[0];
			blurDataCommon.m_maxBlurRadiusGlobal = minMaxRadiusGlobal[1];
			blurDataCommon.m_numDefocuses = numDefocuses;
			blurDataCommon.m_numHorizontalAngles = numHorizontal;
			blurDataCommon.m_numVerticalAngles = numVertical;
			blurDataCommon.m_numChannels = numChannels;
			blurDataCommon.m_numApertures = numApertures;
			blurDataCommon.m_numFocuses = numFocuses;
			blurDataCommon.m_objectDistancesMin = psfParameters.m_objectDioptres.front();
			blurDataCommon.m_objectDistancesMax = psfParameters.m_objectDioptres.back();
			blurDataCommon.m_objectDistancesStep = psfParameters.m_objectDistancesRange.m_step;
			blurDataCommon.m_apertureMin = psfParameters.m_apertureDiameters.front();
			blurDataCommon.m_apertureMax = psfParameters.m_apertureDiameters.back();
			blurDataCommon.m_apertureStep = psfParameters.m_apertureDiametersRange.m_step;
			blurDataCommon.m_focusDistanceMin = psfParameters.m_focusDioptres.front();
			blurDataCommon.m_focusDistanceMax = psfParameters.m_focusDioptres.back();
			blurDataCommon.m_focusDistanceStep = psfParameters.m_focusDistancesRange.m_step;
			blurDataCommon.m_incidentAnglesHorMin = psfParameters.m_incidentAnglesHorizontal.front();
			blurDataCommon.m_incidentAnglesHorMax = psfParameters.m_incidentAnglesHorizontal.back();
			blurDataCommon.m_incidentAnglesHorStep = psfParameters.m_incidentAnglesHorizontalRange.m_step;
			blurDataCommon.m_incidentAnglesVertMin = psfParameters.m_incidentAnglesVertical.front();
			blurDataCommon.m_incidentAnglesVertMax = psfParameters.m_incidentAnglesVertical.back();
			blurDataCommon.m_incidentAnglesVertStep = psfParameters.m_incidentAnglesVerticalRange.m_step;

			uploadBufferData(scene, "TiledSplatBlurCommon", blurDataCommon);
		}

		// Bind the SSBOs
		for (auto const& bufferName : Buffers::bufferNames(scene, object))
		{
			Debug::log_trace() << "Binding buffer: " << bufferName << Debug::end;

			bindBuffer(scene, bufferName);
		}

		// Whether the PSF texture should be regenerated or not
		bool regenerateTexture = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis ||
			blurDataCommon.m_numApertures > 1 || blurDataCommon.m_numFocuses > 1;

		// Interpolate the PSF textures
		if (regenerateTexture)
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "PSF Interpolation");

			// Bind the PSF cache textures
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
			{
				// Extract the current camera parameters
				const float apertureDiameter = camera->component<Camera::CameraComponent>().m_fixedAperture;
				const float focusDistance = 1.0f / camera->component<Camera::CameraComponent>().m_focusDistance;

				// Compute the corresponding PSF indices
				const float apertureId = blurDataCommon.m_numApertures <= 1 ? 0.0f : (apertureDiameter - blurDataCommon.m_apertureMin) / blurDataCommon.m_apertureStep;
				const float focusId = blurDataCommon.m_numFocuses <= 1 ? 0.0f : (focusDistance - blurDataCommon.m_focusDistanceMin) / blurDataCommon.m_focusDistanceStep;

				// Cache texture indices
				const std::array<size_t, 2> apertureIds =
				{
					size_t(glm::clamp(glm::floor(apertureId), 0.0f, float(blurDataCommon.m_numApertures - 1))),
					size_t(glm::clamp(glm::ceil(apertureId), 0.0f, float(blurDataCommon.m_numApertures - 1))),
				};
				const std::array<size_t, 2> focusIds =
				{
					size_t(glm::clamp(glm::floor(focusId), 0.0f, float(blurDataCommon.m_numFocuses - 1))),
					size_t(glm::clamp(glm::ceil(focusId), 0.0f, float(blurDataCommon.m_numFocuses - 1))),
				};

				// Bind the textures
				Scene::bindTexture(scene, PsfTexture::textureNameCache(scene, object, apertureIds[0], focusIds[0]), GPU::TextureIndices::TEXTURE_POST_PROCESS_2);
				Scene::bindTexture(scene, PsfTexture::textureNameCache(scene, object, apertureIds[1], focusIds[0]), GPU::TextureIndices::TEXTURE_POST_PROCESS_3);
				Scene::bindTexture(scene, PsfTexture::textureNameCache(scene, object, apertureIds[0], focusIds[1]), GPU::TextureIndices::TEXTURE_POST_PROCESS_4);
				Scene::bindTexture(scene, PsfTexture::textureNameCache(scene, object, apertureIds[1], focusIds[1]), GPU::TextureIndices::TEXTURE_POST_PROCESS_5);
			}

			if (object->component<TiledSplatBlurComponent>().m_clearPsfTexture)
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Clear");

				// Clear the PSF weight textures
				glm::vec3 clearData(0.0f);
				glClearTexImage(scene.m_textures[PsfTexture::textureName(scene, object)].m_texture, 0, GL_RGB, GL_FLOAT, glm::value_ptr(clearData));
			}

			// Clear the dispatch command buffer
			glm::uvec4 emptyDispatch = glm::uvec4(0);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene.m_genericBuffers["TiledSplatBlur_DispatchIndirect"].m_buffer);
			glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, glm::value_ptr(emptyDispatch));

			// Bind the dispatch command buffer
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, scene.m_genericBuffers["TiledSplatBlur_DispatchIndirect"].m_buffer);

			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Parameters Command");

				// Bind the corresponding shader
				bindShader(scene, object, "psf_texture_command_params", shaderParameters);

				// Dispatch the compute shader
				glDispatchCompute(1, 1, 1);

				// Place a memory barrier for the SSBOs
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
			}

			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Parameters");

				// Bind the corresponding shader
				bindShader(scene, object, "psf_texture_params", shaderParameters);

				// Dispatch the compute shader
				glDispatchComputeIndirect(0);

				// Place a memory barrier for the SSBOs
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}

			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Texture Command");

				// Bind the corresponding shader
				bindShader(scene, object, "psf_texture_command_texture", shaderParameters);

				// Dispatch the compute shader
				glDispatchCompute(1, 1, 1);

				// Place a memory barrier for the SSBOs
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
			}

			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Texture");

				// Bind the corresponding shader
				bindShader(scene, object, "psf_texture_texture", shaderParameters);

				// Bind the PSF textures as images
				glBindImageTexture(1, scene.m_textures[PsfTexture::textureName(scene, object)].m_texture, 0,
					GL_FALSE, 0, GL_WRITE_ONLY, PsfTexture::imageFormatEnum(scene, object));

				// Dispatch the compute shader
				glDispatchComputeIndirect(0);

				// Place a memory barrier for the SSBOs
				glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
			}
		}

		// Bind the PSF weight texture
		Scene::bindTexture(scene, PsfTexture::textureName(scene, object));

		// Build the per-fragment buffer
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Fragment Buffer");

			// Build the per-fragment buffer
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Build");

				// Bind the corresponding shader
				bindShader(scene, object, "fragment_buffer_build", shaderParameters);

				// Dispatch the compute shader
				glDispatchCompute(numTiles.x, numTiles.y, 1);

				// Place a memory barrier for the SSBOs
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}

			// Merge neighbouring fragments in the per-fragment buffer
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Merge");

				// Bind the corresponding shader
				bindShader(scene, object, "fragment_buffer_merge", shaderParameters);

				// Perform the specified number of merge steps
				for (size_t i = 0; i < object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_fragmentMergeSteps; ++i)
				{
					Profiler::ScopedGpuPerfCounter perfCounter(scene, "Fragment Merge #" + std::to_string(i + 1));

					// Compute the number of dispatch groups needed
					const glm::ivec2 numDispatchGroups = Tiling::computeNumGroups(renderResolution,
						object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_merge,
						object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i].m_blockSize);

					// Extract the merge parameters
					auto const& mergePreset = object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i];

					// Upload the merge parameters
					TiledSplatBlur::UniformDataFragmentMerge blurDataMerge;
					blurDataMerge.m_blockSize = mergePreset.m_blockSize;
					blurDataMerge.m_colorSimilarityThreshold = mergePreset.m_colorSimilarityThreshold;
					blurDataMerge.m_colorContrastThreshold = mergePreset.m_colorContrastThreshold;
					blurDataMerge.m_depthSimilarityThreshold = mergePreset.m_depthSimilarityThreshold;
					blurDataMerge.m_minBlurRadiusThreshold = mergePreset.m_minBlurRadiusThreshold;

					uploadBufferData(scene, "TiledSplatBlurPass", blurDataMerge);

					// Dispatch the compute shader
					glDispatchCompute(numDispatchGroups.x, numDispatchGroups.y, 1);

					// Place a memory barrier for the SSBOs
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}
			}
		}

		// Build the per-tile buffers
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Tile Buffer");

			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Build");

				// Bind the corresponding shader
				bindShader(scene, object, "tile_buffer_build", shaderParameters);

				// Dispatch the compute shader
				glDispatchCompute(numTiles.x, numTiles.y, 1);

				// Place a memory barrier for the SSBOs
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}

			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Splat");

				// Generate the splat command
				{
					Profiler::ScopedGpuPerfCounter perfCounter(scene, "Command Generation");

					// Bind the corresponding shader
					bindShader(scene, object, "tile_buffer_splat_command", shaderParameters);

					// Dispatch the compute shader
					glDispatchCompute(1, 1, 1);

					// Place a memory barrier for the SSBOs
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
				}

				// Splat the tile fragments
				{
					Profiler::ScopedGpuPerfCounter perfCounter(scene, "Fragment Splatting");

					// Bind the corresponding shader
					bindShader(scene, object, "tile_buffer_splat", shaderParameters);

					// Dispatch the compute shader
					glDispatchComputeIndirect(0);

					// Place a memory barrier for the SSBOs
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}
			}

			// Sort the per-tile buffers
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortTileBuffer)
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Sort");

				// Clear the dispatch command buffer again
				glm::uvec4 emptyDispatch = glm::uvec4(0);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene.m_genericBuffers["TiledSplatBlur_DispatchIndirect"].m_buffer);
				glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, glm::value_ptr(emptyDispatch));
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

				// Bind the dispatch command buffer
				glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, scene.m_genericBuffers["TiledSplatBlur_DispatchIndirect"].m_buffer);

				// Generate sort parameters
				{
					Profiler::ScopedGpuPerfCounter perfCounter(scene, "Command Generation");

					// Bind the corresponding shader
					bindShader(scene, object, "tile_buffer_sort_params", shaderParameters);

					// Dispatch the compute shader
					glDispatchCompute(numTiles.x, numTiles.y, 1);

					// Place a memory barrier for the SSBOs
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
				}

				// Do the pre-sort
				{
					Profiler::ScopedGpuPerfCounter perfCounter(scene, "Pre-Sort");

					// Bind the corresponding shader
					bindShader(scene, object, "tile_buffer_sort_presort", shaderParameters);

					// Dispatch the compute shader
					glDispatchComputeIndirect(0);

					// Place a memory barrier for the SSBOs
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}

				// Go through each group size
				// TODO: something is wrong with sorting
				for (size_t k = maxSharedIndices * 2; k <= maxSortElements; k *= 2)
				{
					Profiler::ScopedGpuPerfCounter perfCounter(scene, "Group Size [" + std::to_string(k) + "]");

					// Group id for the current group size
					const size_t groupId = Tiling::sortGroupId(k, maxSharedIndices);

					// Element index into the dispatch buffer
					size_t dispatchElementIndex = Tiling::dispatchElementIndex(k, maxSharedIndices);

					// Do the outer sort
					{
						Profiler::ScopedGpuPerfCounter perfCounter(scene, "Outer Sort");

						// Bind the corresponding shader
						bindShader(scene, object, "tile_buffer_sort_outer", shaderParameters);

						for (size_t j = k / 2; j >= maxSharedIndices; j /= 2)
						{
							// Upload the group parameters
							TiledSplatBlur::UniformDataSort blurDataSort;
							blurDataSort.m_groupId = groupId;
							blurDataSort.m_groupSize = k;
							blurDataSort.m_compareDistance = j;
							uploadBufferData(scene, "TiledSplatBlurPass", blurDataSort);

							// Dispatch the compute shader
							glDispatchComputeIndirect(dispatchElementIndex * dispatchBufferElementStride);

							// Increment the element index
							++dispatchElementIndex;

							// Place a memory barrier for the SSBOs
							glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
						}
					}

					// Do the inner sort
					{
						Profiler::ScopedGpuPerfCounter perfCounter(scene, "Inner Sort");

						// Bind the corresponding shader
						bindShader(scene, object, "tile_buffer_sort_inner", shaderParameters);

						// Upload the group parameters
						TiledSplatBlur::UniformDataSort blurDataSort;
						blurDataSort.m_groupId = groupId;
						blurDataSort.m_groupSize = k;
						uploadBufferData(scene, "TiledSplatBlurPass", blurDataSort);

						// Dispatch the compute shader
						glDispatchComputeIndirect(dispatchElementIndex * dispatchBufferElementStride);

						// Place a memory barrier for the SSBOs
						glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					}
				}
			}
		}

		// Perform the convolution
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Convolution");

			// Bind the corresponding shader
			bindShader(scene, object, "convolution", shaderParameters);

			// Number of dispatch groups
			glm::ivec2 numDispatchGroups = Tiling::computeNumGroups(renderResolution,
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_convolution);

			// Dispatch the compute shader
			glDispatchCompute(numDispatchGroups.x, numDispatchGroups.y, 1);

			// Place a memory barrier for the output texture
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
		}

		// Swap read buffers
		RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);

		// Upscale, if needed
		if (renderResolution != outputResolution)
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Upscale");

			// Blit the fbo contents
			RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings,
				RenderSettings::GB_WriteBuffer, RenderSettings::GB_ReadBuffer, 0, GL_READ_FRAMEBUFFER);
			RenderSettings::bindGbufferLayerOpenGL(scene, simulationSettings, renderSettings,
				RenderSettings::GB_WriteBuffer, RenderSettings::GB_WriteBuffer, 0, GL_DRAW_FRAMEBUFFER);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, renderResolution.x, renderResolution.y, 0, 0, outputResolution.x, outputResolution.y, 
				GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			// Swap read buffers
			RenderSettings::swapGbufferBuffers(scene, simulationSettings, renderSettings);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

		bool aberrationChanged = false;
		bool shaderChanged = false;
		bool psfSettingsChanged = false;
		bool renderSettingsChanged = false;

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

		// Groups settings
		if (ImGui::BeginTabItem("Tiling", activeTab.c_str()))
		{
			ImGui::SliderInt("Tile Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize, 8, 32); renderSettingsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SliderInt("Interpolation Group Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_interpolation, 1, 32); shaderChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SliderInt("Merge Group Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_merge, 1, 32); shaderChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SliderInt("Splat Group Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_splat, 1, 32); shaderChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SliderInt("Sort Group Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_sort, 1, 1024); shaderChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SliderInt("Convolution Group Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_convolution, 1, 32); shaderChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SliderInt("Max Sort Elements", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxSortElements, 1, 8192); shaderChanged |= ImGui::IsItemDeactivatedAfterEdit();

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// Fragment merge parameters
		if (ImGui::BeginTabItem("Fragment Merge", activeTab.c_str()))
		{
			shaderChanged |= ImGui::SliderInt("Merge Steps", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_fragmentMergeSteps, 0, 4);
			if (ImGui::BeginTabBar("Merge Steps"))
			{
				for (int i = 0; i < object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets.size(); ++i)
				{
					std::string name = "Merge Step " + std::to_string(i + 1);
					if (ImGui::BeginTabItem(name.c_str()))
					{
						renderSettingsChanged |= ImGui::SliderInt("Block Size", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i].m_blockSize, 0, 8);
						ImGui::SliderFloat("Color Similarity Threshold", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i].m_colorSimilarityThreshold, 0, 0.1f);
						ImGui::SliderFloat("Color Contrast Threshold", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i].m_colorContrastThreshold, 0, 0.1f);
						ImGui::SliderFloat("Depth Similarity Threshold", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i].m_depthSimilarityThreshold, 0, 4.0f);
						ImGui::SliderFloat("Min Blur Radius Threshold", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets[i].m_minBlurRadiusThreshold, 0, 8.0f);
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			}

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// PSF parameters
		if (ImGui::BeginTabItem("PSF", activeTab.c_str()))
		{
			psfSettingsChanged |= ImGui::Combo("PSF Texture Format", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat, TiledSplatBlur::TiledSplatBlurComponent::PsfTextureFormat_meta);
			psfSettingsChanged |= ImGui::Combo("PSF Texture Depth Layout", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout, TiledSplatBlur::TiledSplatBlurComponent::PsfTextureDepthLayout_meta);
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout == TiledSplatBlur::TiledSplatBlurComponent::PsfTextureDepthLayout::DiopterBased)
			{
				psfSettingsChanged |= ImGui::Combo("PSF Texture Angle Layout", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureAngleLayout, TiledSplatBlur::TiledSplatBlurComponent::PsfTextureAngleLayout_meta);
				psfSettingsChanged |= ImGui::Combo("Axis Method", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod, TiledSplatBlur::TiledSplatBlurComponent::PsfAxisMethod_meta);
				ImGui::Separator();
				ImGui::SliderFloat3("PSF Layers - s", glm::value_ptr(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS), 0.0f, 1.0f); psfSettingsChanged |= ImGui::IsItemDeactivatedAfterEdit();
				ImGui::SliderFloat3("PSF Layers - p", glm::value_ptr(object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP), 0.0f, 1.0f); psfSettingsChanged |= ImGui::IsItemDeactivatedAfterEdit();
				ImGui::Separator();
			}
			else if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout == TiledSplatBlur::TiledSplatBlurComponent::PsfTextureDepthLayout::RadiusBased)
			{
				object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod = TiledSplatBlur::TiledSplatBlurComponent::PsfAxisMethod::OnAxis;
			}
			ImGui::Checkbox("Clear PSF Texture", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearPsfTexture);
			ImGui::SliderInt("Max CoC", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxCoC, 8, 128); psfSettingsChanged |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::Combo("Weight Scale Method", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_weightScaleMethod, TiledSplatBlur::TiledSplatBlurComponent::WeightScaleMethod_meta);
			ImGui::Combo("Weight Rescale Method", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_weightRescaleMethod, TiledSplatBlur::TiledSplatBlurComponent::WeightRescaleMethod_meta);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// Convolution parameters
		if (ImGui::BeginTabItem("Convolution", activeTab.c_str()))
		{
			renderSettingsChanged |= ImGui::Combo("Render Resolution", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_renderResolutionId, renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolutionNames);
			ImGui::Combo("Input Dynamic Range", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_inputDynamicRange, TiledSplatBlur::TiledSplatBlurComponent::InputDynamicRange_meta);
			shaderChanged |= ImGui::Combo("Shader Type", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_shaderType, TiledSplatBlur::TiledSplatBlurComponent::ShaderType_meta);
			shaderChanged |= ImGui::Combo("Output Mode", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_outputMode, TiledSplatBlur::TiledSplatBlurComponent::OutputMode_meta);
			shaderChanged |= ImGui::Combo("Overlay Mode", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_overlayMode, TiledSplatBlur::TiledSplatBlurComponent::OverlayMode_meta);
			ImGui::Combo("Accumulation Method", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_accumulationMethod, TiledSplatBlur::TiledSplatBlurComponent::AccumulationMethod_meta);
			shaderChanged |= ImGui::SliderInt("Channels", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_numWavelengths, 1, Psfs::getAberration(scene, object).m_psfParameters.m_lambdas.size());
			ImGui::SliderFloat("Alpha Threshold", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_alphaThreshold, 0.0f, 1.0f);
			ImGui::DragFloat("Depth Offset", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_depthOffset, 0.001f);
			ImGui::DragFloat("Sort Depth Offset", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortDepthOffset, 0.0001f, -0.1f, 1.0f, "%.6f");
			ImGui::DragFloat("Sort Depth Scale", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortDepthScale, 0.0001f, -0.1f, 1.0f, "%.6f");

			ImGui::Checkbox("Sort Tile Buffer", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortTileBuffer);
			ImGui::SameLine();
			ImGui::Checkbox("Normalize Result", &object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_normalizeResult);

			EditorSettings::editorProperty<std::string>(scene, object, "MainTabBar_SelectedTab") = ImGui::CurrentTabItemName();
			ImGui::EndTabItem();
		}

		// End the tab bar
		ImGui::EndTabBar();

		// Updates
		if (aberrationChanged)
		{
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearAberrationCache)
				Computations::initPsfStack(scene, object);
			Computations::computePsfs(scene, object);
			if (object->component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearAberrationCache)
				Computations::clearPsfCache(scene, object);
		}

		if (aberrationChanged || psfSettingsChanged)
		{
			Computations::updateDerivedParameters(scene, object);
		}

		if (aberrationChanged || psfSettingsChanged || renderSettingsChanged || shaderChanged)
		{
			Computations::loadShaders(scene, object);
		}

		if (aberrationChanged || psfSettingsChanged || renderSettingsChanged)
		{
			Computations::updateBuffers(scene, object);
		}

		if (aberrationChanged || psfSettingsChanged)
		{
			Computations::uploadPsfData(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{

		// @CONSOLE_VAR(TiledSplatBlur, Tile Size, -tsb_tile_size, 8, 16, 32)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"tsb_tile_size", "TiledSplatBlur",
			"Tile size to use.",
			"N", { "8" }, {},
			Config::attribRegexInt()
		});

		// @CONSOLE_VAR(TiledSplatBlur, Axis Method, -tsb_axis_method, OnAxis, OffAxis)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"tsb_axis_method", "TiledSplatBlur",
			"How to handle the PSF axis method.",
			"", { "OnAxis" },
			{
				{ "OnAxis", "Use only the on-axis PSFs (isoplanatic)" },
				{ "OffAxis", "Use the off-axis PSFs too (anisoplanatic)" }
			},
			Config::attribRegexString()
		});

		// @CONSOLE_VAR(TiledSplatBlur, Focus Distance, -tsb_focus_distance, Fixed, Variable)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"tsb_focus_distance", "TiledSplatBlur",
			"How to handle the PSF focus distance.",
			"", { "Fixed" },
			{
				{ "Fixed", "Use only a fixed focus distance" },
				{ "Variable", "Use a variable focus distance" }
			},
			Config::attribRegexString()
		});

		// @CONSOLE_VAR(TiledSplatBlur, Aperture Diameter, -tsb_aperture_diameter, Fixed, Variable)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"tsb_aperture_diameter", "TiledSplatBlur",
			"How to handle the PSF aperture diameter.",
			"", { "Fixed" },
			{
				{ "Fixed", "Use only a fixed focus distance" },
				{ "Variable", "Use a variable focus distance" }
			},
			Config::attribRegexString()
		});
	};

	////////////////////////////////////////////////////////////////////////////////
	void demoSetup(Scene::Scene& scene)
	{
		// @CONSOLE_VAR(Scene, Object Groups, -object_group, Aberration_TiledSplat)
		SimulationSettings::createGroup(scene, "Aberration", "Aberration_TiledSplat");

		Scene::Object* renderSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);
		Scene::Object* camera = RenderSettings::getMainCamera(scene, renderSettings);

		// Add the tiled splat blur object.
		auto& tiledSplatBlur = createObject(scene, Scene::OBJECT_TYPE_TILED_SPLAT_BLUR, Scene::extendDefaultObjectInitializerBefore([&](Scene::Scene& scene, Scene::Object& object)
		{
			object.m_enabled = true;
			object.m_groups = SimulationSettings::makeGroupFlags(scene, "Aberration_TiledSplat");

			// Fragment merge
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_fragmentMergeSteps = 1;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_mergePresets =
			{
				/* blk., simil., contr., defoc., blur */
				{    2,   0.10f,  0.10f,  0.01f, 5.00f  },
				{    4,   0.02f,  0.02f,  0.01f, 5.00f  },
			};

			// Convolution
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_renderResolutionId = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_inputDynamicRange = TiledSplatBlur::TiledSplatBlurComponent::LDR;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_outputMode = TiledSplatBlur::TiledSplatBlurComponent::Convolution;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_overlayMode = TiledSplatBlur::TiledSplatBlurComponent::None;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_accumulationMethod = TiledSplatBlur::TiledSplatBlurComponent::FrontToBack;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureFormat = TiledSplatBlur::TiledSplatBlurComponent::F11;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureDepthLayout = TiledSplatBlur::TiledSplatBlurComponent::DiopterBased;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfTextureAngleLayout = TiledSplatBlur::TiledSplatBlurComponent::PsfBased;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_weightScaleMethod = TiledSplatBlur::TiledSplatBlurComponent::AreaSquare;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_weightRescaleMethod = TiledSplatBlur::TiledSplatBlurComponent::AlphaBlend;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_coeffLerpMethod = TiledSplatBlur::TiledSplatBlurComponent::Lerp;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod = Config::AttribValue("tsb_axis_method").get_enum<TiledSplatBlurComponent::PsfAxisMethod_meta_class>();
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_shaderType = TiledSplatBlur::TiledSplatBlurComponent::Auto;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize = Config::AttribValue("tsb_tile_size").get<int>();
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxCoC = 80;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_numWavelengths = 3;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_alphaThreshold = 1.0;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortDepthOffset = 0.01f;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortDepthScale = 0.0f;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_depthOffset = 0.0f;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_sortTileBuffer = true;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_normalizeResult = true;

			// Group sizes
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_interpolation = 8;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_merge = 8;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_splat = 32;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_convolution = Config::AttribValue("tsb_tile_size").get<int>();
			if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_tileSize <= 8)
			{
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_sort = 128;
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxSortElements = 256;
			}
			else
			{
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_sort = 256;
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxSortElements = 512;
				//object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_groupSizes.m_sort = 1024;
				//object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_maxSortElements = 4096;
			}

			// Aberration parameters
			// - Aberration conditions
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_name = Config::AttribValue("aberration").get<std::string>();

			// Aberration computation settings
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_backend = Aberration::PSFStackParameters::ComputationBackend::GPU;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_collectDebugInfo = false;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_logProgress = true;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_logStats = true;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_logDebug = false;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_cropThresholdCoeff = 0.00f;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_cropThresholdSum = 0.99f;

			// PSF settings
			//  - on-axis
			if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis)
			{
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS = glm::vec3(0.0f, 0.0f, 0.0f);
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP = glm::vec3(0.0f, 0.0f, 0.0f);
			}
			//  - off-axis
			else if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
			{
				if (Config::AttribValue("tsb_aperture_diameter").get<std::string>() == "Fixed" && Config::AttribValue("tsb_focus_distance").get<std::string>() == "Fixed")
				{
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS = glm::vec3(1.0f, 1.0f, 1.0f);
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP = glm::vec3(0.5f, 0.5f, 0.5f);
				}
				else
				{
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersS = glm::vec3(100.0f, 100.0f, 100.0f);
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfLayersP = glm::vec3(100.0f, 100.0f, 100.0f);
				}
			}

			// Parameter ranges
			//   - defocus dioptres
			if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis)
			{
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_objectDistances = { 0.125f, 8.125f, 33 };
			}
			else if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
			{
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_objectDistances = { 0.125f, 8.125f, 9 };
			}

			//   - incident angles
			//         - [on-axis]
			if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis)
			{
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesHorizontal = { 0.0f, 0.0f, 1 };
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesVertical = { 0.0f, 0.0f, 1 };
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearPsfTexture = true;
			}
			//         - [off-axis]
			else if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
			{
				if (camera->component<Camera::CameraComponent>().m_fovyMin == 50.0f)
				{
					if (Config::AttribValue("tsb_aperture_diameter").get<std::string>() == "Fixed" && Config::AttribValue("tsb_focus_distance").get<std::string>() == "Fixed")
					{
						object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesHorizontal = { -45.0f, 45.0f, 31 };
						object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesVertical = { -25.0f, 25.0f, 21 };
					}
					else
					{
						object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesHorizontal = { -45.0f, 45.0f, 23 };
						object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_incidentAnglesVertical = { -24.0f, 24.0f, 13 };
					}
				}
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearPsfTexture = false;
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearAberrationCache = true;
				// TODO: configure other field of views properly
			}
			//   - pupil sizes
			if (Config::AttribValue("tsb_aperture_diameter").get<std::string>() == "Fixed")
			{
				const float cameraAperture = camera->component<Camera::CameraComponent>().m_fixedAperture;
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_apertureDiameters = { cameraAperture, cameraAperture, 1 };
			}
			else if (Config::AttribValue("tsb_aperture_diameter").get<std::string>() == "Variable")
			{
				if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis)
				{
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_apertureDiameters = { 2.0f, 7.0f, 6 };
				}
				else if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
				{
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_apertureDiameters = { 2.0f, 6.0f, 5 };
				}
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearAberrationCache = true;
			}

			//   - focus distances
			if (Config::AttribValue("tsb_focus_distance").get<std::string>() == "Fixed")
			{
				const float cameraFocus = camera->component<Camera::CameraComponent>().m_focusDistance;
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_focusDistances = { 1.0f / cameraFocus, 1.0f / cameraFocus, 1 };
			}
			else if (Config::AttribValue("tsb_focus_distance").get<std::string>() == "Variable")
			{
				if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OnAxis)
				{
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_focusDistances = { 0.125f, 6.125f, 7 };
				}
				else if (object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_psfAxisMethod == TiledSplatBlur::TiledSplatBlurComponent::OffAxis)
				{
					object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfParameters.m_focusDistances = { 0.125f, 4.125f, 5 };
				}
				object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_clearAberrationCache = true;
			}

			// Preview
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfPreview.m_resolutionId = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_resolutionId;
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfPreview.m_resolution = RenderSettings::getResolutionById(scene, object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfPreview.m_resolutionId);
			object.component<TiledSplatBlur::TiledSplatBlurComponent>().m_aberration.m_psfPreview.m_fovy = camera->component<Camera::CameraComponent>().m_fovy;
		}));
	}
}