#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
/// PROFILER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace Profiler
{
	////////////////////////////////////////////////////////////////////////////////
	// Forward declaration of the data entry
	struct ProfilerDataEntry;

	////////////////////////////////////////////////////////////////////////////////
	/** Whether profiling is on by default or not. */
	int profilingDefault();

	////////////////////////////////////////////////////////////////////////////////
	/** Maximum number of threads to profile. */
	int numProfiledThreads();

	////////////////////////////////////////////////////////////////////////////////
	/** Filter condition for the parameter thread id. */
	bool threadFilter(int threadId);

	////////////////////////////////////////////////////////////////////////////////
	bool shouldStoreValue(Scene::Scene& scene, ProfilerDataEntry& entry);

	////////////////////////////////////////////////////////////////////////////////
	int getFrameId(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a profiler category */
	struct Category : std::vector<std::string>
	{
		using vector::vector;

		Category(std::string s) :
			vector({ s })
		{}

		Category(const char* s) :
			vector({ std::string(s) })
		{}
	};

	////////////////////////////////////////////////////////////////////////////////
	// Future callback type.
	using FutureValue = std::function<void(ProfilerDataEntry&)>;

	////////////////////////////////////////////////////////////////////////////////
	/** Profiler data block. */
	struct ProfilerDataEntry
	{
		// Full category of the entry
		std::string m_category = "";

		// The various entry data types
		struct EntryDataFieldTime : type_safe::strong_typedef<EntryDataFieldTime, float> { using strong_typedef::strong_typedef; };
		struct EntryDataFieldCountedInt : type_safe::strong_typedef<EntryDataFieldTime, int>{ using strong_typedef::strong_typedef; };
		struct EntryDataFieldCountedFloat : type_safe::strong_typedef<EntryDataFieldTime, float>{ using strong_typedef::strong_typedef; };
		struct EntryDataFieldOtherInt : type_safe::strong_typedef<EntryDataFieldOtherInt, int> { using strong_typedef::strong_typedef; };
		struct EntryDataFieldOtherFloat : type_safe::strong_typedef<EntryDataFieldOtherFloat, float> { using strong_typedef::strong_typedef; };
		struct EntryDataFieldOtherString : type_safe::strong_typedef<EntryDataFieldOtherFloat, std::string>{ using strong_typedef::strong_typedef; };

		// Represents one field of all the entry data.
		using EntryDataField = std::variant<std::monostate, EntryDataFieldTime, EntryDataFieldOtherInt, EntryDataFieldOtherFloat, EntryDataFieldOtherString>;

		// Partial sum of the current value
		EntryDataField m_currentTmp;

		// Current value for this frame.
		EntryDataField m_current;

		// Minimum value.
		EntryDataField m_min;

		// Maximum value
		EntryDataField m_max;

		// Total value
		EntryDataField m_total;

		// Number of values, in total
		int m_currentCountTmp = 0;

		// Number of values, in total
		int m_currentCount = 0;

		// Number of entries, in total
		int m_totalEntryCount = 0;

		// Number of values, in total
		int m_totalValueCount = 0;

		// Average value
		EntryDataField m_avg;

		// Previous values
		std::unordered_map<unsigned, EntryDataField> m_previousValues;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct ProfilerTreeEntry
	{
		// Name of the data entry
		std::string m_name = "";

		// Full category of the entry
		std::string m_category = "";

		// Future value callbacks that are executed before its value is retrieved.
		std::vector<FutureValue> m_futureValues;
	};

	////////////////////////////////////////////////////////////////////////////////
	using ProfilerData = std::unordered_map<std::string, ProfilerDataEntry>;

	////////////////////////////////////////////////////////////////////////////////
	/** Data structure for holding the debug data entries. */
	using ProfilerThreadTree = tree<ProfilerTreeEntry>;
	using ProfilerTree = std::array<ProfilerThreadTree, Constants::s_maxProfilerThreads>;

	////////////////////////////////////////////////////////////////////////////////
	/** An iterator into the profiler data tree */
	using ProfilerThreadTreeIterator = typename ProfilerTree::value_type::iterator;
	using ProfilerTreeIterator = std::array<ProfilerThreadTreeIterator, Constants::s_maxProfilerThreads>;

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, ProfilerTreeEntry& it);

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, ProfilerThreadTreeIterator& it);

	////////////////////////////////////////////////////////////////////////////////
	ProfilerDataEntry& dataEntry(Scene::Scene& scene, ProfilerTreeIterator& it, size_t threadId);

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator enterRegion(Scene::Scene& scene, std::string const& category, size_t threadId, FutureValue const& finalize, FutureValue const& current = nullptr);

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator enterRegion(Scene::Scene& scene, Category const& category, size_t threadId, FutureValue const& finalize, FutureValue const& current = nullptr);

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator leaveRegion(Scene::Scene& scene, size_t threadId, int numRegions = 1);

	////////////////////////////////////////////////////////////////////////////////
	ProfilerTreeIterator leaveRegion(Scene::Scene& scene, Category const& category, size_t threadId);

	////////////////////////////////////////////////////////////////////////////////
	void appendEntryTime(ProfilerDataEntry& entry, float const& time, bool sum);
	void appendEntryIntValue(ProfilerDataEntry& entry, int const& value, bool sum);
	void appendEntryFloatValue(ProfilerDataEntry& entry, float const& value, bool sum);
	void appendEntryStringValue(ProfilerDataEntry& entry, std::string const& value, bool sum);

	////////////////////////////////////////////////////////////////////////////////
	void storeEntryTime(ProfilerDataEntry& entry, int frameId, bool store);
	void storeEntryIntValue(ProfilerDataEntry& entry, int frameId, bool store);
	void storeEntryFloatValue(ProfilerDataEntry& entry, int frameId, bool store);
	void storeEntryStringValue(ProfilerDataEntry& entry, int frameId, bool store);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T> using AppendDataCallback = void(*)(ProfilerDataEntry&, T const&, bool);
	using StoreDataCallback = void(*)(ProfilerDataEntry&, int, bool);

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	template<typename T>
	void appendValue(ProfilerDataEntry& entry, T const& value, bool sum, AppendDataCallback<T> const& appendCallback)
	{
		appendCallback(entry, value, sum);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	template<typename T>
	void appendValue(Scene::Scene& scene, ProfilerTreeIterator& entry, size_t threadId, T const& value, bool sum, AppendDataCallback<T> const& appendCallback)
	{
		// Store the data for each thread
		Threading::invoke_for_threads(threadId, numProfiledThreads(), [&](int threadId)
		{
			appendValue(dataEntry(scene, entry, threadId), value, sum, appendCallback);
		});
	}
	
	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	template<typename T>
	void appendValue(Scene::Scene& scene, ProfilerTreeIterator& entry, T const& value, bool sum, AppendDataCallback<T> const& appendCallback)
	{
		appendValue(scene, entry, Threading::numThreads(), value, appendCallback);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(ProfilerDataEntry& entry, StoreDataCallback const& storeCallback, int frameId, bool shouldStore);
	
	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(Scene::Scene& scene, ProfilerDataEntry& entry, StoreDataCallback const& storeCallback);

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(Scene::Scene& scene, ProfilerTreeIterator& entry, size_t threadId, StoreDataCallback const& storeCallback);

	////////////////////////////////////////////////////////////////////////////////
	/** Generic update callback handler. */
	void storeValue(Scene::Scene& scene, ProfilerTreeIterator& entry, StoreDataCallback const& storeCallback);

	////////////////////////////////////////////////////////////////////////////////
	/** Stores a single instance of debug data */
	void storeData(Scene::Scene& scene, Category const& category, int data, bool sum = false, size_t threadId = Threading::numThreads());
	void storeData(Scene::Scene& scene, Category const& category, float data, bool sum = false, size_t threadId = Threading::numThreads());
	void storeData(Scene::Scene& scene, Category const& category, std::string const& data, bool sum = false, size_t threadId = Threading::numThreads());

	////////////////////////////////////////////////////////////////////////////////
	template<typename T> struct EntryDataIndex{ static const int index() { return 0; } };
	template<> struct EntryDataIndex<ProfilerDataEntry::EntryDataFieldTime> { static const int index() { return 1; } };
	template<> struct EntryDataIndex<ProfilerDataEntry::EntryDataFieldCountedInt> { static const int index() { return 0; } };
	template<> struct EntryDataIndex<ProfilerDataEntry::EntryDataFieldCountedFloat> { static const int index() { return 0; } };
	template<> struct EntryDataIndex<ProfilerDataEntry::EntryDataFieldOtherInt> { static const int index() { return 2; } };
	template<> struct EntryDataIndex<ProfilerDataEntry::EntryDataFieldOtherFloat> { static const int index() { return 3; } };
	template<> struct EntryDataIndex<ProfilerDataEntry::EntryDataFieldOtherString> { static const int index() { return 4; } };

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct EntryDataConverter
	{
		static inline T convert(std::monostate) { return T(); }
		static inline T convert(ProfilerDataEntry::EntryDataFieldTime value) { return (T)((float)value); }
		static inline T convert(ProfilerDataEntry::EntryDataFieldOtherInt value) { return (T)((int)value); }
		static inline T convert(ProfilerDataEntry::EntryDataFieldOtherFloat value) { return (T)((float)value); }
		static inline T convert(ProfilerDataEntry::EntryDataFieldOtherString value) { return T(); }
	};

	////////////////////////////////////////////////////////////////////////////////
	template<>
	struct EntryDataConverter<float>
	{
		static inline float convert(std::monostate) { return 0.0f; }
		static inline float convert(ProfilerDataEntry::EntryDataFieldTime value) { return (float)value; }
		static inline float convert(ProfilerDataEntry::EntryDataFieldOtherInt value) { return (int)value; }
		static inline float convert(ProfilerDataEntry::EntryDataFieldOtherFloat value) { return (float)value; }
		static inline float convert(ProfilerDataEntry::EntryDataFieldOtherString value) { return 0.0f; }
	};

	////////////////////////////////////////////////////////////////////////////////
	template<>
	struct EntryDataConverter<std::string>
	{
		static inline std::string convert(std::monostate) { return ""; }
		static inline std::string convert(ProfilerDataEntry::EntryDataFieldOtherInt value) { return std::to_string((int)value); }
		static inline std::string convert(ProfilerDataEntry::EntryDataFieldOtherFloat value) { return std::to_string((float)value); }
		static inline std::string convert(ProfilerDataEntry::EntryDataFieldOtherString value) { return (std::string)value; }
		static inline std::string convert(ProfilerDataEntry::EntryDataFieldTime value) { return Units::secondsToString(Units::millisecondsToSeconds((float)value)); }
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T convertEntryData(ProfilerDataEntry::EntryDataField value)
	{
		return std::visit([](auto v) { return EntryDataConverter<T>::convert(v); }, value);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	bool isEntryType(ProfilerDataEntry::EntryDataField value)
	{
		return value.index() == EntryDataIndex<T>::index();
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T slidingAverage(ProfilerDataEntry const& entry, int firstFrame, int lastFrame, bool countNonExistent = true)
	{
		// Result and number of entries
		T sum = T(0);
		int count = 0;

		// Compute the sum
		for (int i = firstFrame; i <= lastFrame; ++i)
		{
			if (auto it = entry.m_previousValues.find(i); it != entry.m_previousValues.end())
			{
				sum += convertEntryData<T>(it->second);
				++count;
			}
			else if (countNonExistent)
			{
				sum += T(0);
				++count;
			}
		}

		// Return the final average
		return sum / T(count > 0 ? count : 1);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T slidingAverageWindowed(ProfilerDataEntry const& entry, int frameId, int windowSize, bool countNonExistent = true, bool backward = true, bool forward = true)
	{
		// Make sure the window size is odd
		windowSize = (windowSize / 2) * 2 + 1;

		// Compute the average
		return slidingAverage<T>(entry, backward ? frameId - windowSize / 2 : frameId, forward ? frameId + windowSize / 2 : frameId, countNonExistent);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T slidingMin(ProfilerDataEntry const& entry, int firstFrame, int lastFrame)
	{
		// Result and number of entries
		T min = T(0);
		bool first = true;

		// Compute the sum
		for (int i = firstFrame; i <= lastFrame; ++i)
		{
			if (auto it = entry.m_previousValues.find(i); it != entry.m_previousValues.end())
			{
				T curr = convertEntryData<T>(it->second);
				min = first ? curr : std::min(min, curr);
				first = false;
			}
		}

		// Return the final result
		return min;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T slidingMinWindowed(ProfilerDataEntry const& entry, int frameId, int windowSize, bool backward = true, bool forward = true)
	{
		// Make sure the window size is odd
		windowSize = (windowSize / 2) * 2 + 1;

		// Compute the average
		return slidingMin<T>(entry, backward ? frameId - windowSize / 2 : frameId, forward ? frameId + windowSize / 2 : frameId);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T slidingMax(ProfilerDataEntry const& entry, int firstFrame, int lastFrame)
	{
		// Result and number of entries
		T max = T(0);
		bool first = true;

		// Compute the sum
		for (int i = firstFrame; i <= lastFrame; ++i)
		{
			if (auto it = entry.m_previousValues.find(i); it != entry.m_previousValues.end())
			{
				T curr = convertEntryData<T>(it->second);
				max = first ? curr : std::max(max, curr);
				first = false;
			}
		}

		// Return the final result
		return max;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T slidingMaxWindowed(ProfilerDataEntry const& entry, int frameId, int windowSize, bool backward = true, bool forward = true)
	{
		// Make sure the window size is odd
		windowSize = (windowSize / 2) * 2 + 1;

		// Compute the average
		return slidingMax<T>(entry, backward ? frameId - windowSize / 2 : frameId, forward ? frameId + windowSize / 2 : frameId);
	}

	////////////////////////////////////////////////////////////////////////////////
	struct ScopedCategory
	{
		Scene::Scene& m_scene;
		size_t m_threadId;
		Category m_category;
		ProfilerTreeIterator m_iterator;

		ScopedCategory(Scene::Scene& scene, Category const& category, size_t threadId = Threading::currentThreadId());
		~ScopedCategory();
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Scoped CPU perf counter impl. */
	struct ScopedCpuPerfCounterImpl
	{
		Scene::Scene& m_scene;
		bool m_sum;
		size_t m_threadId;
		double m_times[2];
		ProfilerTreeIterator m_iterator;
		Category m_category;

		ScopedCpuPerfCounterImpl(Scene::Scene& scene, Category const& category, bool sum, size_t threadId);
		~ScopedCpuPerfCounterImpl();
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a RAII scoped CPU perf counter. */
	struct ScopedCpuPerfCounter
	{
		std::unique_ptr<ScopedCpuPerfCounterImpl> m_impl;

		ScopedCpuPerfCounter(Scene::Scene& scene, Category const& category, bool sum = false, size_t threadId = Threading::currentThreadId());
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Scoped GPU perf counter implementation. */
	struct ScopedGpuPerfCounterImpl
	{
		Scene::Scene& m_scene;
		std::string m_counterName;
		Category m_category;
		bool m_sum;
		double m_times[2];

		ScopedGpuPerfCounterImpl(Scene::Scene& scene, Category const& category, bool sum);
		~ScopedGpuPerfCounterImpl();
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a RAII scoped GPU perf counter. */
	struct ScopedGpuPerfCounter
	{
		std::unique_ptr<ScopedGpuPerfCounterImpl> m_impl;

		ScopedGpuPerfCounter(Scene::Scene& scene, Category const& category, bool sum = false);
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Scoped counter object implementation. */
	struct ScopedCounterImpl
	{
		Scene::Scene& m_scene;
		int m_count;
		Category m_category;
		bool m_sum;
		size_t m_threadId;

		ScopedCounterImpl(Scene::Scene& scene, Category const& category, size_t startCount = 0, bool sum = false, size_t threadId = Threading::numThreads());
		~ScopedCounterImpl();

		ScopedCounterImpl& operator++();
		ScopedCounterImpl& operator--();
		ScopedCounterImpl& operator=(int i);
		operator int() const;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a RAII scoped counter object. */
	struct ScopedCounter
	{
		std::unique_ptr<ScopedCounterImpl> m_impl;

		ScopedCounter(Scene::Scene& scene, Category const& category, size_t startCount = 0, bool sum = false, size_t threadId = Threading::numThreads());

		ScopedCounter& operator++();
		ScopedCounter& operator--();
		ScopedCounter& operator=(int i);
		operator int() const;
	};

	////////////////////////////////////////////////////////////////////////////////
	void init(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void beginFrame(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void endFrame(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	/** Prints the profiler tree. */
	void printTree(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	/** Clears the profiler tree. */
	void clearTree(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	/** Exports the profiler tree. */
	std::string exportTreeCsv(Scene::Scene& scene, std::string rootCategory, bool printHeaders = true);
}