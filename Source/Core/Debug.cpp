#include "PCH.h"
#include "Debug.h"
#include "Config.h"
#include "EnginePaths.h"
#include "DateTime.h"
#include "GPU.h"
#include "StaticInitializer.h"
#include "Threading.h"

namespace Debug
{
	////////////////////////////////////////////////////////////////////////////////
	// Init the log devices
	static bool s_initDone = false;

	////////////////////////////////////////////////////////////////////////////////
	std::string s_region[Constants::s_maxThreads];

	////////////////////////////////////////////////////////////////////////////////
	size_t enterLogRegion(std::string regionName, size_t threadId)
	{
		regionName = "["s + regionName + "]"s;
		Threading::invoke_for_threads(threadId, [&regionName](int threadId)
		{
			s_region[threadId] += regionName;
		});
		return regionName.length();
	}

	////////////////////////////////////////////////////////////////////////////////
	void leaveLogRegion(size_t regionNameLength, size_t threadId)
	{
		Threading::invoke_for_threads(threadId, [regionNameLength](int threadId)
		{
			if (s_region[threadId].length() >= regionNameLength)
				s_region[threadId] = s_region[threadId].substr(0, s_region[threadId].length() - regionNameLength);
			else
				s_region[threadId].clear();
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	DebugRegion::DebugRegion(std::string const& regionName, int currentThread):
		m_regionStringLength(0),
		m_threadId(currentThread == -1 ? Threading::numThreads() : currentThread)
	{
		enterRegion(regionName);
	}

	////////////////////////////////////////////////////////////////////////////////
	DebugRegion::DebugRegion(std::vector<std::string> const& regionNames, int currentThread) :
		m_regionStringLength(0),
		m_threadId(currentThread == -1 ? Threading::numThreads() : currentThread)
	{
		for (auto region : regionNames)
			enterRegion(region);
	}

	////////////////////////////////////////////////////////////////////////////////
	DebugRegion::~DebugRegion()
	{
		leaveLogRegion(m_regionStringLength, m_threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	void DebugRegion::enterRegion(std::string const& regionName)
	{
		m_regionStringLength += enterLogRegion(regionName, m_threadId);
	}

	////////////////////////////////////////////////////////////////////////////////
	Logger::Logger(std::string const& regionName):
		m_regionNames(1, regionName)
	{}

	////////////////////////////////////////////////////////////////////////////////
	Logger::Logger(std::vector<std::string> const& regionNames):
		m_regionNames(regionNames)
	{}

	////////////////////////////////////////////////////////////////////////////////
	int getInMemoryBufferSize()
	{
		return 1 << 16;
	}

	////////////////////////////////////////////////////////////////////////////////
	InMemoryLogBuffer::InMemoryLogBuffer(int bufferSize):
		m_bufferLength(bufferSize),
		m_startMessageId(0),
		m_numMessages(0),
		m_outMessageId(0),
		m_numMessagesTotal(0),
		m_messages(bufferSize)
	{}

	////////////////////////////////////////////////////////////////////////////////
	int InMemoryLogBuffer::sync()
	{
		// Append to the output message
		m_messages[m_outMessageId].m_message += str();

		// Flush the buffer
		str("");

		// Success
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Resize to a new size, while keeping the buffer contents intact
	void InMemoryLogBuffer::resize(int newSize)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void InMemoryLogBuffer::clear()
	{
		m_startMessageId = 0;
		m_numMessages = 0;
		m_outMessageId = 0;
		m_numMessagesTotal = 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	void InMemoryLogBuffer::advance()
	{
		// Flush the remaining contents
		pubsync();

		// Increment the total message counter
		++m_numMessagesTotal;

		// Advance to the next message
		m_outMessageId = m_numMessagesTotal % m_bufferLength;

		// Clear the current message
		m_messages[m_outMessageId].m_message.clear();

		// Increment the number of messages
		m_numMessages = std::min(m_numMessagesTotal, m_bufferLength - 1);

		// Advance the start message id
		//m_startMessageId = m_numMessagesTotal < m_bufferLength ? 0 : m_numMessagesTotal % (m_bufferLength - 1);
		m_startMessageId = (m_numMessagesTotal - m_numMessages) % m_bufferLength;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<std::string> InMemoryLogEntry::s_componentNames =
	{
		"Source",
		"Region",
		"Date",
		"Function",
		"File",
		"Line",
		"Message"
	};

	namespace logger_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		/** In-memory log buffers. */
		InMemoryLogBuffer trace_only_inmemory_buffer   { getInMemoryBufferSize() };
		InMemoryLogBuffer trace_full_inmemory_buffer   { getInMemoryBufferSize() };
		InMemoryLogBuffer debug_only_inmemory_buffer   { getInMemoryBufferSize() };
		InMemoryLogBuffer debug_full_inmemory_buffer   { getInMemoryBufferSize() };
		InMemoryLogBuffer info_only_inmemory_buffer    { getInMemoryBufferSize() };
		InMemoryLogBuffer info_full_inmemory_buffer    { getInMemoryBufferSize() };
		InMemoryLogBuffer warning_only_inmemory_buffer { getInMemoryBufferSize() };
		InMemoryLogBuffer warning_full_inmemory_buffer { getInMemoryBufferSize() };
		InMemoryLogBuffer error_only_inmemory_buffer   { getInMemoryBufferSize() };
		InMemoryLogBuffer error_full_inmemory_buffer   { getInMemoryBufferSize() };

		////////////////////////////////////////////////////////////////////////////////
		/** In-memory log streams. */
		std::ostream trace_only_inmemory   { &trace_only_inmemory_buffer };
		std::ostream trace_full_inmemory   { &trace_full_inmemory_buffer };
		std::ostream debug_only_inmemory   { &debug_only_inmemory_buffer };
		std::ostream debug_full_inmemory   { &debug_full_inmemory_buffer };
		std::ostream info_only_inmemory    { &info_only_inmemory_buffer };
		std::ostream info_full_inmemory    { &info_full_inmemory_buffer };
		std::ostream warning_only_inmemory { &warning_only_inmemory_buffer };
		std::ostream warning_full_inmemory { &warning_full_inmemory_buffer };
		std::ostream error_only_inmemory   { &error_only_inmemory_buffer };
		std::ostream error_full_inmemory   { &error_full_inmemory_buffer };

		////////////////////////////////////////////////////////////////////////////////
		/** Output files. */
		std::ofstream null_file;
		std::ofstream trace_only_file   = std::ofstream(EnginePaths::logFilesFolder() / "trace_only.txt");
		std::ofstream trace_full_file   = std::ofstream(EnginePaths::logFilesFolder() / "trace_full.txt");
		std::ofstream debug_only_file   = std::ofstream(EnginePaths::logFilesFolder() / "debug_only.txt");
		std::ofstream debug_full_file   = std::ofstream(EnginePaths::logFilesFolder() / "debug_full.txt");
		std::ofstream info_only_file    = std::ofstream(EnginePaths::logFilesFolder() / "info_only.txt");
		std::ofstream info_full_file    = std::ofstream(EnginePaths::logFilesFolder() / "info_full.txt");
		std::ofstream warning_only_file = std::ofstream(EnginePaths::logFilesFolder() / "warning_only.txt");
		std::ofstream warning_full_file = std::ofstream(EnginePaths::logFilesFolder() / "warning_full.txt");
		std::ofstream error_only_file   = std::ofstream(EnginePaths::logFilesFolder() / "error_only.txt");
		std::ofstream error_full_file   = std::ofstream(EnginePaths::logFilesFolder() / "error_full.txt");

		////////////////////////////////////////////////////////////////////////////////
		/** Null devices. */
		Device null_device                   { std::ref(null_file),             Device::File };
		Device trace_only_file_device        { std::ref(trace_only_file),       Device::File };
		Device trace_only_inmemory_device    { std::ref(trace_only_inmemory),   Device::InMemory };
		Device trace_full_file_device        { std::ref(trace_full_file),       Device::File };
		Device trace_full_inmemory_device    { std::ref(trace_full_inmemory),   Device::InMemory };
		Device debug_only_file_device        { std::ref(debug_only_file),       Device::File };
		Device debug_only_inmemory_device    { std::ref(debug_only_inmemory),   Device::InMemory };
		Device debug_full_file_device        { std::ref(debug_full_file),       Device::File };
		Device debug_full_inmemory_device    { std::ref(debug_full_inmemory),   Device::InMemory };
		Device info_only_file_device         { std::ref(info_only_file),        Device::File };
		Device info_only_inmemory_device     { std::ref(info_only_inmemory),    Device::InMemory };
		Device info_full_file_device         { std::ref(info_full_file),        Device::File };
		Device info_full_inmemory_device     { std::ref(info_full_inmemory),    Device::InMemory };
		Device warning_only_file_device      { std::ref(warning_only_file),     Device::File };
		Device warning_only_inmemory_device  { std::ref(warning_only_inmemory), Device::InMemory };
		Device warning_full_file_device      { std::ref(warning_full_file),     Device::File };
		Device warning_full_inmemory_device  { std::ref(warning_full_inmemory), Device::InMemory };
		Device error_only_file_device        { std::ref(error_only_file),       Device::File };
		Device error_only_inmemory_device    { std::ref(error_only_inmemory),   Device::InMemory };
		Device error_full_file_device        { std::ref(error_full_file),       Device::File };
		Device error_full_inmemory_device    { std::ref(error_full_inmemory),   Device::InMemory };
		Device cout_device                   { std::ref(std::cout),             Device::Console };
		Device cerr_device                   { std::ref(std::cerr),             Device::Console };

		////////////////////////////////////////////////////////////////////////////////
		/** Log streams. */
		LogOutput log_null
		{
			// name
			"Null", 
			// color
			"0",
			// devices
			{}
		};

		LogOutput log_debug
		{
			// name
			"Debug",
			// color
			"90",
			// devices
			{
				// file
				std::ref(debug_only_file_device), 
				std::ref(debug_full_file_device),
				std::ref(debug_only_inmemory_device), 
				std::ref(debug_full_inmemory_device),
			} // devices
		};

		LogOutput log_trace
		{
			// name
			"Trace",
			// color
			"94",
			// devices
			{
				// file
				std::ref(debug_full_file_device), 
				std::ref(trace_only_file_device), 
				std::ref(trace_full_file_device),
				// in-memory
				std::ref(debug_full_inmemory_device), 
				std::ref(trace_only_inmemory_device), 
				std::ref(trace_full_inmemory_device),
			} // devices
		};

		LogOutput log_info
		{
			// name
			"Info",
			// color
			"97",
			// devices
			{
				// file
				std::ref(trace_full_file_device), 
				std::ref(debug_full_file_device), 
				std::ref(info_only_file_device), 
				std::ref(info_full_file_device),
				// in-memory
				std::ref(trace_full_inmemory_device), 
				std::ref(debug_full_inmemory_device), 
				std::ref(info_only_inmemory_device), 
				std::ref(info_full_inmemory_device),
			} // devices
		};

		LogOutput log_warning
		{
			// name
			"Warning",
			// color
			"93",
			// devices
			{
				// file
				std::ref(trace_full_file_device), 
				std::ref(debug_full_file_device), 
				std::ref(info_full_file_device), 
				std::ref(warning_only_file_device), 
				std::ref(warning_full_file_device),
				// in-memory
				std::ref(trace_full_inmemory_device), 
				std::ref(debug_full_inmemory_device), 
				std::ref(info_full_inmemory_device), 
				std::ref(warning_only_inmemory_device), 
				std::ref(warning_full_inmemory_device),
			} // devices
		};

		LogOutput log_error
		{
			// name
			"Error",
			// color
			"91",
			// devices
			{
				// file
				std::ref(trace_full_file_device), 
				std::ref(debug_full_file_device), 
				std::ref(info_full_file_device), 
				std::ref(warning_full_file_device), 
				std::ref(error_only_file_device), 
				std::ref(error_full_file_device),
				// in-memory
				std::ref(trace_full_inmemory_device), 
				std::ref(debug_full_inmemory_device), 
				std::ref(info_full_inmemory_device), 
				std::ref(warning_full_inmemory_device), 
				std::ref(error_only_inmemory_device), 
				std::ref(error_full_inmemory_device)
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		/** Log streams. */
		LogOutput& make_log_output_from_level(DebugOutputLevel level)
		{
			switch (level)
			{
				case Debug:   return log_debug;
				case Trace:   return log_trace;
				case Info:    return log_info;
				case Warning: return log_warning;
				case Error:   return log_error;
				default:      return log_null;
			}
			return log_null;
		}
		LogOutputRef make_log_from_output(LogOutput& output, const char* file, int line, const char* function)
		{
			// Perform on-demand init
			if (!s_initDone)
			{
				s_initDone = true;
				initLogDevices();
			}
			return LogOutputRef(output, LogMessageSource{ file, line, function });
		}
		LogOutputRef make_log_from_level(DebugOutputLevel level, const char* file, int line, const char* function)
		{
			return make_log_from_output(make_log_output_from_level(level), file, line, function);
		}
		LogOutputRef make_log_null(const char* file, int line, const char* function)
		{
			return make_log_from_output(log_null, file, line, function);
		}
		LogOutputRef make_log_debug(const char* file, int line, const char* function)
		{
			return make_log_from_output(log_debug, file, line, function);
		}
		LogOutputRef make_log_trace(const char* file, int line, const char* function)
		{
			return make_log_from_output(log_trace, file, line, function);
		}
		LogOutputRef make_log_info(const char* file, int line, const char* function)
		{
			return make_log_from_output(log_info, file, line, function);
		}
		LogOutputRef make_log_warning(const char* file, int line, const char* function)
		{
			return make_log_from_output(log_warning, file, line, function);
		}
		LogOutputRef make_log_error(const char* file, int line, const char* function)
		{
			return make_log_from_output(log_error, file, line, function);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Output device description. */
	Device::Device(std::reference_wrapper<std::ostream> const& stream, Device::DeviceType type) :
		m_deviceType(type),
		m_stream(stream)
	{}

	////////////////////////////////////////////////////////////////////////////////
	/** Log output object. */
    LogOutput::LogOutput(std::string const& name, std::string const& m_colorCode, std::initializer_list<std::reference_wrapper<Device>> const& devices) :
        m_name(name),
		m_nameUpper(name),
        m_colorCode(m_colorCode),
        m_devices(devices.begin(), devices.end()),
        m_newLine(true)
    {
		std::transform(m_nameUpper.begin(), m_nameUpper.end(), m_nameUpper.begin(), [](unsigned char ch) { return std::toupper(ch); });
	}

	////////////////////////////////////////////////////////////////////////////////
	LogOutputRef::LogOutputRef(LogOutput& output, LogMessageSource const& source) :
		m_ref(output),
		m_lock(output.m_mutex),
		m_source(source)
	{
		output << source;
	}

	////////////////////////////////////////////////////////////////////////////////
	void putColorCodeSuffix(Device& device, std::string const& colorCode)
	{
		switch (device.m_deviceType)
		{
		case Device::Console:
			device.m_stream.get() << "\033[m";
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLinePrefixConsole(Device& device, LogOutput& log, bool newLine)
	{
		std::ostream& stream = device.m_stream.get();
		stream << "\033[" << log.m_colorCode << "m";
		stream << "[" << DateTime::getDateStringUtf8(DateTime::timeFormatDisplay()) << "]";
		//stream << "[" << Threading::currentThreadId() << "]";
		//stream << "[" << log.m_nameUpper << "]";
		stream << s_region[Threading::currentThreadId()];
		stream << ": ";
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLinePrefixFile(Device& device, LogOutput& log, bool newLine)
	{
		std::ostream& stream = device.m_stream.get();
		stream << "[" << DateTime::getDateStringUtf8(DateTime::timeFormatDisplay()) << "]";
		stream << "[" << Threading::currentThreadId() << "]";
		stream << "[" << log.m_nameUpper << "]";
		stream << s_region[Threading::currentThreadId()];
		stream << ": ";
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLinePrefixInMemory(Device& device, LogOutput& log, bool newLine)
	{
		InMemoryLogBuffer* buffer = (InMemoryLogBuffer*)(device.m_stream.get().rdbuf());
		InMemoryLogEntry* entry = &buffer->m_messages[buffer->m_outMessageId];

		entry->m_dateEpoch = time(nullptr);
		entry->m_date = DateTime::getDateStringUtf8(DateTime::dateFormatDisplay());
		entry->m_sourceLog = log.m_name;
		entry->m_threadId = std::to_string(Threading::currentThreadId());
		entry->m_region = s_region[Threading::currentThreadId()];
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLinePrefix(Device& device, LogOutput& log, bool newLine)
	{
		switch (device.m_deviceType)
		{
		case Device::Console:  putLinePrefixConsole(device, log, newLine); break;
		case Device::File:     putLinePrefixFile(device, log, newLine); break;
		case Device::InMemory: putLinePrefixInMemory(device, log, newLine); break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line prefix */
	LogOutput& putLinePrefix(LogOutput& log, bool newLine)
	{
		// Write out the logger name and the color code prefix
		if (log.m_newLine)
		{
			for (auto const& device : log.m_devices)
				putLinePrefix(device, log, newLine);
			log.m_newLine = false;
		}

		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSuffixConsole(Device& device, LogOutput& log, bool newLine)
	{
		device.m_stream.get() << "\033[0m";
		if (newLine) device.m_stream.get() << std::endl;
		else         device.m_stream.get() << "\r";
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSuffixFile(Device& device, LogOutput& log, bool newLine)
	{
		if (newLine) device.m_stream.get() << std::endl;
		else         device.m_stream.get() << "\r";
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSuffixInMemory(Device& device, LogOutput& log, bool newLine)
	{
		InMemoryLogBuffer* buffer = (InMemoryLogBuffer*)(device.m_stream.get().rdbuf());
		if (newLine) buffer->advance();
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSuffix(Device& device, LogOutput& log, bool newLine)
	{
		switch (device.m_deviceType)
		{
		case Device::Console:  putLineSuffixConsole(device, log, newLine); break;
		case Device::File:     putLineSuffixFile(device, log, newLine); break;
		case Device::InMemory: putLineSuffixInMemory(device, log, newLine); break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line suffix */
	LogOutput& putLineSuffix(LogOutput& log, bool newLine)
	{
		for (auto device : log.m_devices)
			putLineSuffix(device, log, newLine);
		log.m_newLine = true;

		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSourceConsole(Device& device, LogMessageSource const& source)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSourceInMemory(Device& device, LogMessageSource const& source)
	{
		InMemoryLogBuffer* buffer = (InMemoryLogBuffer*)(device.m_stream.get().rdbuf());
		InMemoryLogEntry* entry = &buffer->m_messages[buffer->m_outMessageId];

		entry->m_sourceFile = source.m_file;
		entry->m_sourceLine = std::to_string(source.m_line);
		entry->m_sourceFunction = source.m_function;
	}

	////////////////////////////////////////////////////////////////////////////////
	void putLineSourceFile(Device& device, LogMessageSource const& source)
	{}

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line source */
	void putLineSource(Device& device, LogMessageSource const& source)
	{
		switch (device.m_deviceType)
		{
		case Device::Console:  putLineSourceConsole(device, source); break;
		case Device::File:     putLineSourceFile(device, source); break;
		case Device::InMemory: putLineSourceInMemory(device, source); break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/**  Writes out a log line source */
	LogOutput& putLineSource(LogOutput& log, LogMessageSource const& source)
	{
		for (auto device : log.m_devices)
			putLineSource(device, source);

		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	LogOutput& cr(LogOutput& log)
	{
		putLineSuffix(log, false);
		putLinePrefix(log, false);
		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	LogOutput& end(LogOutput& log)
	{
		putLineSuffix(log, true);
		return log;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Manipulator apply operator */
	LogOutput& operator<<(LogOutput& log, LogManipulator manipulator)
	{
		return manipulator(log);
	}
	
	///////////////////////////////////////////////////////
	/** Manipulator apply operator */
	LogOutput& operator<<(LogOutput& log, FormattedMessage const& formattedMessage)
	{
		return log << formattedMessage.m_data;
	}

	///////////////////////////////////////////////////////
	// Helper for finding the iterator to a device
	auto findDevice(LogOutput const& log, std::reference_wrapper<Device> const& paramDevice)
	{
		return std::find_if(log.m_devices.begin(), log.m_devices.end(), 
			[&](std::reference_wrapper<Device> const& device)
			{ 
				return std::addressof(device.get().m_stream.get()) == std::addressof(paramDevice.get().m_stream.get()); 
			});
	}

	////////////////////////////////////////////////////////////////////////////////
	void enableLogDevice(bool enabled, LogOutput& outputRef, std::reference_wrapper<Device> const& paramDevice)
	{
		auto deviceRef = findDevice(outputRef, paramDevice);

		if (enabled && deviceRef == outputRef.m_devices.end())
		{
			outputRef.m_devices.push_back(paramDevice);
		}
		else if (enabled == false && deviceRef != outputRef.m_devices.end())
		{
			outputRef.m_devices.erase(deviceRef);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void setMemoryLogging(LogChannels const& channels)
	{
		enableLogDevice(channels.m_debug, logger_impl::log_debug, std::ref(logger_impl::debug_only_inmemory_device));
		enableLogDevice(channels.m_debug, logger_impl::log_debug, std::ref(logger_impl::debug_full_inmemory_device));

		enableLogDevice(channels.m_trace, logger_impl::log_trace, std::ref(logger_impl::debug_full_inmemory_device));
		enableLogDevice(channels.m_trace, logger_impl::log_trace, std::ref(logger_impl::trace_only_inmemory_device));
		enableLogDevice(channels.m_trace, logger_impl::log_trace, std::ref(logger_impl::trace_full_inmemory_device));

		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::debug_full_inmemory_device));
		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::trace_full_inmemory_device));
		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::info_only_inmemory_device));
		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::info_full_inmemory_device));

		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::debug_full_inmemory_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::trace_full_inmemory_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::info_full_inmemory_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::warning_only_inmemory_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::warning_full_inmemory_device));

		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::debug_full_inmemory_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::trace_full_inmemory_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::info_full_inmemory_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::warning_full_inmemory_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::error_only_inmemory_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::error_full_inmemory_device));
	}

	////////////////////////////////////////////////////////////////////////////////
	void setFileLogging(LogChannels const& channels)
	{
		enableLogDevice(channels.m_debug, logger_impl::log_debug, std::ref(logger_impl::debug_only_file_device));
		enableLogDevice(channels.m_debug, logger_impl::log_debug, std::ref(logger_impl::debug_full_file_device));

		enableLogDevice(channels.m_trace, logger_impl::log_trace, std::ref(logger_impl::debug_full_file_device));
		enableLogDevice(channels.m_trace, logger_impl::log_trace, std::ref(logger_impl::trace_only_file_device));
		enableLogDevice(channels.m_trace, logger_impl::log_trace, std::ref(logger_impl::trace_full_file_device));

		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::debug_full_file_device));
		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::trace_full_file_device));
		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::info_only_file_device));
		enableLogDevice(channels.m_info, logger_impl::log_info, std::ref(logger_impl::info_full_file_device));

		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::debug_full_file_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::trace_full_file_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::info_full_file_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::warning_only_file_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::warning_full_file_device));

		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::debug_full_file_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::trace_full_file_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::info_full_file_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::warning_full_file_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::error_only_file_device));
		enableLogDevice(channels.m_error, logger_impl::log_error, std::ref(logger_impl::error_full_file_device));
	}

	////////////////////////////////////////////////////////////////////////////////
	void setConsoleLogging(LogChannels const& channels)
	{
		enableLogDevice(channels.m_debug,   logger_impl::log_debug,   std::ref(logger_impl::cout_device));
		enableLogDevice(channels.m_trace,   logger_impl::log_trace,   std::ref(logger_impl::cout_device));
		enableLogDevice(channels.m_info,    logger_impl::log_info,    std::ref(logger_impl::cout_device));
		enableLogDevice(channels.m_warning, logger_impl::log_warning, std::ref(logger_impl::cout_device));
		enableLogDevice(channels.m_error,   logger_impl::log_error,   std::ref(logger_impl::cerr_device));
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string formatText(std::string str)
	{
		// List of all format strings
		static std::map<std::string, std::function<std::string()>> s_formatStrings =
		{
			// current date
			{ "$date$", []() { return DateTime::getDateString(DateTime::dateFormatFilename()); } }
		};

		// Go through each format string
		for (auto const& formatStr : s_formatStrings)
		{
			size_t start;
			while ((start = str.find(formatStr.first)) != std::string::npos)
			{
				std::string replacement = formatStr.second();
				str.replace(start, formatStr.first.size(), replacement);
			}
		}

		// Return the result
		return str;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Is the param logger enabled for memory output. */
	bool memoryLogEnabled(LogOutputRef const& output)
	{
		return Config::AttribValue("log_memory").contains(output.m_ref.get().m_name);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Is the param logger enabled for console output. */
	bool consoleLogEnabled(LogOutputRef const& output)
	{
		return Config::AttribValue("log_console").contains(output.m_ref.get().m_name);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Is the param logger enabled for file output. */
	bool fileLogEnabled(LogOutputRef const& output)
	{
		return Config::AttribValue("log_file").contains(output.m_ref.get().m_name);
	}

	////////////////////////////////////////////////////////////////////////////////
	LogChannels defaultLogChannelsMemory()
	{
		LogChannels memoryChannels;
		memoryChannels.m_debug = memoryLogEnabled(log_debug());
		memoryChannels.m_trace = memoryLogEnabled(log_trace());
		memoryChannels.m_info = memoryLogEnabled(log_info());
		memoryChannels.m_warning = memoryLogEnabled(log_warning());
		memoryChannels.m_error = memoryLogEnabled(log_error());
		return memoryChannels;
	}

	////////////////////////////////////////////////////////////////////////////////
	LogChannels defaultLogChannelsConsole()
	{
		LogChannels stdoutChannels;
		stdoutChannels.m_debug = consoleLogEnabled(log_debug());
		stdoutChannels.m_trace = consoleLogEnabled(log_trace());
		stdoutChannels.m_info = consoleLogEnabled(log_info());
		stdoutChannels.m_warning = consoleLogEnabled(log_warning());
		stdoutChannels.m_error = consoleLogEnabled(log_error());
		return stdoutChannels;
	}

	////////////////////////////////////////////////////////////////////////////////
	LogChannels defaultLogChannelsFile()
	{
		LogChannels fileChannels;
		fileChannels.m_debug = fileLogEnabled(log_debug());
		fileChannels.m_trace = fileLogEnabled(log_trace());
		fileChannels.m_info = fileLogEnabled(log_info());
		fileChannels.m_warning = fileLogEnabled(log_warning());
		fileChannels.m_error = fileLogEnabled(log_error());
		return fileChannels;
	}

	////////////////////////////////////////////////////////////////////////////////
	void initLogDevices()
	{
		// Set the localte for the consoles
		std::cout.imbue(std::locale("en_US.UTF-8"));
		std::cerr.imbue(std::locale("en_US.UTF-8"));

		// Set default log levels
		LogChannels memoryChannels;
		memoryChannels.m_debug = true;
		memoryChannels.m_trace = true;
		memoryChannels.m_info = true;
		memoryChannels.m_warning = true;
		memoryChannels.m_error = true;
		setMemoryLogging(memoryChannels);

		LogChannels consoleChannels;
		consoleChannels.m_debug = false;
		consoleChannels.m_trace = false;
		consoleChannels.m_info = true;
		consoleChannels.m_warning = true;
		consoleChannels.m_error = true;
		setConsoleLogging(consoleChannels);

		LogChannels fileChannels;
		memoryChannels.m_debug = true;
		memoryChannels.m_trace = true;
		memoryChannels.m_info = true;
		memoryChannels.m_warning = true;
		memoryChannels.m_error = true;
		setFileLogging(fileChannels);
	}

	////////////////////////////////////////////////////////////////////////////////
	void printSystemInfo()
	{
		// Extract the current display params
		const GLFWvidmode* displayParams = glfwGetVideoMode(glfwGetPrimaryMonitor());

		// Print some debug info
		Debug::log_debug() << std::string(80, '=') << Debug::end;
		Debug::log_debug() << "Config" << Debug::end;
		Config::printValues();
		Debug::log_debug() << Debug::end;

		// - System
		Debug::log_debug() << "System" << Debug::end;
		Debug::log_debug() << "  - Hardware concurrency: " << std::thread::hardware_concurrency() << Debug::end;
		Debug::log_debug() << "  - Number of threads: " << Threading::numThreads() << Debug::end;
		Debug::log_debug() << Debug::end;

		// - OpenGL
		Debug::log_debug() << "OpenGL" << Debug::end;
		Debug::log_debug() << "  - Version: " << glGetString(GL_VERSION) << Debug::end;
		Debug::log_debug() << "  - GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << Debug::end;
		Debug::log_debug() << "  - Vendor: " << glGetString(GL_VENDOR) << Debug::end;
		Debug::log_debug() << "  - Renderer: " << glGetString(GL_RENDERER) << Debug::end;
		Debug::log_debug() << Debug::end;

		// - Window context
		Debug::log_debug() << "Context" << Debug::end;
		Debug::log_debug() << "  - Resolution: " << displayParams->width << "x" << displayParams->height << " (" << displayParams->refreshRate << " hz)" << Debug::end;
		Debug::log_debug() << "  - Format: " << displayParams->redBits << displayParams->greenBits << displayParams->blueBits << Debug::end;
		Debug::log_debug() << Debug::end;

		// - GPU capabilities
		Debug::log_debug() << "GPU Capabilities" << Debug::end;
		for (auto const& capabilityCategoryIt : GPU::s_capabilityAttribs)
		{
			auto const& [category, capabilities] = capabilityCategoryIt;

			Debug::log_debug() << "    " << category << Debug::end;
			
			for (auto const& capabilityIt : capabilities)
			{
				auto const& [capabilityId, capability] = capabilityIt;

				Debug::log_debug() << "      - " << GPU::enumToString(capability.m_enum) << ": " << capability.m_values << Debug::end;
			}
		}
		Debug::log_debug() << Debug::end;

		// - Tensorflow
		#ifdef HAS_TensorFlow
		Debug::log_debug() << "TensorFlow" << Debug::end;
		Debug::log_debug() << "  - Version: " << TF_Version() << Debug::end;
		Debug::log_debug() << Debug::end;
		#endif

		// - Eigen
		Debug::log_debug() << "Eigen" << Debug::end;
		Debug::log_debug() << "  - Number of threads: " << Eigen::nbThreads() << Debug::end;
		Debug::log_debug() << Debug::end;

		Debug::log_debug() << std::string(80, '=') << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(Log, Memory, -log_memory, Debug, Trace, Info, Warning, Error)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"log_memory", "Logging",
			"Enable memory logging for a certain level.",
			"LEVEL", { "Info", "Warning", "Error" },
			{
				{ "Debug", "Debug level" },
				{ "Trace", "Trace level" },
				{ "Info", "Info level" },
				{ "Warning", "Warning level" },
				{ "Error", "Error level" },
			},
			Config::attribRegexString()
		});

		// @CONSOLE_VAR(Log, Console, -log_console, Debug, Trace, Info, Warning, Error)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"log_console", "Logging",
			"Enable console output for a certain level.",
			"LEVEL", { "Info", "Warning", "Error" },
			{
				{ "Debug", "Debug level" },
				{ "Trace", "Trace level" },
				{ "Info", "Info level" },
				{ "Warning", "Warning level" },
				{ "Error", "Error level" },
			},
			Config::attribRegexString()
		});

		// @CONSOLE_VAR(Log, File, -log_file, Debug, Trace, Info, Warning, Error)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"log_file", "Logging",
			"Enable file output for a certain log level.",
			"LEVEL",  { "Debug", "Info", "Warning", "Error" },
			{
				{ "Debug", "Debug level" },
				{ "Trace", "Trace level" },
				{ "Info", "Info level" },
				{ "Warning", "Warning level" },
				{ "Error", "Error level" },
			},
			Config::attribRegexString()
		});

		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"config", "Application",
			"Current build config.",
			"CONFIG", { CONFIG }, {},
			Config::attribRegexString()
		});
	};
}