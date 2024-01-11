#include "PCH.h"
#include "Config.h"
#include "Debug.h"
#include "StaticInitializer.h"

namespace Config
{
	////////////////////////////////////////////////////////////////////////////////
	std::string attribRegex(std::string const& value, int count, std::string const& sep, std::string const& open, std::string const& close)
	{
		std::stringstream ss;
		ss << "[[:space:]]+";
		ss << open;
		for (int i = 0; i < count; ++i)
		{
			if (i > 0) ss << sep;
			ss << "(" << value << ")";
		}
		ss << close;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string attribRegexBool()
	{
		return attribRegex("[[:digit:]]+");
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string attribRegexInt(int numComponents)
	{
		return attribRegex("[[:digit:]]+", numComponents, 
			"[[:space:]]*[\\.,;:xX]?[[:space:]]*", "[\\(\\[\\{]?", "[\\)\\]\\}]?");
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string attribRegexFloat(int numComponents)
	{
		return attribRegex("[[:digit:]]+\\.[[:digit:]]+", numComponents,
			"[[:space:]]*[\\.,;:xX]?[[:space:]]*", "[\\(\\[\\{]?", "[\\)\\]\\}]?");
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string attribRegexString()
	{
		return attribRegex("[[:alnum:]_]+");
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Map of all the config value tokens */
	std::vector<AttributeDescriptor> s_descriptors;

	////////////////////////////////////////////////////////////////////////////////
	void registerConfigAttribute(AttributeDescriptor const& descriptor)
	{
		s_descriptors.push_back(descriptor);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Map of all the config values. */
	std::unordered_map<std::string, std::vector<std::vector<std::string>>> s_attribValues;

	////////////////////////////////////////////////////////////////////////////////
	// Constructor.
	AttribValue::AttribValue(std::string const& name) :
		m_name(name),
		m_values(s_attribValues)
	{
		if (s_attribValues.empty())
			init();
	}

	////////////////////////////////////////////////////////////////////////////////
	// Test whether the value has any value assigned to it
	bool AttribValue::any() const
	{
		return m_values.get().find(m_name) != m_values.get().end() && m_values.get()[m_name].size() > 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Return the number of values held
	size_t AttribValue::count() const
	{
		return (!any()) ? 0 : m_values.get()[m_name].size();
	}

	////////////////////////////////////////////////////////////////////////////////
	void printUsage()
	{
		Debug::log_info() << "Available command line arguments: " << Debug::end;
		
		// Determine the max length of the attribute name
		size_t maxNameLength = 0;
		for (auto const& attribute : s_descriptors)
		{
			size_t nameLength = 4 + 1 + attribute.m_name.length() + 2 + 1 + attribute.m_valueName.length();
			maxNameLength = std::max(nameLength, maxNameLength);
		}

		// Log the attributes
		std::string prevCategory;
		for (auto const& attribute: s_descriptors)
		{
			// Print the category header
			if (attribute.m_category != prevCategory)
			{
				if (prevCategory.empty() == false) Debug::log_info() << " " << Debug::end;
				prevCategory = attribute.m_category;
				Debug::log_info() << "OPTIONS - " << attribute.m_category << Debug::end;
				Debug::log_info() << std::string(strlen("OPTIONS") + 3 + attribute.m_category.length(), '-') << Debug::end;
				Debug::log_info() << " " << Debug::end;
			}

			// Length of this variable's name
			size_t nameLength = 4 + 1 + attribute.m_name.length() + (attribute.m_valueName.empty() ? 0 : 1) + attribute.m_valueName.length();

			Debug::log_info() 
				<< std::string(4, ' ') << '-' << attribute.m_name // name
				<< (attribute.m_valueName.empty() ? "" : "=") << attribute.m_valueName // value
				<< std::string(maxNameLength - nameLength + 1, ' ') << attribute.m_description // description
				<< (attribute.m_choices.empty() ? "" : " One of:")
				<< Debug::end;

			// Log the default value
			if (attribute.m_choices.empty())
			{
				if (attribute.m_default.size() > 1)
					Debug::log_info() << std::string(10, ' ') << "default: " << attribute.m_default << Debug::end;
				else
					Debug::log_info() << std::string(10, ' ') << "default: " << attribute.m_default[0] << Debug::end;
			}

			// Log the list of options
			else
			{
				for (auto const& option : attribute.m_choices)
				{
					// Extract the name and the description
					auto const& [name, description] = option;

					// Length of the option name
					size_t nameLength = 10 + name.length();

					// Check if the value is on by default or not
					bool isDefault = false;
					for (auto const& defaultVal : attribute.m_default)
						isDefault |= (name == defaultVal);

					// Log the option
					Debug::log_info()
						<< std::string(9, ' ') << (isDefault ? '*' : ' ') << name
						<< std::string(maxNameLength - nameLength + 3, ' ') << description
						<< Debug::end;
				}
			}
		}
		Debug::log_info() << " " << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void printValues()
	{
		for (auto const& attrib : s_attribValues)
			Debug::log_debug() << "  - " << attrib.first << ": " << attrib.second << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	void matchConfigValue(AttributeDescriptor const& token, std::string const& target)
	{
		std::string pattern = "-" + token.m_name + token.m_regex;
		std::regex matcher = std::regex(pattern);
		std::string::const_iterator searchStart(target.cbegin());
		std::smatch capture;

		// Extract all matches
		while (std::regex_search(searchStart, target.cend(), capture, matcher))
		{
			std::vector<std::string> matches(std::vector<std::string>(capture.begin() + 1, capture.end()));
			s_attribValues[token.m_name].push_back(matches);
			searchStart += capture.position() + capture.length();
			Debug::log_debug() << "attrib '" << token.m_name << "' matched, value(s): '" << matches << "'" << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Extract all the config values. */
	void init()
	{
		Debug::log_debug() << std::string(128, '=') << Debug::end;
		Debug::log_debug() << "Processing command line arguments..." << Debug::end;

		// Construct the full command line argument list
		std::string fullArgList = GetCommandLineA();

		Debug::log_debug() << "Full command line command: '" << fullArgList << "'" << Debug::end;

		// Extract the remainder of the config values
		for (auto const& token : s_descriptors)
		{
			std::regex pattern = std::regex("-" + token.m_name + token.m_regex);
			std::string::const_iterator searchStart(fullArgList.cbegin());
			std::smatch capture;

			// Extract all matches
			matchConfigValue(token, fullArgList);

			// Assign default values
			if (s_attribValues[token.m_name].empty())
			{
				Debug::log_debug() << "attrib '" << token.m_name << "' used default value" << Debug::end;

				for (auto const& defaultVal : token.m_default)
				{
					std::string defaultDefinition = "-" + token.m_name + " " + defaultVal;
					matchConfigValue(token, defaultDefinition);
				}
			}
		}
		Debug::log_debug() << std::string(128, '=') << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"info", "Application",
			"Whether to stop after printing out the startup info.",
			"", { "Off" }, {},
			attribRegexString()
		});
	};
}