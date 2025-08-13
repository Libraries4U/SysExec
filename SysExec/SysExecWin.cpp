#include "SysExec.h"

#ifndef PLATFORM_POSIX

#include "ArgEnv.h"

#include "ShellLib.h"
#include <process.h>

#define LLOG(x) // RLOG(x)

#include <fcntl.h>

NAMESPACE_UPP

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command, passing a command line to it and gathering the output
// from both stdout and stderr
bool SysExec(String const &command, String const &argLine, const VectorMap<String, String> &envMap, String &outStr, String &errStr)
{
	bool res = false;
	outStr.Clear();
	errStr.Clear();

	HANDLE hStdOutRead, hStdOutWrite;
	HANDLE hStdErrRead, hStdErrWrite;
	SECURITY_ATTRIBUTES sa{sizeof(sa), NULL, TRUE};

	// Create pipes for stdout and stderr
	if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))
		return false;
	if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0))
	{
		CloseHandle(hStdOutWrite);
		return false;
	}

	// Ensure the read handles are not inherited
	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdErrWrite;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	PROCESS_INFORMATION pi{};


	// build  command line
	String commandLine = "\"" + command + "\" " + argLine;

	// build environment
	String envBuf;
	for (auto &[key, val] : ~envMap)
	{
		envBuf += key + "=" + val;
		envBuf.Cat(0);
	}
	envBuf.Cat(0);

	res = CreateProcessA(nullptr,					  // lpApplicationName
								  ~StringBuffer(commandLine), // lpCommandLine (mutable buffer)
								  nullptr,					  // lpProcessAttributes
								  nullptr,					  // lpThreadAttributes
								  TRUE,						  // bInheritHandles
								  0,						  // dwCreationFlags
								  ~StringBuffer(envBuf),	  // lpEnvironment
								  nullptr,					  // lpCurrentDirectory
								  &si,						  // lpStartupInfo
								  &pi						  // lpProcessInformation
	);

	// Close write ends in parent
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);
	
	if(!res)
		return false;

	char buffer[4096];

	for (;;)
	{
		DWORD outAvail = 0, errAvail = 0;

		// Check available data
		PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &outAvail, NULL);
		PeekNamedPipe(hStdErrRead, NULL, 0, NULL, &errAvail, NULL);

		// Read stdout if available
		if (outAvail > 0)
		{
			DWORD bytesRead = 0;
			if (::ReadFile(hStdOutRead, buffer, sizeof(buffer) < outAvail ? sizeof(buffer) : outAvail, &bytesRead, NULL) && bytesRead > 0)
			{
				outStr.Cat(buffer, bytesRead);
			}
		}

		// Read stderr if available
		if (errAvail > 0)
		{
			DWORD bytesRead = 0;
			if (ReadFile(hStdErrRead, buffer, sizeof(buffer) < errAvail ? sizeof(buffer) : errAvail, &bytesRead, NULL) && bytesRead > 0)
			{
				errStr.Cat(buffer, bytesRead);
			}
		}

		// Check process state
		DWORD waitRes = WaitForSingleObject(pi.hProcess, 50); // short timeout
		if (waitRes == WAIT_OBJECT_0)
		{
			// Drain remaining data after process exit
			while (true)
			{
				BOOL any = FALSE;
				DWORD bytesRead;
				if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &outAvail, NULL) && outAvail > 0)
				{
					if (ReadFile(hStdOutRead, buffer, sizeof(buffer) < outAvail ? sizeof(buffer) : outAvail, &bytesRead, NULL) && bytesRead > 0)
					{
						outStr.Cat(buffer, bytesRead);
						any = TRUE;
					}
				}
				if (PeekNamedPipe(hStdErrRead, NULL, 0, NULL, &errAvail, NULL) && errAvail > 0)
				{
					if (ReadFile(hStdErrRead, buffer, sizeof(buffer) < errAvail ? sizeof(buffer) : errAvail, &bytesRead, NULL) && bytesRead > 0)
					{
						errStr.Cat(buffer, bytesRead);
						any = TRUE;
					}
				}
				if (!any)
					break;
			}
			break;
		}
	}

	DWORD exitCode = 0;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command, passing a command line to it without waiting for its termination
// optionally returns pid of started process
bool SysStart(String const &command, String const &argLine, const VectorMap<String, String> &envMap, intptr_t *pid)
{
	intptr_t result = -1;

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// build  command line
	String commandLine = "\"" + command + "\" " + argLine;

	Cerr() << "CommandLine:" << commandLine << "\n";

	// build environment
	String envBuf;
	for (auto &[key, val] : ~envMap)
	{
		envBuf += key + "=" + val;
		envBuf.Cat(0);
	}
	envBuf.Cat(0);

	BOOL success = CreateProcessA(nullptr,					  // lpApplicationName
								  ~StringBuffer(commandLine), // lpCommandLine (mutable buffer)
								  nullptr,					  // lpProcessAttributes
								  nullptr,					  // lpThreadAttributes
								  FALSE,					  // bInheritHandles
								  0,						  // dwCreationFlags
								  ~StringBuffer(envBuf),	  // lpEnvironment
								  nullptr,					  // lpCurrentDirectory
								  &si,						  // lpStartupInfo
								  &pi						  // lpProcessInformation
	);
	if (success)
		result = (intptr_t)pi.hProcess;
	else
		result = -1;

	if (pid)
		*pid = result;

	if (result == -1)
		Cerr() << "Error spawning process\n";

	return (result != -1 ? true : false);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as Admin user, passing a command line to it without waiting for its termination
// it WILL prompt for user intervention on secure systems (linux - Vista+ OSes)
// return true on success, false otherwise
bool SysStartAdmin(String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	// on windows, no pass should be needed, it'll display the dialog automatically
	if (IsUserAdministrator())
		return SysStart(command, argLine, envMap);
	else
		return ShellExec(command + " " + argLine, envMap, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as Admin user, passing a command line to it waiting for its termination
// it WILL prompt for user intervention on secure systems (linux - Vista+ OSes)
// return true on success, false otherwise
bool SysExecAdmin(String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	// on windows, no pass should be needed, it'll display the dialog automatically
	if (IsUserAdministrator())
		return SysExec(command, argLine, envMap);
	else
		return ShellExec(command + " " + argLine, envMap, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as required user, passing a command line to it without waiting for its termination
// on linux, will return an error if password is required AND wrong
// on windows, by now... it just spawn the process without changing security level
// I still shall find a way to go back to user mode on windows
// return true on success, false otherwise
bool SysStartUser(String const &user, String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	return ShellExecUser(command, argLine, envMap, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as required user, passing a command line to it waiting for its termination
// on linux, will return an error if password is required AND wrong
// on windows, by now... it just spawn the process without changing security level
// I still shall find a way to go back to user mode on windows
// return true on success, false otherwise
bool SysExecUser(String const &user, String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	return ShellExecUser(command, argLine, envMap, true);
}

END_UPP_NAMESPACE

#endif
