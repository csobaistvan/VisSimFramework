#include "PCH.h"
#include "EnginePaths.h"
#include "Config.h"
#include "DateTime.h"

namespace EnginePaths
{
	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path systemFontsFolder()
	{
		static std::filesystem::path s_fontPath = "C:\\Windows\\Fonts";
		return s_fontPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path rootFolder()
	{
		static std::filesystem::path s_rootPath;

		// Locate the root folder on first call
		if (s_rootPath.empty())
		{
			// Start from the current working directory
			s_rootPath = std::filesystem::absolute(std::filesystem::current_path());

			// Use the assets folder as a determinator
			while (std::filesystem::exists(s_rootPath / "Assets") == false)
				s_rootPath = std::filesystem::absolute(s_rootPath.parent_path());
		}

		// Return the cached root path
		return s_rootPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path executableFolder()
	{
		static std::filesystem::path s_executablePath = rootFolder() / "Build" / "Binaries" / Config::AttribValue("config").get<std::string>();
		return s_executablePath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path assetsFolder()
	{
		static std::filesystem::path s_assetsPath = rootFolder() / "Assets";
		return s_assetsPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path generatedFilesFolder()
	{
		static std::filesystem::path s_generatedFilesPath;
		if (s_generatedFilesPath.empty())
		{
			s_generatedFilesPath = assetsFolder() / "Generated";
			makeDirectoryStructure(s_generatedFilesPath);
		}
		return s_generatedFilesPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path configFilesFolder()
	{
		static std::filesystem::path s_configFilesPath;
		if (s_configFilesPath.empty())
		{
			s_configFilesPath = generatedFilesFolder() / "Config";
			makeDirectoryStructure(s_configFilesPath);
		}
		return s_configFilesPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path logFilesFolder()
	{
		static std::filesystem::path s_logFilesPath;
		if (s_logFilesPath.empty())
		{
			s_logFilesPath = generatedFilesFolder() / "Log";
			makeDirectoryStructure(s_logFilesPath);
		}
		return s_logFilesPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path openGlShadersFolder()
	{
		static std::filesystem::path s_path = assetsFolder() / "Shaders" / "OpenGL";
		return s_path;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path generatedOpenGlShadersFolder()
	{
		static std::filesystem::path s_path;
		if (s_path.empty())
		{
			s_path = generatedFilesFolder() / "Shaders" / "OpenGL";
			makeDirectoryStructure(s_path);
		}
		return s_path;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool makeDirectoryStructure(std::filesystem::path path, bool isFile)
	{
		// Extract the parent for regular files
		if (isFile) path = path.parent_path();

		if (std::filesystem::exists(path) == false)
		{
			return std::filesystem::create_directories(path);
		}
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace StringToFileName
	{
		////////////////////////////////////////////////////////////////////////////////
		void removeInvalidChars(std::string& theString)
		{
			std::string_replace_all(theString, " ", "_");
			std::string_erase_all(theString, std::string("()"));
		}

		////////////////////////////////////////////////////////////////////////////////
		void removeUpperCase(std::string& theString)
		{
			bool isFirstChar = true;
			bool isPrevLower = false;

			std::string targetString;
			targetString.reserve(theString.size());
			for (const char ch : theString)
			{
				if (isupper(ch) && !isFirstChar && isPrevLower)
					targetString.push_back('_');

				targetString.push_back(tolower(ch));
				isPrevLower = islower(ch);
				isFirstChar = false;
			}
			theString = targetString;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** String to filename converter. */
	std::string stringToFileName(std::string theString)
	{
		StringToFileName::removeInvalidChars(theString);
		StringToFileName::removeUpperCase(theString);
		return theString;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string generateUniqueFilename(std::string prefix, std::string extension)
	{
		return prefix + DateTime::getDateString(DateTime::dateFormatFilename()) + extension;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string generateUniqueFilepath(std::string prefix, std::string extension)
	{
		return (generatedFilesFolder() / generateUniqueFilename(prefix, extension)).string();
	}
}