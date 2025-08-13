#include <Core/Core.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
	if(CommandLine().GetCount() < 1)
	{
		Cerr() << "Need at least 1 argument\n";
		Exit(1);
	}
	
	if(CommandLine()[0] == "SysStart")
	{
		if(CommandLine().GetCount() < 2)
		{
			Cerr() << "Need at least 2 arguments\n";
			Exit(1);
		}
		String fName = CommandLine()[1];
		FileOut out(fName);
		
		out << "Checking SysStart -- inside child\n";
		out << "-------------------------------------------------------------\n";
		out << CommandLine()[0] << " check\n";
		out << "This goes to out file\n";
		out << "CommandLine:\n";
		for(int iArg = 0; iArg < CommandLine().GetCount(); iArg++)
		{
			out << FormatIntDec(iArg, 3) << ":" << CommandLine()[iArg] << "\n";
		}
		out << "\nEnvironment:\n";
		for(int iEnv = 0; iEnv < Environment().GetCount(); iEnv++)
		{
			out << FormatIntDec(iEnv, 3) << ":" << Environment().GetKey(iEnv) << "=" << Environment()[iEnv] << "\n";
		}
		out.Close();
	}
	else
	{
		// SysExec check
		Cerr() << "Checking SysExec -- inside child\n";
		Cerr() << "-------------------------------------------------------------\n";
		Cerr() << CommandLine()[0] << " check\n";
		Cerr() << "This goes to stderr\n";
		Cout() << "This goes to stdout\n";
		Cerr() << "CommandLine:\n";
		for(int iArg = 0; iArg < CommandLine().GetCount(); iArg++)
		{
			Cerr() << FormatIntDec(iArg, 3) << ":" << CommandLine()[iArg] << "\n";
		}
		Cerr() << "\nEnvironment:\n";
		for(int iEnv = 0; iEnv < Environment().GetCount(); iEnv++)
		{
			Cerr() << FormatIntDec(iEnv, 3) << ":" << Environment().GetKey(iEnv) << "=" << Environment()[iEnv] << "\n";
		}
	}
}
