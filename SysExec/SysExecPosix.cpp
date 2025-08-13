#include "SysExec.h"
#include "ArgEnv.h"

#ifdef PLATFORM_POSIX

#include <unistd.h>
#include <sys/wait.h>
#include "SudoLib.h"

#define LLOG(x) RLOG(x)

NAMESPACE_UPP

// replacement of tmpfile() -- has problems in windows
static FILE *TempFile(void)
{
	String fName = GetTempFileName();
	FILE *f = fopen(fName, "w+");
	return f;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command, passing a command line to it and gathering the output
// from both stdout and stderr
bool SysExec(String const &command, String const &argLine, const VectorMap<String, String> &envMap, String &outStr, String &errStr)
{
	// flushes both stdout and stderr files
	fflush(stdout);
	fflush(stderr);

	outStr.Clear();
	errStr.Clear();

	// saves stdout and stderr stream states
	int saveStdout = dup(fileno(stdout));
	int saveStderr = dup(fileno(stderr));

	// creates and opens 2 temporary files and assign stdout and stderr to them
	int OutFile = fileno(TempFile());
	int ErrFile = fileno(TempFile());
	dup2(OutFile, 1);
	dup2(ErrFile, 2);

	// builds the arguments and the environment
	char **argv = BuildArgs(command, argLine);
	char **envv = BuildEnv(envMap);
	
	// executes the command
	bool res = true;
	pid_t pID = fork();
	if (pID == 0)
	{
		// Code only executed by child process

		// exec svn, function shall not return if all ok
		execvpe(command, argv, envv);
		
		// flushes both stdout and stderr files
		fflush(stdout);
		fflush(stderr);
	
		// restores stdout and stderr handles
		dup2(saveStdout, 1);
		dup2(saveStderr, 2);

		// closes files
		close(OutFile);
		close(ErrFile);

		// if it returns couldn't execute
		LLOG("Couldn't execute command");
		_exit(255);
	}
	if (pID < 0)
	{
		// code executed only if couldn't fork

		// flushes both stdout and stderr files
		fflush(stdout);
		fflush(stderr);
	
		// restores stdout and stderr handles
		dup2(saveStdout, 1);
		dup2(saveStderr, 2);

		LLOG("Couldn't fork");
		res = false;
	}
	if (pID > 0)
	{
		// Code only executed by parent process

		// waits for parent to terminate
		int status = -1;
		int result;
		wait(&status);

		if (WIFEXITED(status))
		{
			result = WEXITSTATUS(status);
			LLOG("The process ended with exit(" << result << ")");
		}
		if (WIFSIGNALED(status))
		{
			result = WTERMSIG(status);
			LLOG("The process ended with kill -" << result);
		}

		if(result == 255)
			res = false;

		// flushes both stdout and stderr files
		fflush(stdout);
		fflush(stderr);
	
		// restores stdout and stderr handles
		dup2(saveStdout, 1);
		dup2(saveStderr, 2);
	}

	if(res)
	{
		// allocates char buffers and reads out end err data into them
		int OutSize = lseek(OutFile, 0L, SEEK_END);
		if (OutSize > 0)
		{
			char *buf = (char *)malloc(OutSize + 1);
			lseek(OutFile, 0L, SEEK_SET);
			/*int dummy = */ (void)read(OutFile, buf, OutSize);
			buf[OutSize] = 0;
			outStr.Cat(buf);
			free(buf);
		}
	
		int ErrSize = lseek(ErrFile, 0L, SEEK_END);
		if (ErrSize > 0)
		{
			char *buf = (char *)malloc(ErrSize + 1);
			lseek(ErrFile, 0L, SEEK_SET);
			/* int dummy = */(void)read(ErrFile, buf, ErrSize);
			buf[ErrSize] = 0;
			errStr.Cat(buf);
			free(buf);
		}
	}
	else
		LLOG("Error spawning process");

	// closes files
	close(OutFile);
	close(ErrFile);

	return res;

} // END SysExec()

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command, passing a command line to it without waiting for its termination
// optionally returns pid of started process
bool SysStart(String const &command, String const &argLine, const VectorMap<String, String> &envMap, intptr_t *pidPtr)
{
	LLOG("SysStart() STARTED");

	// builds the arguments and the environment
	char **argv = BuildArgs(GetFileName(command), argLine);
	char **envv = BuildEnv(envMap);
	
	// initializes result in case of successful fork but
	// failed execvpe.... so the return value is correct
	if(pidPtr)
		*pidPtr = -1;
	
	// forks the process
	bool res = true;
	pid_t pID = fork();
	if (pID == 0)
	{
		// Code only executed by child process

		// exec process, function shall not return if all ok
		execvpe(command, argv, envv);

		// if it returns couldn't execute
		LLOG("Couldn't execute command");
		_exit(255);
	}
	if (pID < 0)
	{
		// code executed only if couldn't fork
		Cerr() << "Couldn't fork...\n";
		res = false;
	}
	if(pID > 0)
	{
		// Code only executed by parent process
		// waits for parent to terminate

		int status = -1;
		int result;
		pID = wait(&status);

		if (WIFEXITED(status))
		{
			result = WEXITSTATUS(status);
			LLOG("The process ended with exit(" << result << ")");
		}
		if (WIFSIGNALED(status))
		{
			result = WTERMSIG(status);
			LLOG("The process ended with kill -" << result);
		}

		if(result == 255)
			res = false;
		
		if(pidPtr)
			*pidPtr = pID;
	}

	if(!res)
		LLOG("Error spawning process");

	return res;

}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as Admin user, passing a command line to it without waiting for its termination
// it WILL prompt for user intervention on secure systems (linux - Vista+ OSes)
// return true on success, false otherwise
bool SysStartAdmin(String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	LLOG("SysStartAdmin() STARTED");
	// on linux, we must provide the password
	return SudoExec("root", password, command + " " + argLine, envMap, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as Admin user, passing a command line to it waiting for its termination
// it WILL prompt for user intervention on secure systems (linux - Vista+ OSes)
// return true on success, false otherwise
bool SysExecAdmin(String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	// on linux, we must provide the password
	return SudoExec("root", password, command + " " + argLine, envMap, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as required user, passing a command line to it without waiting for its termination
// on linux, will return an error if password is required AND wrong
// on windows, by now... it just spawn the process without changing security level
// I still shall find a way to go back to user mode on windows
// return true on success, false otherwise
bool SysStartUser(String const &user, String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	// on linux, we must provide the password
	return SudoExec(user, password, command + " " + argLine, envMap, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// executes an external command as required user, passing a command line to it waiting for its termination
// on linux, will return an error if password is required AND wrong
// on windows, by now... it just spawn the process without changing security level
// I still shall find a way to go back to user mode on windows
// return true on success, false otherwise
bool SysExecUser(String const &user, String const &password, String const &command, String const &argLine, const VectorMap<String, String> &envMap)
{
	// on linux, we must provide the password
	return SudoExec(user, password, command + " " + argLine, envMap, true);
}

END_UPP_NAMESPACE

#endif
