#include <Core/Core.h>

#include <SysExec/SysExec.h>

#include <fcntl.h>
#include <stdio.h>

#ifdef PLATFORM_POSIX
#include <sys/wait.h>
#endif


using namespace Upp;

CONSOLE_APP_MAIN
{
	bool res;

	VectorMap<String, String> myEnv(Environment(), 1);
	myEnv.Add("PIPPO") = "PLUTO";
	
	String path = GetExeFilePath();
#ifdef PLATFORM_POSIX
	path = AppendFileName(GetFileFolder(path), "SysExecTestChild");
#else
	path = AppendFileName(GetFileFolder(path), "SysExecTestChild.exe");
#endif
	

///////////////////////////////////////////////////////////////////////////////////////////////

	Cerr() << "//////////////////////////////////////////////////////////\n";
	Cerr() << "\nTesting SysExec\n";

	String out, err;
	res = SysExec(path, "SysExec arg2 \"arg 3\"", myEnv, out, err);
	
	if(res)
	{
		Cerr() << "SysExec() success\n";
		Cerr() << "From stdout:" << out << "\n";
		Cerr() << "From stderr:" << err << "\n";
	}
	else
	{
		Cerr() << "SysExec failed\n";
		Cerr() << "ErrorCode is:" << GetLastError() << "\n";
	}

///////////////////////////////////////////////////////////////////////////////////////////////

	
	Cerr() << "\n\n\n//////////////////////////////////////////////////////////\n";
	Cerr() << "\nTesting SysStart\n";

	String fName = GetTempFileName();
	Cerr() << "fName:" << fName << "\n";

	intptr_t pid = 0;
	res = SysStart(path, "SysStart " + fName + " arg3 \"arg 4\"", myEnv, &pid);
	if(res)
	{
		Cerr() << "SysStart() success, waiting for process to end\n";
#ifdef PLATFORM_POSIX
		int status;
		wait(&status);
#else
		WaitForSingleObject((HANDLE)pid, INFINITE);
#endif
		Cerr() << "Done waiting\n";
#ifdef PLATFORM_POSIX
		int outFile = open(~fName, O_RDONLY, S_IREAD);
#else
		int outFile = _open(~fName, _O_RDONLY | _O_TEXT, _S_IREAD);
#endif

		String outStr;
		int outSize = lseek(outFile, 0L, SEEK_END);
		if (outSize > 0)
		{
			char *buf = (char *)malloc(outSize + 1);
			lseek(outFile, 0L, SEEK_SET);
			(void)read(outFile, buf, outSize);
			buf[outSize] = 0;
			outStr.Cat(buf);
			free(buf);
		}
		close(outFile);
		::remove(fName);
		Cerr() << "Output from child:\n" << outStr << "<n";

	}
	else
	{
		Cerr() << "SysExec failed\n";
		Cerr() << "ErrorCode is:" << GetLastError() << "\n";
	}

}
