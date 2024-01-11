////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Profiler.h"
#include "Scene/Includes.h"

namespace Profiler
{
	////////////////////////////////////////////////////////////////////////////////
	/** Whether profiling is on by default or not. */
	int profilingDefault()
	{
		static int s_profiling = glm::clamp(Config::AttribValue("profiling").get<size_t>(), (size_t)0, (size_t)1);
		return s_profiling;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Maximum number of threads to profile. */
	int numProfiledThreads()
	{
		return glm::min(Threading::numThreads(), (int) Constants::s_maxProfilerThreads);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Filter condition for the parameter thread id. */
	bool threadFilter(int threadId)
	{
		return threadId == Threading::numThreads() || threadId < numProfiledThreads();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shouldStoreValue(Scene::Scene& scene, ProfilerDataEntry& entry)
	{
		// Determine if we need to store the profiler result or not
		Scene::Object* debugSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS);
		Scene::Object* guiSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_GUI_SETTINGS);
		if (debugSettings == nullptr || guiSettings == nullptr) 
			return false;

		return debugSettings->component<DebugSettings::DebugSettingsComponent>().m_profilerStoreValues && 
			guiSettings->component<GuiSettings::GuiSettingsComponent>().m_profilerChartsSettings.m_nodesToShow.count(entry.m_category) > 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	int getFrameId(Scene::Scene& scene)
	{
		// Extract the current frame id
		Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
		return simulationSettings != nullptr ? simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId : 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, std::string const& category)
	{
		auto it = scene.m_profilerData.find(category);
		if (it == scene.m_profilerData.end()) scene.m_profilerData[category].m_category = category;
		return scene.m_profilerData[category];
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, ProfilerTreeEntry& it)
	{
		return dataEntry(scene, it.m_category);
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, ProfilerThreadTreeIterator& it)
	{
		return dataEntry(scene, it->m_category);
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, ProfilerTreeIterator& it, size_t threadId)
	{
		return dataEntry(scene, it[threadId < numProfiledThreads() ? threadId : 0]->m_category);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for a time field. */
	void appendEntryTime(ProfilerDataEntry& entry, float const& time, bool sum)
	{
		++entry.m_currentCountTmp;
		if (entry.m_currentTmp.index() == 0 || sum == false)
		{
			entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldTime(time);
		}
		else
		{
			entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldTime((float)std::get_or(entry.m_currentTmp, ProfilerDataEntry::EntryDataFieldTime(0.0f)) + time);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for an integer value field. */
	void appendEntryIntValue(ProfilerDataEntry& entry, int const& value, bool sum)
	{
		++entry.m_currentCountTmp;
		if (entry.m_currentTmp.index() == 0 || sum == false)
		{
			entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherInt(value);
		}
		else
		{
			entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherInt((int)std::get_or(entry.m_currentTmp, ProfilerDataEntry::EntryDataFieldOtherInt(0)) + value);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for a float value field. */
	void appendEntryFloatValue(ProfilerDataEntry& entry, float const& value, bool sum)
	{
		++entry.m_currentCountTmp;
		if (entry.m_currentTmp.index() == 0 || sum == false)
		{
			entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherFloat(value);
		}
		else
		{
			entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherFloat((float)std::get_or(entry.m_currentTmp, ProfilerDataEntry::EntryDataFieldOtherFloat(0.0f)) + value);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for a string value field. */
	void appendEntryStringValue(ProfilerDataEntry& entry, std::string const& value, bool sum)
	{
		++entry.m_currentCountTmp;
		entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherString(value);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for a time field. */
	void storeEntryTime(ProfilerDataEntry& entry, int frameId, bool store)
	{
		++entry.m_totalEntryCount;
		entry.m_current = entry.m_currentTmp;
		entry.m_currentCount = entry.m_currentCountTmp;
		entry.m_totalValueCount += entry.m_currentCount;
		entry.m_min = ProfilerDataEntry::EntryDataFieldTime(glm::min((float)std::get_or(entry.m_min, ProfilerDataEntry::EntryDataFieldTime(FLT_MAX)), (float)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldTime(0.0f))));
		entry.m_max = ProfilerDataEntry::EntryDataFieldTime(glm::max((float)std::get_or(entry.m_max, ProfilerDataEntry::EntryDataFieldTime(-FLT_MAX)), (float)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldTime(0.0f))));
		entry.m_total = ProfilerDataEntry::EntryDataFieldTime((float)std::get_or(entry.m_total, ProfilerDataEntry::EntryDataFieldTime(0.0f)) + (float)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldTime(0.0f)));
		entry.m_avg = ProfilerDataEntry::EntryDataFieldTime((float)std::get<ProfilerDataEntry::EntryDataFieldTime>(entry.m_total) / entry.m_totalValueCount);
		if (store) entry.m_previousValues[frameId] = entry.m_current;
		entry.m_currentCountTmp = 0;
		entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldTime(0.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for an integer value field. */
	void storeEntryIntValue(ProfilerDataEntry& entry, int frameId, bool store)
	{
		++entry.m_totalEntryCount;
		entry.m_current = entry.m_currentTmp;
		entry.m_currentCount = entry.m_currentCountTmp;
		entry.m_totalValueCount += entry.m_currentCount;
		entry.m_min = ProfilerDataEntry::EntryDataFieldOtherInt(glm::min((int)std::get_or(entry.m_min, ProfilerDataEntry::EntryDataFieldOtherInt(INT_MAX)), (int)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldOtherInt(0))));
		entry.m_max = ProfilerDataEntry::EntryDataFieldOtherInt(glm::max((int)std::get_or(entry.m_max, ProfilerDataEntry::EntryDataFieldOtherInt(-INT_MAX)), (int)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldOtherInt(0))));
		entry.m_total = ProfilerDataEntry::EntryDataFieldOtherInt((int)std::get_or(entry.m_total, ProfilerDataEntry::EntryDataFieldOtherInt(0)) + (int)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldOtherInt(0)));
		entry.m_avg = ProfilerDataEntry::EntryDataFieldOtherInt((int)std::get<ProfilerDataEntry::EntryDataFieldOtherInt>(entry.m_total) / entry.m_totalValueCount);
		if (store) entry.m_previousValues[frameId] = entry.m_current;
		entry.m_currentCountTmp = 0;
		entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherInt(0);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for a float value field. */
	void storeEntryFloatValue(ProfilerDataEntry& entry, int frameId, bool store)
	{
		++entry.m_totalEntryCount;
		entry.m_current = entry.m_currentTmp;
		entry.m_currentCount = entry.m_currentCountTmp;
		entry.m_totalValueCount += entry.m_currentCount;
		entry.m_min = ProfilerDataEntry::EntryDataFieldOtherFloat(glm::min((float)std::get_or(entry.m_min, ProfilerDataEntry::EntryDataFieldOtherFloat(FLT_MAX)), (float)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldOtherFloat(0.0f))));
		entry.m_max = ProfilerDataEntry::EntryDataFieldOtherFloat(glm::max((float)std::get_or(entry.m_max, ProfilerDataEntry::EntryDataFieldOtherFloat(-FLT_MAX)), (float)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldOtherFloat(0.0f))));
		entry.m_total = ProfilerDataEntry::EntryDataFieldOtherFloat((float)std::get_or(entry.m_total, ProfilerDataEntry::EntryDataFieldOtherFloat(0.0f)) + (float)std::get_or(entry.m_current, ProfilerDataEntry::EntryDataFieldOtherFloat(0.0f)));
		entry.m_avg = ProfilerDataEntry::EntryDataFieldOtherFloat((float)std::get<ProfilerDataEntry::EntryDataFieldOtherFloat>(entry.m_total) / entry.m_totalValueCount);
		if (store) entry.m_previousValues[frameId] = entry.m_current;
		entry.m_currentCountTmp = 0;
		entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherFloat(0.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Update callback for a string value field. */
	void storeEntryStringValue(ProfilerDataEntry& entry, int frameId, bool store)
	{
		++entry.m_totalEntryCount;
		entry.m_current = entry.m_currentTmp;
		entry.m_currentCount = entry.m_currentCountTmp;
		entry.m_totalValueCount += entry.m_currentCount;
		entry.m_min = ProfilerDataEntry::EntryDataFieldOtherString(""s);
		entry.m_max = ProfilerDataEntry::EntryDataFieldOtherString(""s);
		entry.m_total = ProfilerDataEntry::EntryDataFieldOtherString(""s);
		entry.m_avg = ProfilerDataEntry::EntryDataFieldOtherString(""s);
		if (store) entry.m_previousValues[frameId] = entry.m_current;
		entry.m_currentCountTmp = 0;
		entry.m_currentTmp = ProfilerDataEntry::EntryDataFieldOtherString(""s);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the fully qualified name of a debug category. */
	std::string getFullyQualifiedName(Scene::Scene& scene, Category category, size_t threadId)
	{
		std::string result = category[0];

		if (threadId == Threading::numThreads())
			threadId = 0;

		for (auto it = scene.m_profilerWritePosition[threadId]; 
			scene.m_profilerTree[scene.m_profilerBufferWriteId][threadId].is_head(it) == false; 
			it = scene.m_profilerTree[scene.m_profilerBufferWriteId][threadId].parent(it))
		{
			result = it->m_name + "::" + result;
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(ProfilerDataEntry& entry, StoreDataCallback const& storeCallback, int frameId, bool shouldStore)
	{
		storeCallback(entry, frameId, shouldStore);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(Scene::Scene& scene, ProfilerDataEntry& entry, StoreDataCallback const& storeCallback)
	{
		storeCallback(entry, getFrameId(scene), shouldStoreValue(scene, entry));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(Scene::Scene& scene, ProfilerTreeIterator& entry, size_t threadId, StoreDataCallback const& storeCallback)
	{
		// Store the data for each thread
		Threading::invoke_for_threads(threadId, numProfiledThreads(), [&](int threadId)
		{
			storeValue(dataEntry(scene, entry, threadId), storeCallback, getFrameId(scene), shouldStoreValue(scene, dataEntry(scene, entry, threadId)));
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(Scene::Scene& scene, ProfilerTreeIterator& entry, StoreDataCallback const& storeCallback)
	{
		storeValue(scene, entry, Threading::numThreads(), storeCallback);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Stores a single instance of debug data */
	void storeData(Scene::Scene& scene, Category const& category, int data, bool sum, size_t threadId)
	{
		auto it = enterRegion(scene, category, threadId, [&](ProfilerDataEntry& entry)
		{
			storeValue(scene, entry, &storeEntryIntValue);
		});
		appendValue(scene, it, threadId, data, sum, &appendEntryIntValue);
		leaveRegion(scene, category, threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Stores a single instance of debug data */
	void storeData(Scene::Scene& scene, Category const& category, float data, bool sum, size_t threadId)
	{
		auto it = enterRegion(scene, category, threadId, [&](ProfilerDataEntry& entry)
		{
			storeValue(scene, entry, &storeEntryFloatValue);
		});
		appendValue(scene, it, threadId, data, sum, &appendEntryFloatValue);
		leaveRegion(scene, category, threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Stores a single instance of debug data */
	void storeData(Scene::Scene& scene, Category const& category, std::string const& data, bool sum, size_t threadId)
	{
		auto it = enterRegion(scene, category, threadId, [&](ProfilerDataEntry& entry)
		{
			storeValue(scene, entry, &storeEntryStringValue);
		});
		appendValue(scene, it, threadId, data, sum, &appendEntryStringValue);
		leaveRegion(scene, category, threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	void enterRegionImpl(Scene::Scene& scene, std::string const& category, ProfilerThreadTree& tree, ProfilerThreadTreeIterator& writePosition, size_t threadId, FutureValue const& finalize, FutureValue const& current)
	{
		// Look for an existing node
		for (auto it = tree.begin(writePosition); it != tree.end(writePosition); ++it)
		{
			if (it->m_name == category)
			{
				// Store the necessary callbacks
				if (it->m_futureValues.empty() && finalize) it->m_futureValues.push_back(finalize);
				if (current) it->m_futureValues.push_back(current);

				writePosition = it;
				return;
			}
		}

		// Construct the new entry
		ProfilerTreeEntry entry;
		entry.m_name = category;
		entry.m_category = getFullyQualifiedName(scene, category, threadId);
		if (finalize && current) entry.m_futureValues = { finalize, current };
		else if (finalize)       entry.m_futureValues = { finalize };
		else if (current)        entry.m_futureValues = { current };

		// Store it
		writePosition = scene.m_profilerTree[scene.m_profilerBufferWriteId][threadId].append_child(writePosition, entry);
		assert(writePosition.node != nullptr);
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator enterRegion(Scene::Scene& scene, std::string const& category, size_t threadId, FutureValue const& finalize, FutureValue const& current)
	{
		Threading::invoke_for_threads(threadId, numProfiledThreads(), [&](int threadId)
		{
			enterRegionImpl(scene, category, scene.m_profilerTree[scene.m_profilerBufferWriteId][threadId], scene.m_profilerWritePosition[threadId], threadId, finalize, current);
		});

		return scene.m_profilerWritePosition;
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator enterRegion(Scene::Scene& scene, Category const& category, size_t threadId, FutureValue const& finalize, FutureValue const& current)
	{
		for (size_t i = 0; i < category.size() - 1; ++i)
		{
			enterRegion(scene, category[i], threadId, nullptr, nullptr);
		}
		enterRegion(scene, category.back(), threadId, finalize, current);

		return scene.m_profilerWritePosition;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Leaves a debugging region */
	void leaveRegionImpl(Scene::Scene& scene, size_t threadId, int numRegions)
	{
		while (numRegions-- >= 1)
		{
			scene.m_profilerWritePosition[threadId] = scene.m_profilerTree[scene.m_profilerBufferWriteId][threadId].parent(scene.m_profilerWritePosition[threadId]);
			assert(scene.m_profilerWritePosition[threadId].node != nullptr);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator leaveRegion(Scene::Scene& scene, size_t threadId, int numRegions)
	{
		Threading::invoke_for_threads(threadId, numProfiledThreads(), [&](int threadId)
		{
			leaveRegionImpl(scene, threadId, numRegions);
		});

		return scene.m_profilerWritePosition;
	}

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator leaveRegion(Scene::Scene& scene, Category const& category, size_t threadId)
	{
		return leaveRegion(scene, threadId, category.size());
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCategory::ScopedCategory(Scene::Scene& scene, Category const& category, size_t threadId):
		m_scene(scene),
		m_category(category),
		m_threadId(threadId)
	{
		// Insert it into the tree
		m_iterator = enterRegion(m_scene, m_category, m_threadId,
			[&](ProfilerDataEntry& entry)
			{
				storeValue(scene, entry, &storeEntryStringValue);
			});
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCategory::~ScopedCategory()
	{
		// Update the entry
		appendValue(m_scene, m_iterator, m_threadId, ""s, false, &appendEntryStringValue);

		// Leave the region
		leaveRegion(m_scene, m_category, m_threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCpuPerfCounterImpl::ScopedCpuPerfCounterImpl(Scene::Scene& scene, Category const& category, bool sum, size_t threadId) :
		m_scene(scene),
		m_category(category),
		m_threadId(threadId),
		m_sum(sum)
	{
		// Store the start time
		m_times[0] = glfwGetTime();

		// Insert it into the tree
		m_iterator = enterRegion(m_scene, m_category, threadId, 
			[&](ProfilerDataEntry& entry)
			{
				storeValue(scene, entry, &storeEntryTime);
			});
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCpuPerfCounterImpl::~ScopedCpuPerfCounterImpl()
	{
		// Store the end time
		m_times[1] = glfwGetTime();

		// Compute the elapsed time
		float time = (m_times[1] - m_times[0]) * 1000.0f;

		// Update the entry
		appendValue(m_scene, m_iterator, m_threadId, time, m_sum, &appendEntryTime);

		// Leave the region
		leaveRegion(m_scene, m_category, m_threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
    ScopedCpuPerfCounter::ScopedCpuPerfCounter(Scene::Scene& scene, Category const& category, bool sum, size_t threadId)
    {
        // Extract the debug settings component
        Scene::Object* debugSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS);

        // Create the impl if we are profiling
        if (threadFilter(threadId) && ((debugSettings == nullptr && profilingDefault()) || (debugSettings != nullptr && debugSettings->component<DebugSettings::DebugSettingsComponent>().m_profileCpu)))
        {
            m_impl = std::make_unique<ScopedCpuPerfCounterImpl>(scene, category, sum, threadId);
        }
    }

	////////////////////////////////////////////////////////////////////////////////
	ScopedGpuPerfCounterImpl::ScopedGpuPerfCounterImpl(Scene::Scene& scene, Category const& category, bool sum) :
		m_scene(scene),
		m_counterName(getFullyQualifiedName(scene, category, Threading::numThreads())),
		m_category(category),
		m_sum(sum)
	{
		// Create the perf counter
		createPerfCounter(scene, m_counterName);

		// Place the perf counter
		glQueryCounter(scene.m_perfCounters[m_counterName].m_counters[0], GL_TIMESTAMP);

		// Store the start time
		m_times[0] = glfwGetTime();

		// Insert it into the tree
		enterRegion(m_scene, m_category, Threading::numThreads(), 
			[&](ProfilerDataEntry& entry)
			{
				storeValue(scene, entry, &storeEntryTime);
			},
			[&scene, &counter = scene.m_perfCounters[m_counterName], sum = m_sum](ProfilerDataEntry& entry)
			{
				// Compute the elapsed time
				GLint64 start, end;
				glGetQueryObjecti64v(counter.m_counters[0], GL_QUERY_RESULT, &start);
				glGetQueryObjecti64v(counter.m_counters[1], GL_QUERY_RESULT, &end);

				// Compute the elapsed time
				float time = (end - start) / 1000000.0f;

				// Store the value
				appendValue(entry, time, sum, &appendEntryTime);
			});
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedGpuPerfCounterImpl::~ScopedGpuPerfCounterImpl()
	{
		// Store the end time
		m_times[1] = glfwGetTime();

		// Compute the elapsed time
		// TODO: find a way to store this
		float time = (m_times[1] - m_times[0]) * 1000.0f;

		// Place the end counter
		glQueryCounter(m_scene.m_perfCounters[m_counterName].m_counters[1], GL_TIMESTAMP);

		// Leave the profiling region
		leaveRegion(m_scene, m_category, Threading::numThreads());
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a RAII scoped GPU perf counter. */
    ScopedGpuPerfCounter::ScopedGpuPerfCounter(Scene::Scene& scene, Category const& category, bool sum)
    {
        // Extract the debug settings component
        Scene::Object* debugSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS);

        // Create the impl if we are profiling
        if ((debugSettings == nullptr && profilingDefault()) || (debugSettings != nullptr && debugSettings->component<DebugSettings::DebugSettingsComponent>().m_profileGpu))
        {
            m_impl = std::make_unique<ScopedGpuPerfCounterImpl>(scene, category, sum);
        }
    }

	////////////////////////////////////////////////////////////////////////////////
	ScopedCounterImpl::ScopedCounterImpl(Scene::Scene& scene, Category const& category, size_t startCount, bool sum, size_t threadId) :
		m_scene(scene),
		m_category(category),
		m_threadId(threadId),
		m_count(startCount),
		m_sum(sum)
	{}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCounterImpl::~ScopedCounterImpl()
	{
		// Store the entry
		storeData(m_scene, m_category, m_count, m_sum, m_threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCounterImpl& ScopedCounterImpl::operator++()
	{
		++m_count;
		return *this;
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCounterImpl& ScopedCounterImpl::operator--()
	{
		--m_count;
		return *this;
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCounterImpl& ScopedCounterImpl::operator=(int i)
	{
		m_count = i;
		return *this;
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedCounterImpl::operator int() const
	{
		return m_count;
	}

	////////////////////////////////////////////////////////////////////////////////
    ScopedCounter::ScopedCounter(Scene::Scene& scene, Category const& category, size_t startCount, bool sum, size_t threadId)
    {
        // Extract the debug settings component
        Scene::Object* debugSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DEBUG_SETTINGS);

        // Create the impl if we are profiling
        if (threadFilter(threadId) && ((debugSettings == nullptr && profilingDefault()) || (debugSettings != nullptr && debugSettings->component<DebugSettings::DebugSettingsComponent>().m_profileCpu)))
        {
            m_impl = std::make_unique<ScopedCounterImpl>(scene, category, startCount, sum, threadId);
        }
    }

	////////////////////////////////////////////////////////////////////////////////
    ScopedCounter& ScopedCounter::operator++()
    {
        if (m_impl.get() != nullptr) m_impl->operator++();
        return *this;
    }

	////////////////////////////////////////////////////////////////////////////////
    ScopedCounter& ScopedCounter::operator--()
    {
        if (m_impl.get() != nullptr) m_impl->operator--();
        return *this;
    }

	////////////////////////////////////////////////////////////////////////////////
    ScopedCounter& ScopedCounter::operator=(int i)
    {
        if (m_impl.get() != nullptr) m_impl->operator=(i);
        return *this;
    }

	////////////////////////////////////////////////////////////////////////////////
    ScopedCounter::operator int() const
    {
        if (m_impl.get() != nullptr) return m_impl->operator int();
		return 0;
    }

	////////////////////////////////////////////////////////////////////////////////
	void queryFutureValues(Scene::Scene& scene, ProfilerThreadTree const& tree, ProfilerThreadTreeIterator root, size_t threadId)
	{
		// Iterate over the children
		for (auto it = tree.begin(root); it != tree.end(root); ++it)
		{
			// Extract the list of futures to evaluate
			auto& futures = it->m_futureValues;

			// Invoke the callbacks
			for (auto future = futures.rbegin(); future != futures.rend(); ++future)
			{
				(*future)(dataEntry(scene, *it));
			}
			futures.clear();

			// Do this recursively, if they do
			if (std::distance(tree.begin(it), tree.end(it)) > 0)
			{
				queryFutureValues(scene, tree, it, threadId);
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	void init(Scene::Scene& scene)
	{
		// Initialize the profiler tree
		for (size_t i = 0; i < scene.m_profilerWritePosition.size(); ++i)
		{
			scene.m_profilerWritePosition[i] = scene.m_profilerTree[scene.m_profilerBufferWriteId][i].set_head(Profiler::ProfilerTreeEntry{});
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void beginFrame(Scene::Scene& scene)
	{
		// Query the queued future values
		queryFutureValues(scene, scene.m_profilerTree[scene.m_profilerBufferReadId][0], scene.m_profilerTree[scene.m_profilerBufferReadId][0].begin(), 0);
	}

	////////////////////////////////////////////////////////////////////////////////
	void endFrame(Scene::Scene& scene)
	{
		// Swap the profiler buffers
		scene.m_profilerBufferWriteId = (scene.m_profilerBufferWriteId + 1) % scene.m_profilerTree.size();
		scene.m_profilerBufferReadId = (scene.m_profilerBufferReadId + 1) % scene.m_profilerTree.size();

		for (size_t i = 0; i < Profiler::numProfiledThreads(); ++i)
		{
			if (scene.m_profilerTree[scene.m_profilerBufferWriteId][i].empty())
			{
				scene.m_profilerWritePosition[i] = scene.m_profilerTree[scene.m_profilerBufferWriteId][i].set_head(Profiler::ProfilerTreeEntry{});
			}
			else
			{
				scene.m_profilerTree[scene.m_profilerBufferWriteId][i] = ProfilerThreadTree(ProfilerTreeEntry{});
				scene.m_profilerWritePosition[i] = scene.m_profilerTree[scene.m_profilerBufferWriteId][i].begin();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Prints the profiler sub-tree. */
	void printTree(Scene::Scene& scene, ProfilerThreadTree const& tree, ProfilerThreadTreeIterator root, int depth, int threadId)
	{
		// Iterate over the children
		for (auto it = tree.begin(root); it != tree.end(root); ++it)
		{
			Debug::log_info() << std::string(depth * 4, ' ');

			// Extract the data entry
			auto& node = dataEntry(scene, *it);

			// Extract the current value
			std::array<std::string, 7> values =
			{
				it->m_name.c_str(),
				Profiler::convertEntryData<std::string>(node.m_current),
				Profiler::convertEntryData<std::string>(node.m_min),
				Profiler::convertEntryData<std::string>(node.m_avg),
				Profiler::convertEntryData<std::string>(node.m_max),
				Profiler::convertEntryData<std::string>(node.m_total),
				std::to_string(node.m_totalEntryCount)
			};

			for (int i = 0; i < values.size(); ++i)
			{
				Debug::log_info() << values[i] << " ";
			}

			Debug::log_info() << Debug::end;

			// Do this recursively, if they do
			if (std::distance(tree.begin(it), tree.end(it)) > 0)
			{
				printTree(scene, tree, it, depth + 1, threadId);
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Prints the profiler tree. */
	void printTree(Scene::Scene& scene)
	{
		printTree(scene, scene.m_profilerTree[scene.m_profilerBufferWriteId][0], scene.m_profilerTree[scene.m_profilerBufferWriteId][0].begin(), 0, 0);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Clears the profiler tree. */
	void clearTree(Scene::Scene& scene)
	{
		scene.m_profilerData.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Prints the profiler sub-tree. */
	void exportTreeCsv(Scene::Scene& scene, std::stringstream& result, std::regex const& rootCategory, bool print, ProfilerThreadTree const& tree, ProfilerThreadTreeIterator root, std::string prefix, int depth, int threadId)
	{
		// Iterate over the children
		for (auto it = tree.begin(root); it != tree.end(root); ++it)
		{
			// Update the print flag
			bool printThis = printThis = print || std::regex_match(it->m_category, rootCategory);

			// Print the current values
			if (printThis)
			{
				// Extract the data entry
				auto& node = dataEntry(scene, *it);

				// Extract the current values
				std::array<std::string, 8> values =
				{
					it->m_category,
					it->m_name,
					Profiler::convertEntryData<std::string>(node.m_current),
					Profiler::convertEntryData<std::string>(node.m_min),
					Profiler::convertEntryData<std::string>(node.m_avg),
					Profiler::convertEntryData<std::string>(node.m_max),
					Profiler::convertEntryData<std::string>(node.m_total),
					std::to_string(node.m_totalEntryCount)
				};

				// Append them to the output
				for (int i = 0; i < values.size(); ++i)
				{
					result << values[i] << ";";
				}

				result << std::endl;
			}

			// Do this recursively, if they do
			if (std::distance(tree.begin(it), tree.end(it)) > 0)
			{
				exportTreeCsv(scene, result, rootCategory, printThis, tree, it, prefix, depth + 1, threadId);
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Exports the profiler tree. */
	std::string exportTreeCsv(Scene::Scene& scene, std::string rootCategory, bool printHeaders)
	{
		// Validate the root category
		try
		{
			std::regex r(rootCategory);
		}
		catch (std::exception e)
		{
			Debug::log_error() << "Invalid regex pattern '" << rootCategory << "' - fallback to default pattern" << Debug::end;
			rootCategory = ".*";
		}

		// Resulting stringstream
		std::stringstream result;
		
		// Column headers
		std::array<std::string, 8> headers =
		{
			"FullName",
			"EntryName",
			"Current",
			"Min",
			"Max",
			"Average",
			"Count",
			"Total"
		};

		// print the headers
		if (printHeaders)
		{
			for (int i = 0; i < headers.size(); ++i)
			{
				result << headers[i] << ";";
			}
			result << std::endl;
		}

		// Iterate the current tree root
		exportTreeCsv(scene, result, std::regex(rootCategory), false, scene.m_profilerTree[scene.m_profilerBufferReadId][0], scene.m_profilerTree[scene.m_profilerBufferReadId][0].begin(), "", 0, 0);

		// Return the result
		return result.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(Application, Profiling, -profiling, 1, 0)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"profiling", "Application",
			"Should we enable profiling by default?",
			"0|1", { "0" }, {},
			Config::attribRegexBool()
		});
	};
}