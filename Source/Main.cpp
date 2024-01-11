////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"

#include "Demo/Demo.h"

////////////////////////////////////////////////////////////////////////////////
//  MAIN SIMULATION
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
	// Simply print the usage and leave if we are in info mode
	if (Config::AttribValue("info").get<std::string>() != "Off")
	{
		Config::printUsage();
		return 0;
	}

	// Perform the initialization
	{
		Debug::DebugRegion region({ "Initialization" });
		
		Context::init();
		GPU::init();
		Demo::init();
		Debug::printSystemInfo();
	}

	// Perform the main loop
	{
		Debug::DebugRegion region({ "MainLoop" });

		Demo::mainLoop();
	}

	// Perform cleanup
	{
		Debug::DebugRegion region({ "CleanUp" });

		Demo::cleanup();
		Context::cleanup();
	}

	return 0;
}
