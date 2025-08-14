#include "SysInfo.h"

#ifdef PLATFORM_WIN32

#include <TlHelp32.h>
#include <Psapi.h>

NAMESPACE_UPP

#define LLOG(x) RLOG(x)

////////////////////////////////////////////////////////////////////////////////////
// utility function to see if running Vista or newer OSs
bool IsVistaOrLater(void)
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	return (osvi.dwMajorVersion >= 6);
}

////////////////////////////////////////////////////////////////////////////////////
// utility function to see if running Xp or newer OSs
bool IsXpOrLater(void)
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	return (osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1));
}

#ifdef __MINGW32__
extern "C" BOOL WINAPI CheckTokenMembership(HANDLE, PSID, PBOOL);
#endif

static BOOL IsGroupMember(DWORD dwRelativeID, BOOL bProcessRelative, BOOL *pIsMember)
{
	HANDLE hToken, hDupToken;
	PSID pSid = NULL;
	SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

	if (!pIsMember)
	{
		SetLastError(ERROR_INVALID_USER_BUFFER);
		return FALSE;
	}

	if (bProcessRelative || !OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE, TRUE, &hToken))
	{
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hToken))
			return FALSE;
	}

	if (!DuplicateToken(hToken, SecurityIdentification, &hDupToken))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	hToken = hDupToken;

	if (!AllocateAndInitializeSid(&SidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, dwRelativeID, 0, 0, 0, 0, 0, 0, &pSid))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	if (!CheckTokenMembership(hToken, pSid, pIsMember))
	{
		CloseHandle(hToken);
		FreeSid(pSid);

		*pIsMember = FALSE;
		return FALSE;
	}

	CloseHandle(hToken);
	FreeSid(pSid);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
// utility functions to check whether an app is running in elevated mode
bool IsUserAdministrator(void)
{
	BOOL isAdmin;

	// always an admin for XP and previous versions
	if (!IsVistaOrLater())
		return true;

	// check if running in admin mode for Vista or newers
	IsGroupMember(DOMAIN_ALIAS_RID_ADMINS, FALSE, &isAdmin);
	return isAdmin;
}


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

////////////////////////////////////////////////////////////////////////////////////
// get process name by id
String GetProcessName(dword pid)
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
// get processes id given process name
Vector<dword> GetProcessesIdsByName(String const &targetName)
{
	Vector<dword> pids;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return pids;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	if (Process32First(hSnap, &pe))
	{
		do
		{
			if (targetName == pe.szExeFile)
				pids.Add(pe.th32ProcessID);
		} while (Process32Next(hSnap, &pe));
	}
	CloseHandle(hSnap);
	return pids;
}

END_UPP_NAMESPACE

#endif
