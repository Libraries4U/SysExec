#include "SysInfo.h"

#ifdef PLATFORM_POSIX

#include <filesystem>

NAMESPACE_UPP

////////////////////////////////////////////////////////////////////////////////////
// find which process uses a dll
// requires elevated rights to run

static bool ProcessUsesDLL(dword pid, const String &libName)
{
	String mapsPath = "/proc/" + std::to_string(pid) + "/maps";
	FileIn maps(mapsPath);
	if(!maps)
		return false;

	String line;
	while( (line = maps.GetLine()) != "")
	{
		if(line.Find(libName) >=0)
			return true;
	}
	return false;
}

VectorMap<dword, String> ProcessesUsingDll(String DLLName)
{
	VectorMap<dword, String> res;
	
	for(auto& entry : std::filesystem::directory_iterator("/proc"))
	{
		const auto& name = entry.path().filename().string();
		if(std::all_of(name.begin(), name.end(), ::isdigit))
		{
			int pid = std::stoi(name);
			if(ProcessUsesDLL(pid, DLLName))
				res.Add(pid);
		}
	}

	return res;
}

////////////////////////////////////////////////////////////////////////////////////
// get process name by id
String GetProcessName(dword pid)
{
	String path = "/proc/" + FormatInt(pid) + "/comm";
	FileIn f(path);
	return f.GetLine();
}

////////////////////////////////////////////////////////////////////////////////////
// get processes id given process name
Vector<dword> GetProcessesIdsByName(String const& name)
{
	Vector<dword> pids;
	DIR* dir = opendir("/proc");
	if(!dir)
		return pids;

	struct dirent* entry;
	while((entry = readdir(dir)) != nullptr) {
		if(entry->d_type == DT_DIR) {
			String dname = entry->d_name;
			if(std::all_of(dname.begin(), dname.end(), ::isdigit)) {
				int pid = atoi(dname);
				String commPath = "/proc/" + dname + "/comm";
				FileIn commFile(commPath);
				if(commFile) {
					String procName = commFile.GetLine();
					if(procName == name)
						pids.Add(pid);
				}
			}
		}
	}
	return pids;
}

END_UPP_NAMESPACE

#endif
