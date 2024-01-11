#include "PCH.h"
#include "Units.h"

namespace Units
{
	// Various unit names
	static std::array<std::string, 9> s_unitPrefixes
	{
		"",
		"K",
		"M",
		"G",
		"T",
		"P",
		"E",
		"Z",
		"Y"
	};

	// Various unit names
	static std::array<std::string, 9> s_unitNames
	{
		"Byte",
		"Kilobyte",
		"Megabyte",
		"Gigabyte",
		"Terabyte",
		"Petabyte",
		"Exabyte",
		"Zettabyte",
		"Yottabyte"
	};

	// Various short unit names
	static std::array<std::string, 9> s_shortUnitNames
	{
		"Byte",
		"KByte",
		"MByte",
		"GByte",
		"TByte",
		"PByte",
		"EByte",
		"ZByte",
		"YByte"
	};

	////////////////////////////////////////////////////////////////////////////////
	float bytesToKilobytes(float bytes)
    {
        return bytes / (1024.0f);
    }
    
	////////////////////////////////////////////////////////////////////////////////
	float bytesToMegabytes(float bytes)
    {
        return bytes / (1024.0f * 1024.0f);
    }
    
	////////////////////////////////////////////////////////////////////////////////
	float bytesToGigabytes(float bytes)
    {
        return bytes / (1024.0f * 1024.0f * 1024.0f);
    }
    
	////////////////////////////////////////////////////////////////////////////////
	float bytesToTerrabytes(float bytes)
    {
        return bytes / (1024.0f * 1024.0f * 1024.0f * 1024.0f);
    }

	////////////////////////////////////////////////////////////////////////////////
	float kilobytesToBytes(float kbytes)
    {
        return kbytes * (1024.0f);
    }
    
	////////////////////////////////////////////////////////////////////////////////
	float megabytesToBytes(float mbytes)
    {
        return mbytes * (1024.0f * 1024.0f);
    }
    
	////////////////////////////////////////////////////////////////////////////////
	float gigabytesToBytes(float gbytes)
    {
        return gbytes * (1024.0f * 1024.0f * 1024.0f);
    }
    
	////////////////////////////////////////////////////////////////////////////////
	float terrabytesToBytes(float tbytes)
    {
        return tbytes * (1024.0f * 1024.0f * 1024.0f * 1024.0f);
    }

	////////////////////////////////////////////////////////////////////////////////
	std::pair<float, std::string> normalizeBytes(float bytes)
	{
		int unit = 0;

		while(bytes >= 1024.0f && unit < s_shortUnitNames.size() - 1)
		{ bytes /= 1024.0f; ++unit; }
	
		return std::make_pair(bytes, s_shortUnitNames[unit]);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string bytesToString(float bytes)
	{
		auto [nbytes, unit] = normalizeBytes(bytes);
		std::stringstream ss;
		ss << nbytes << " " << unit;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	/// TIME METRICS
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	double secondsToMilliseconds(double seconds)
	{
		return seconds * 1e3;
	}

	////////////////////////////////////////////////////////////////////////////////
	double secondsToMicroseconds(double seconds)
	{
		return seconds * 1e6;
	}

	////////////////////////////////////////////////////////////////////////////////
	double secondsToNanoseconds(double seconds)
	{
		return seconds * 1e9;
	}

	////////////////////////////////////////////////////////////////////////////////
	double secondsToMinutes(double seconds)
	{
		return seconds / 60.0;
	}

	////////////////////////////////////////////////////////////////////////////////
	double secondsToHours(double seconds)
	{
		return seconds / 3600.0;
	}

	////////////////////////////////////////////////////////////////////////////////
	double secondsToDays(double seconds)
	{
		return seconds / 86400.0;
	}

	////////////////////////////////////////////////////////////////////////////////
	double millisecondsToSeconds(double milli)
	{
		return milli / 1e3;
	}

	////////////////////////////////////////////////////////////////////////////////
	double microsecondsToSeconds(double micro)
	{
		return micro / 1e6;
	}

	////////////////////////////////////////////////////////////////////////////////
	double nanosecondsToSeconds(double nano)
	{
		return nano / 1e9;
	}

	////////////////////////////////////////////////////////////////////////////////
	double minutesToSeconds(double minutes)
	{
		return minutes * 60.0;
	}

	////////////////////////////////////////////////////////////////////////////////
	double hoursToSeconds(double hours)
	{
		return hours * 3600.0;
	}

	////////////////////////////////////////////////////////////////////////////////
	double daysToSeconds(double days)
	{
		return days * 86400.0;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string secondsToString(double seconds)
	{
		double t;

		// Minutes
		t = secondsToMinutes(seconds);
		if (t > 1.0)
		{
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << t << " m";
			return stream.str();
		}

		// Seconds
		t = seconds;
		if (t > 1.0)
		{
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << t << " s";
			return stream.str();
		}

		// Milliseconds
		t = secondsToMilliseconds(seconds);
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << t << " ms";
		return stream.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string minutesToString(double minutes)
	{
		std::stringstream stream;
		stream << std::setfill('0') << std::setw(2) << int(minutes) << ":" << 
			std::setfill('0') << std::setw(2) << int(minutesToSeconds(minutes)) - int(minutesToSeconds(int(minutes)));
		return stream.str();
	}
}