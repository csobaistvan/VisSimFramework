#include "PCH.h"
#include "DateTime.h"
#include "LibraryExtensions/TabulateEx.h"

namespace DateTime
{
	////////////////////////////////////////////////////////////////////////////////
	Timer::Timer(bool autoStart)
	{
		if (autoStart) start();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool Timer::start()
	{
		bool wasRunning = m_isRunning;
		m_isRunning = true;
		m_startTime = glfwGetTime();
		return wasRunning;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool Timer::stop()
	{
		bool wasRunning = m_isRunning;
		m_isRunning = false;
		m_endTime = glfwGetTime();
		return wasRunning;
	}

	////////////////////////////////////////////////////////////////////////////////
	double Timer::getElapsedTime() const
	{
		return m_endTime - m_startTime;
	}

	////////////////////////////////////////////////////////////////////////////////
	double Timer::getAvgTime(size_t numComputations) const
	{
		return getElapsedTime() / std::max(size_t(1), numComputations);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string Timer::getElapsedTime(TimeUnit minTimeUnit, bool truncate, bool shorten) const
	{
		return getDeltaTimeFormatted(getElapsedTime(), truncate, shorten, minTimeUnit);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string Timer::getAvgTime(size_t numComputations, TimeUnit minTimeUnit, bool truncate, bool shorten) const
	{
		return getDeltaTimeFormatted(getAvgTime(numComputations), truncate, shorten, minTimeUnit);
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedTimer::ScopedTimer(const Debug::DebugOutputLevel logLevel, const size_t numComputations, const TimeUnit minTimeUnit, std::string const& computationName, bool truncate, bool shorten, Timer* timer):
		m_outputLevel(logLevel),
		m_computationName(computationName),
		m_numComputations(numComputations),
		m_minTimeUnit(minTimeUnit),
		m_truncate(truncate),
		m_shorten(shorten),
		m_defaultTimer(false),
		m_timer(timer != nullptr ? timer : &m_defaultTimer)
	{
		Debug::log_output(m_outputLevel)
			<< "Computation (" << m_computationName << ", " << m_numComputations << " computations" << ") "
			<< "started."
			<< Debug::end;

		m_timer->start();
	}

	////////////////////////////////////////////////////////////////////////////////
	ScopedTimer::~ScopedTimer()
	{
		if (m_timer)
		{
			m_timer->stop();

			Debug::log_output(m_outputLevel)
				<< "Computation (" << m_computationName << ", " << m_numComputations << " computations" << ") "
				<< "finished in " << m_timer->getElapsedTime(m_minTimeUnit, m_truncate, m_shorten) << ", "
				<< "average time per entry: " << m_timer->getAvgTime(m_numComputations, m_minTimeUnit, m_truncate, m_shorten)
				<< Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	TimerSet::ScopedComputation::ScopedComputation(TimerSet* owner, size_t numComputations, std::string const& computationName):
		m_owner(owner),
		m_timer(startComputation(computationName, numComputations)),
		m_scopedTimer(owner->m_outputLevel, numComputations, owner->m_minTimeUnit, computationName, false, false, m_timer)
	{}

	////////////////////////////////////////////////////////////////////////////////
	TimerSet::ScopedComputation::~ScopedComputation()
	{
		finishComputation();
	}

	////////////////////////////////////////////////////////////////////////////////
	Timer* TimerSet::ScopedComputation::startComputation(std::string const& computationName, size_t numComputations)
	{
		m_owner->m_writePos = m_owner->m_computations.append_child(m_owner->m_writePos, Computation{ computationName, numComputations });
		return &(m_owner->m_writePos->m_timer);
	}

	////////////////////////////////////////////////////////////////////////////////
	void TimerSet::ScopedComputation::finishComputation()
	{
		m_owner->m_writePos = m_owner->m_computations.parent(m_owner->m_writePos);
	}

	////////////////////////////////////////////////////////////////////////////////
	TimerSet::TimerSet(Debug::DebugOutputLevel logLevel, TimeUnit minTimeUnit):
		m_outputLevel(logLevel),
		m_minTimeUnit(minTimeUnit),
		m_writePos(m_computations.set_head(Computation{ "Root", 1 }))
	{}

	////////////////////////////////////////////////////////////////////////////////
	TimerSet::ScopedComputation TimerSet::startComputation(std::string const& computationName, size_t numComputations)
	{
		return ScopedComputation(this, numComputations, computationName);
	}

	////////////////////////////////////////////////////////////////////////////////
	void TimerSet::displaySummary() const
	{
		displaySummary(m_outputLevel, m_minTimeUnit);
	}

	////////////////////////////////////////////////////////////////////////////////
	void TimerSet::collectTimersetTableEntries(TimeUnit minTimeUnit, bool truncate, bool shorten, tabulate::Table& result, size_t depth, ComputationIt root) const
	{
		const bool isRoot = root->m_name == "Root";

		// Append the current row (unless it's the root)
		if (!isRoot)
		{
			result.add_row
			({
				(depth == 0 ? "" : ("|" + std::string(depth, '-')) + " ") + root->m_name,
				std::to_string(root->m_numComputations),
				std::to_string(root->m_timer.getElapsedTime(minTimeUnit, truncate, shorten)),
				std::to_string(root->m_timer.getAvgTime(root->m_numComputations, minTimeUnit, truncate, shorten)),
			});
		}

		// Build a list of sorted entries
		std::vector<Computation> sortedComputations(m_computations.begin(root), m_computations.end(root));
		std::sort(sortedComputations.begin(), sortedComputations.end(), [](Computation const& a, Computation const& b)
			{ return a.m_timer.getElapsedTime() > b.m_timer.getElapsedTime(); });

		// Insert an empty line for padding
		if (depth == 0 && !isRoot) result.add_row({ "|", "", "", "" });

		// Generate the sorted entries
		for (auto const& computation : sortedComputations)
		{
			auto treeIt = std::find_if(m_computations.begin(root), m_computations.end(root), 
				[&](auto const& computationIt){ return computationIt.m_name == computation.m_name; });
			collectTimersetTableEntries(minTimeUnit, truncate, shorten, result, depth + (isRoot ? 0 : 1), treeIt);
		}

		// Insert an empty line for padding
		if (depth > 0 && sortedComputations.size() > 0) result.add_row({ "|", "", "", "" });
	}

	////////////////////////////////////////////////////////////////////////////////
	void TimerSet::displaySummary(Debug::DebugOutputLevel logLevel, TimeUnit minTimeUnit, bool truncate, bool shorten) const
	{
		// Tabulate the list of computations
		tabulate::Table table;
		table.add_row({ "Computation", "Num. computations", "Total", "Avg. per Computation" });
		collectTimersetTableEntries(minTimeUnit, truncate, shorten, table, 0, m_computations.begin(m_computations.head));
		
		// Apply formatting to the table
		tabulate::formats::treelike(table);

		Debug::log_output(logLevel) << "Computation timings summary:" << Debug::end;
		Debug::log_output(logLevel) << table << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	const char* timeFormatDisplay()
	{
		static const char* s_format = "%H:%M:%S";
		return s_format;
	}

	////////////////////////////////////////////////////////////////////////////////
	const char* dateFormatFilename()
	{
		static const char* s_format = "%G-%m-%d-%H-%M-%S";
		return s_format;
	};

	////////////////////////////////////////////////////////////////////////////////
	const char* dateFormatDisplay()
	{
		static const char* s_format = "%G. %B %d., %H:%M:%S";
		return s_format;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateStringUtf8(const tm* t, std::string const& format)
	{
		char buffer[128];
		wchar_t wbuffer[128];
		strftime(buffer, ARRAYSIZE(buffer), format.c_str(), t);
		MultiByteToWideChar(CP_ACP, 0, buffer, -1, wbuffer, ARRAYSIZE(wbuffer));
		WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, buffer, ARRAYSIZE(buffer), NULL, NULL);
		return std::string(buffer);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateStringUtf8(time_t t, std::string const& format)
	{
		return getDateStringUtf8(localtime(&t), format);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateStringUtf8(std::string const& format)
	{
		time_t rawtime;
		time(&rawtime);
		return getDateStringUtf8(rawtime, format);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateString(const tm* t, std::string const& format)
	{
		char buffer[128];
		strftime(buffer, ARRAYSIZE(buffer), format.c_str(), t);
		return std::string(buffer);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateString(time_t t, std::string const& format)
	{
		return getDateString(localtime(&t), format);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getDateString(std::string const& format)
	{
		time_t rawtime;
		time(&rawtime);
		return getDateString(rawtime, format);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<TimeUnit, std::string> s_unitTexts =
	{
		{ Years, "years" },
		{ Days, "days" },
		{ Hours, "hours" },
		{ Minutes, "minutes" },
		{ Seconds, "seconds" },
		{ Milliseconds, "milliseconds" },
		{ Microseconds, "microseconds" },
		{ Nanoseconds, "nanoseconds" },
	};

	////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<TimeUnit, std::string> s_unitTextsShort =
	{
		{ Years, "y" },
		{ Days, "d" },
		{ Hours, "h" },
		{ Minutes, "m" },
		{ Seconds, "s" },
		{ Milliseconds, "ms" },
		{ Microseconds, "mus" },
		{ Nanoseconds, "ns" },
	};

	////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<TimeUnit, double> s_unitsInSeconds =
	{
		{ Nanoseconds, 1.0 / 1e9 },
		{ Microseconds, 1.0 / 1e6 },
		{ Milliseconds, 1.0 / 1e3 },
		{ Seconds, 1.0 },
		{ Minutes, 60.0 },
		{ Hours, 60.0 * 60.0 },
		{ Days, 24.0 * 60.0 * 60.0 },
		{ Years, 365.0 * 24.0 * 60.0 * 60.0 },
	};

	////////////////////////////////////////////////////////////////////////////////
	std::string getDeltaTimeFormatted(double delta, const bool truncate, const bool shorten, const TimeUnit minUnit)
	{
		std::stringstream result;
		std::string sep;
		const std::string unitSep = (shorten ? "" : " ");
		bool fullProcessed = false;
		for (int unitId = int(TimeUnit_meta.members.size() - 1); !fullProcessed && unitId != -1; --unitId)
		{
			const TimeUnit unit = TimeUnit(unitId);
			const int wholeUnits = int(delta / s_unitsInSeconds[unit]); // Whole time units
			const bool isLast = unitId == 0 || (unit <= minUnit && wholeUnits > 0);
			fullProcessed |= isLast;
			// Convert from seconds to the current unit
			const double timeUnits = (isLast && !truncate && !shorten) ? delta / s_unitsInSeconds[unit] : double(wholeUnits);
			delta -= wholeUnits * s_unitsInSeconds[unit]; // Remove the units from the delta			
			if (wholeUnits == 0 && !isLast) continue; // Omit empty units
			result << sep << timeUnits << unitSep << (shorten ? s_unitTextsShort[unit] : s_unitTexts[unit]);
			sep = (shorten ? " " : ", ");
		}
		return result.str();
	}
}