#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Constants.h"

#include "LibraryExtensions/StdEx.h"

////////////////////////////////////////////////////////////////////////////////
/// DEBUG FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace Debug
{
	////////////////////////////////////////////////////////////////////////////////
	meta_enum(DebugOutputLevel, int, Null, Debug, Trace, Info, Warning, Error);

	////////////////////////////////////////////////////////////////////////////////
	/** Forward declaration of the log output object. */
	struct LogOutput;
	struct LogOutputRef;

	////////////////////////////////////////////////////////////////////////////////
	/** A log message source. */
	struct LogMessageSource
	{
		const char* m_file;
		int m_line;
		const char* m_function;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** An entry of the in-memory log buffer. */
	struct InMemoryLogEntry
	{
		static std::vector<std::string> s_componentNames;

		time_t m_dateEpoch;
		std::string m_date;
		std::string m_sourceFile;
		std::string m_sourceLine;
		std::string m_sourceFunction;
		std::string m_sourceLog;
		std::string m_threadId;
		std::string m_region;
		std::string m_message;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** In-memory log buffer. */
	struct InMemoryLogBuffer: public std::stringbuf
	{
		// Constructor
		InMemoryLogBuffer(int bufferSize = 1024);

		// Virtual sync function
		virtual int sync();

		// Advance to the next message
		void advance();

		// Clears the buffer
		void clear();

		// Resize to a new size, while keeping the buffer contents intact
		void resize(int newSize);

		// ==========================

		// Length of the buffer
		int m_bufferLength;

		// Start and end ids
		int m_startMessageId;
		int m_numMessages;

		// Entries
		std::vector<InMemoryLogEntry> m_messages;

		// ==========================

		// Which message we are writing to
		int m_outMessageId;

		// How many messages we have, in total
		int m_numMessagesTotal;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Output device description. */
	struct Device
	{
		// Type of the device
		meta_enum(DeviceType, int, Console, File, InMemory);
		
		// Inits the device with the parameter ostream reference
		Device(std::reference_wrapper<std::ostream> const& stream, DeviceType type);

		// Whether it's a console output or not
		DeviceType m_deviceType;

		// Reference to the output stream
		std::reference_wrapper<std::ostream> m_stream;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Log manipulator signature. */
	typedef LogOutput& (*LogManipulator)(LogOutput&);

	////////////////////////////////////////////////////////////////////////////////
	/** Log reference constructor signature. */
	typedef LogOutputRef& (*LogReferenceConstructor)();

	////////////////////////////////////////////////////////////////////////////////
	/** Log output object. */
	struct LogOutput
	{
		LogOutput(std::string const& name, std::string const& m_colorCode, std::initializer_list<std::reference_wrapper<Device>> const& devices);

		// Name of the output
		std::string m_name;

		// Name of the output (upper case)
		std::string m_nameUpper;

		// Color code of the output
		std::string m_colorCode;

		// Output devices
		std::vector<std::reference_wrapper<Device>> m_devices;
	
		// Internals
		bool m_newLine;

		// For synchronization
		std::mutex m_mutex;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Log output reference object. */
	struct LogOutputRef
	{
		LogOutputRef(LogOutput& output, LogMessageSource const& source);

		operator LogOutput&() const { return m_ref.get(); }

		std::reference_wrapper<LogOutput> m_ref;
		std::unique_lock<std::mutex> m_lock;
		LogMessageSource m_source;
	};

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line prefix */
	LogOutput& putLinePrefix(LogOutput& log, bool newLine = true);

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line suffix */
	LogOutput& putLineSuffix(LogOutput& log, bool newLine = true);

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line source */
	LogOutput& putLineSource(LogOutput& log, LogMessageSource const& source);

	////////////////////////////////////////////////////////////////////////////////
	/** Outputs log messages. */
	template<typename T>
	LogOutput& putMessage(LogOutput& log, T const& val)
	{
		for (auto device : log.m_devices) 
			device.get().m_stream.get() << val;
		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Output operator for string types. */
	template<typename T, typename std::enable_if<std::is_string<T>::value, int>::type = 0>
	LogOutput& operator<< (LogOutput& log, T const& val)
	{
		std::string line = std::string(val);
		std::stringstream ss(line);
		for (int lineNo = 0; std::getline(ss, line); ++lineNo)
		{
			if (lineNo > 0) putLineSuffix(log);
			putLinePrefix(log, true);
			putMessage(log, line);
		}
		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Output operator for log message sources. */
	template<typename T, typename std::enable_if<std::is_same<T, LogMessageSource>::value, int>::type = 0>
	LogOutput & operator<< (LogOutput & log, T const& val)
	{
		return putLineSource(log, val);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Output operator for IO manipulators. */
	template<typename T, typename std::enable_if<std::is_iomanip<T>::value, int>::type = 0>
	LogOutput& operator<< (LogOutput& log, T const& val)
	{
		return putMessage(log, val);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Output operator for other types. */
	template<typename T, typename std::enable_if<!std::is_string<T>::value && !std::is_iomanip<T>::value && !std::is_same<T, LogMessageSource>::value, int>::type = 0>
	LogOutput& operator<< (LogOutput& log, T const& val)
	{
		return log << std::to_string(val);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Output operator with log reference constructors. */
	template<typename T>
	LogOutput& operator<<(LogReferenceConstructor log, T const& val)
	{
		return log() << val;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Logger manipulators. */
	LogOutput& cr(LogOutput& log);
	LogOutput& end(LogOutput& log);

	////////////////////////////////////////////////////////////////////////////////
	/** Manipulator apply operator */
	LogOutput& operator<<(LogOutput& log, LogManipulator manipulator);

	////////////////////////////////////////////////////////////////////////////////
	/** A printf-style message object. */
	struct FormattedMessage
	{
		// Constructs a formatted message with the specified parameters
		template<typename... Ts>
		FormattedMessage(std::string const& format, Ts&&...  parameters):
			FormattedMessage(format.c_str(), std::forward<Ts>(parameters))
		{}

		// Constructs a formatted message with the specified parameters
		template<typename... Ts>
		FormattedMessage(const char* format, const Ts&...  parameters)
		{
			char buffer[128];
			sprintf_s(buffer, sizeof(buffer), format, parameters...);
			m_data = std::string(buffer);
		}
		std::string m_data;
	};
	
	///////////////////////////////////////////////////////
	/** Formatted message apply operator */
	LogOutput& operator<<(LogOutput& log, FormattedMessage const& formattedMessage);

	///////////////////////////////////////////////////////
	/** Current log region. */
	extern std::string s_region[Constants::s_maxThreads];

	////////////////////////////////////////////////////////////////////////////////
	/** A RAII debug region implementation. */
	struct DebugRegion
	{
		DebugRegion(std::string const& regionName, int currentThread = -1);
		DebugRegion(std::vector<std::string> const& regionNames, int currentThread = -1);
		~DebugRegion();

		void enterRegion(std::string const& regionName);

		int m_threadId;
		int m_regionStringLength;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A static logger object with a constant region attached to it */
	struct Logger
	{
		std::vector<std::string> m_regionNames;

		Logger(std::string const& regionName);
		Logger(std::vector<std::string> const& regionNames);
	};

	////////////////////////////////////////////////////////////////////////////////
	// Logger implementation
	namespace logger_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		/** In-memory log buffers. */
		extern InMemoryLogBuffer debug_only_inmemory_buffer;
		extern InMemoryLogBuffer debug_full_inmemory_buffer;
		extern InMemoryLogBuffer trace_only_inmemory_buffer;
		extern InMemoryLogBuffer trace_full_inmemory_buffer;
		extern InMemoryLogBuffer info_only_inmemory_buffer;
		extern InMemoryLogBuffer info_full_inmemory_buffer;
		extern InMemoryLogBuffer warning_only_inmemory_buffer;
		extern InMemoryLogBuffer warning_full_inmemory_buffer;
		extern InMemoryLogBuffer error_only_inmemory_buffer;
		extern InMemoryLogBuffer error_full_inmemory_buffer;

		////////////////////////////////////////////////////////////////////////////////
		/** In-memory log streams. */
		extern std::ostream debug_only_inmemory;
		extern std::ostream debug_full_inmemory;
		extern std::ostream trace_only_inmemory;
		extern std::ostream trace_full_inmemory;
		extern std::ostream info_only_inmemory;
		extern std::ostream info_full_inmemory;
		extern std::ostream warning_only_inmemory;
		extern std::ostream warning_full_inmemory;
		extern std::ostream error_only_inmemory;
		extern std::ostream error_full_inmemory;

		////////////////////////////////////////////////////////////////////////////////
		/** Output files. */
		extern std::ofstream null_file;
		extern std::ofstream debug_only_file;
		extern std::ofstream debug_full_file;
		extern std::ofstream trace_only_file;
		extern std::ofstream trace_full_file;
		extern std::ofstream info_only_file;
		extern std::ofstream info_full_file;
		extern std::ofstream warning_only_file;
		extern std::ofstream warning_full_file;
		extern std::ofstream error_only_file;
		extern std::ofstream error_full_file;

		////////////////////////////////////////////////////////////////////////////////
		/** Output devices. */
		extern Device null_device;
		extern Device debug_only_file_device;
		extern Device debug_only_inmemory_device;
		extern Device debug_full_file_device;
		extern Device debug_full_inmemory_device;
		extern Device trace_only_file_device;
		extern Device trace_only_inmemory_device;
		extern Device trace_full_file_device;
		extern Device trace_full_inmemory_device;
		extern Device info_only_file_device;
		extern Device info_only_inmemory_device;
		extern Device info_full_file_device;
		extern Device info_full_inmemory_device;
		extern Device warning_only_file_device;
		extern Device warning_only_inmemory_device;
		extern Device warning_full_file_device;
		extern Device warning_full_inmemory_device;
		extern Device error_only_file_device;
		extern Device error_only_inmemory_device;
		extern Device error_full_file_device;
		extern Device error_full_inmemory_device;
		extern Device cout_device;
		extern Device cerr_device;

		////////////////////////////////////////////////////////////////////////////////
		/** Log streams. */
		extern LogOutput log_null;
		extern LogOutput log_debug;
		extern LogOutput log_trace;
		extern LogOutput log_info;
		extern LogOutput log_warning;
		extern LogOutput log_error;

		////////////////////////////////////////////////////////////////////////////////
		/** Log streams. */
		LogOutput& make_log_output_from_level(DebugOutputLevel level);
		LogOutputRef make_log_from_level(DebugOutputLevel output, const char* file, int line, const char* function);
		LogOutputRef make_log_from_output(LogOutput& output, const char* file, int line, const char* function);
		LogOutputRef make_log_null(const char* file, int line, const char* function);
		LogOutputRef make_log_debug(const char* file, int line, const char* function);
		LogOutputRef make_log_trace(const char* file, int line, const char* function);
		LogOutputRef make_log_info(const char* file, int line, const char* function);
		LogOutputRef make_log_warning(const char* file, int line, const char* function);
		LogOutputRef make_log_error(const char* file, int line, const char* function);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Log output constructors
	#define log_output(OUTPUT) logger_impl::make_log_from_level(OUTPUT, __FILE__, __LINE__, __FUNCTION__)
	#define log_null() logger_impl::make_log_null(__FILE__, __LINE__, __FUNCTION__)
	#define log_debug() logger_impl::make_log_debug(__FILE__, __LINE__, __FUNCTION__)
	#define log_trace() logger_impl::make_log_trace(__FILE__, __LINE__, __FUNCTION__)
	#define log_info() logger_impl::make_log_info(__FILE__, __LINE__, __FUNCTION__)
	#define log_warning() logger_impl::make_log_warning(__FILE__, __LINE__, __FUNCTION__)
	#define log_error() logger_impl::make_log_error(__FILE__, __LINE__, __FUNCTION__)

	////////////////////////////////////////////////////////////////////////////////
	// Which channels to log into for a specific log target
	struct LogChannels
	{
		bool m_debug = true;
		bool m_trace = true;
		bool m_info = true;
		bool m_warning = true;
		bool m_error = true;
	};

	////////////////////////////////////////////////////////////////////////////////
	LogChannels defaultLogChannelsMemory();

	////////////////////////////////////////////////////////////////////////////////
	LogChannels defaultLogChannelsConsole();

	////////////////////////////////////////////////////////////////////////////////
	LogChannels defaultLogChannelsFile();

	////////////////////////////////////////////////////////////////////////////////
	void setMemoryLogging(LogChannels const& channels);

	////////////////////////////////////////////////////////////////////////////////
	void setFileLogging(LogChannels const& channels);

	////////////////////////////////////////////////////////////////////////////////
	void setConsoleLogging(LogChannels const& channels);

	////////////////////////////////////////////////////////////////////////////////
	void initLogDevices();

	////////////////////////////////////////////////////////////////////////////////
	std::string formatText(std::string str);

	////////////////////////////////////////////////////////////////////////////////
	/* A generic assert function that logs a message onto the specified logger if the assert condition is not met. */
	#define logged_assert(CONDITION, LOG, ...) \
	if ((CONDITION) == false) \
	{ \
		auto logFn = Debug::logger_impl::CONCAT(make_log_, LOG); \
		logFn(__FILE__, __LINE__, __FUNCTION__) << Debug::FormattedMessage(__VA_ARGS__) << Debug::end; \
		exit(-1); \
	} \

	////////////////////////////////////////////////////////////////////////////////
	/** Function precondition. */
	#define expects(CONDITION, ...) logged_assert((CONDITION), warning, __VA_ARGS__)

	////////////////////////////////////////////////////////////////////////////////
	/** Function postcondition*/
	#define ensures(CONDITION, ...) logged_assert((CONDITION), warning, __VA_ARGS__)

	////////////////////////////////////////////////////////////////////////////////
	#define assert(CONDITION) logged_assert((CONDITION), error, "Assertion failed: {:}", #CONDITION)

	////////////////////////////////////////////////////////////////////////////////
	void printSystemInfo();
}
