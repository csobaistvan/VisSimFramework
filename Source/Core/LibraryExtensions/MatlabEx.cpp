#include "PCH.h"
#include "MatlabEx.h"
#include "Core/Includes.h"

namespace Matlab
{
	#ifdef HAS_Matlab
	////////////////////////////////////////////////////////////////////////////////
	std::unique_ptr<matlab::engine::MATLABEngine> g_matlab;

	////////////////////////////////////////////////////////////////////////////////
	bool matlabEnabled()
	{
		static bool s_enabled = Config::AttribValue("matlab").get<std::string>() != "Disable";
		return s_enabled;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool attachToInstance()
	{
		static bool s_enabled = Config::AttribValue("matlab").get<std::string>() == "External";
		return s_enabled;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Register the scripts inside our matlab folder
	void registerScripts()
	{
		// Create a path for the custom matlab scripts
		auto scriptsFolderPath = stringToDataArray((EnginePaths::assetsFolder() / "Scripts" / "Matlab").string());

		// Generate path names for all the script folders
		matlab::data::CharArray matlabPaths = g_matlab->feval("genpath", { scriptsFolderPath }, logged_buffer(Debug::Debug), logged_buffer(Debug::Error));
		
		// Add them to the path
		g_matlab->feval("addpath", { matlabPaths }, logged_buffer(Debug::Debug), logged_buffer(Debug::Error));

		// Change the working folder to the Matlab script folder
		g_matlab->feval("cd", { scriptsFolderPath }, logged_buffer(Debug::Debug), logged_buffer(Debug::Error));
	}

	////////////////////////////////////////////////////////////////////////////////
	void testInstance()
	{
		// Test it with the MATLAB sqrt function 
		try
		{
			auto argArray = eigenToDataArray(Eigen::RowVector4d{ -2.0, 2.0, 6.0, 8.0 });
			auto results = Matlab::g_matlab->feval(u"sqrt", argArray, logged_buffer(Debug::Debug), logged_buffer(Debug::Error));

			// Compare results
			for (int i = 0; i < results.getNumberOfElements(); i++)
			{
				double arg = argArray[i];
				std::complex<double> val = results[i];

				Debug::log_debug() << "sqrt(" << arg << ") = " << val << Debug::end;
				assert(std::norm(std::sqrt(std::complex<double>(arg)) - val) < 1e-3);
			}
		}
		catch (...)
		{
			Debug::log_error() << "Something went wrong while testing sqrt!" << Debug::end;
		}

		// Now try to cull a custom script
		try
		{
			auto x = Matlab::scalarToDataArray(10.0f);
			auto y = Matlab::eigenToDataArray(Eigen::RowVector2f{ 1, 2 });
			auto result = g_matlab->feval("test_cpp", { x, y }, logged_buffer(Debug::Debug), logged_buffer(Debug::Error));
			
			// Compare results
			float origX = x[0];
			float resultX = result[0];
			assert(x.getNumberOfElements() == result.getNumberOfElements());
			assert(resultX == origX);
		}
		catch (...)
		{
			Debug::log_error() << "Something went wrong; unable to call custom script!" << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void initInstance()
	{
		// Early out if disabled
		if (matlabEnabled() == false)
		{
			Debug::log_trace() << "Matlab is disabled, skipping initialization." << Debug::end;
			return;
		}

		// Try to find a running Matlab instance and attach ourselves to it
		bool attached = false;
		if (attachToInstance())
		{
			try
			{
				Debug::log_trace() << "Attempting to attach to a running Matlab instance..." << Debug::end;
				Matlab::g_matlab = matlab::engine::connectMATLAB();
				attached = true;
				Debug::log_trace() << "Successfully attached to a running Matlab instance." << Debug::end;
			}
			catch (...)
			{
				Debug::log_trace() << "Unable to attach to an existing Matlab instance! Falling back to spawning a new one." << Debug::end;
			}
		}

		// Spawn a new child process if we couldn't
		if (attached == false)
		{
			Debug::log_trace() << "Spawning a new Matlab instance..." << Debug::end;
			try
			{
				Matlab::g_matlab = matlab::engine::startMATLAB({});
				Debug::log_trace() << "Matlab instance successfully spawned!" << Debug::end;
			}
			catch (...)
			{
				Debug::log_error() << "Unable to spawn new Matlab instance-running without Matlab!" << Debug::end;
				Debug::log_error() << "Note: Attaching to existing instances was " << (attachToInstance() ? "enabled" : "disabled") << Debug::end;
			}
		}

		// Register the scripts inside our matlab folder
		registerScripts();

		// Test our instance
		testInstance();
	}

	////////////////////////////////////////////////////////////////////////////////
	LoggedStreamBuffer::LoggedStreamBuffer(Debug::DebugOutputLevel logLevel, const char* file, int line, const char* function) :
		m_logLevel(logLevel),
		m_file(file),
		m_line(line),
		m_function(function)
	{}

	////////////////////////////////////////////////////////////////////////////////
	std::streamsize LoggedStreamBuffer::xsputn(const char16_t* s, std::streamsize count)
	{		
		// Log out the message
		m_buffer.append(std::string(matlab::engine::convertUTF16StringToUTF8String(std::u16string(s))));

		// Split the message to individual lines
		if (size_t offset = m_buffer.find_last_of('\n'); offset != std::string::npos)
		{
			std::string printContents = m_buffer.substr(0, offset);
			std::stringstream ss(printContents);
			std::string line;
			for (int lineNo = 0; std::getline(ss, line); ++lineNo)
			{
				Debug::logger_impl::make_log_from_level(m_logLevel, m_file, m_line, m_function) << line << Debug::end;
			}
			m_buffer = m_buffer.substr(offset + 1);
			return offset;
		}

		// Nothing written
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<LoggedStreamBuffer> make_logged_buffer(Debug::DebugOutputLevel logLevel, const char* file, int line, const char* function)
	{ 
		return std::make_shared<LoggedStreamBuffer>(logLevel, file, line, function);
	};
	#endif

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(External, Matlab, -matlab, Disable, Internal, External)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"matlab", "External",
			"How to use Matlab.",
			"", { "Disable" },
			{
				{ "Disable", "Disable MATLAB" },
				{ "Internal", "Create our own instance" },
				{ "External", "Attach to an existing instance, or create our own"}
			},
			Config::attribRegexString()
		});
	};
}