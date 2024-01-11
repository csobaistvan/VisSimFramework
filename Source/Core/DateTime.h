#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Constants.h"
#include "Debug.h"

namespace DateTime
{
	////////////////////////////////////////////////////////////////////////////////
	meta_enum(TimeUnit, int, Nanoseconds, Microseconds, Milliseconds, Seconds, Minutes, Hours, Days, Years);

	////////////////////////////////////////////////////////////////////////////////
	// Represent a timer object
	struct Timer
	{
		Timer(bool autoStart = false);

		bool start();
		bool stop();

		double getElapsedTime() const;
		double getAvgTime(size_t numComputations) const;
		std::string getElapsedTime(TimeUnit minTimeUnit, bool truncate = true, bool shorten = false) const;
		std::string getAvgTime(size_t numComputations, TimeUnit minTimeUnit, bool truncate = true, bool shorten = false) const;

		// ---- Private members

		bool m_isRunning = false;
		double m_startTime = DBL_MAX;
		double m_endTime = DBL_MAX;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct ScopedTimer
	{
		ScopedTimer(const Debug::DebugOutputLevel logLevel, const size_t numComputations, const TimeUnit minTimeUnit, std::string const& computationName, bool truncate = true, bool shorten = false, Timer* timer = nullptr);
		ScopedTimer(ScopedTimer const& other) = delete;
		ScopedTimer(ScopedTimer&& other) = default;
		~ScopedTimer();

		// ---- Private members

		Debug::DebugOutputLevel m_outputLevel;
		std::string m_computationName;
		size_t m_numComputations;
		TimeUnit m_minTimeUnit;
		bool m_truncate;
		bool m_shorten;
		Timer* m_timer;
		Timer m_defaultTimer;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct TimerSet
	{
		struct ScopedComputation
		{
			ScopedComputation(TimerSet* owner, size_t numComputations, std::string const& computationName);
			ScopedComputation(ScopedComputation const& other) = delete;
			ScopedComputation(ScopedComputation&& other) = default;
			~ScopedComputation();

			Timer* startComputation(std::string const& computationName, size_t numComputations);
			void finishComputation();

			TimerSet* m_owner;
			Timer* m_timer;
			ScopedTimer m_scopedTimer;
		};

		TimerSet() = default;
		TimerSet(Debug::DebugOutputLevel logLevel, TimeUnit minTimeUnit);

		ScopedComputation startComputation(std::string const& computationName, size_t numComputations);

		void displaySummary() const;
		void displaySummary(Debug::DebugOutputLevel logLevel, TimeUnit minTimeUnit, bool truncate = true, bool shorten = false) const;

		// ---- Private members

		struct Computation
		{
			std::string m_name;
			size_t m_numComputations;
			Timer m_timer;
		};

		using ComputationTree = tree<Computation>;
		using ComputationIt = tree<Computation>::iterator;

		void collectTimersetTableEntries(TimeUnit minTimeUnit, bool truncate, bool shorten, tabulate::Table& result, size_t depth, ComputationIt root) const;

		Debug::DebugOutputLevel m_outputLevel;
		TimeUnit m_minTimeUnit;
		ComputationTree m_computations;
		ComputationIt m_writePos;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Various commonly used date format definitions. */
	const char* timeFormatDisplay();
	const char* dateFormatFilename();
	const char* dateFormatDisplay();

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateStringUtf8(const tm* t, std::string const& format);

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateStringUtf8(time_t t, std::string const& format);

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateStringUtf8(std::string const& format);

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateString(const tm* t, std::string const& format);

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateString(time_t t, std::string const& format);

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateString(std::string const& format);

	////////////////////////////////////////////////////////////////////////////////
	std::string getDeltaTimeFormatted(double delta, const bool truncate = false, const bool shorten = false, const TimeUnit minTimeUnit = Seconds);
}