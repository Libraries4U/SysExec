#include "SysInfo.h"

#ifdef PLATFORM_WIN32

#include <iostream>
#include <string>
#include <tchar.h>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>

#include <algorithm>
#include <iostream>
#include <psapi.h>
#include <string>
#include <tchar.h>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>

NAMESPACE_UPP

#define LLOG(x) RLOG(x)

static bool ProcessUsesDLL(dword pid, const String &targetDllLower)
{
	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
		return false;

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(me32);

	if (Module32First(hModuleSnap, &me32))
	{
		do
		{
			String mod = me32.szModule;
			String path = me32.szExePath;

			mod = ToLower(mod);
			path = ToLower(path);

			if(mod.Find(targetDllLower) >= 0 || path.Find(targetDllLower) >= 0)
			{
				CloseHandle(hModuleSnap);
				return true;
			}

		} while (Module32Next(hModuleSnap, &me32));
	}

	CloseHandle(hModuleSnap);
	return false;
}

static String GetProcessName(dword pid)
{
	String name = "(unknown)";
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess)
	{
		char buffer[MAX_PATH];
		if (GetModuleFileNameExA(hProcess, NULL, buffer, MAX_PATH))
		{
			name = buffer;
		}
		CloseHandle(hProcess);
	}
	return name;
}

////////////////////////////////////////////////////////////////////////////////////
// find which process uses a dll
// requires elevated rights to run
VectorMap<dword, String> ProcessesUsingDll(String DLLName)
{
	VectorMap<dword, String> res;

	DLLName = ToLower(DLLName);

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		LLOG("CreateToolhelp32Snapshot Error");
		return res;
	}

	if (Process32First(hSnap, &pe32))
	{
		do
		{
			DWORD pid = pe32.th32ProcessID;
			if (ProcessUsesDLL(pid, DLLName))
			{
				res.Add(pid) = GetProcessName(pid);
			}
		} while (Process32Next(hSnap, &pe32));
	}

	CloseHandle(hSnap);
	return res;
}

END_UPP_NAMESPACE

#endif
