#include "SysExec.h"

NAMESPACE_UPP

bool SysExec(String const &command, String const &argLine, String &OutStr, String &ErrStr)
{
	return SysExec(command, argLine, Environment(), OutStr, ErrStr);

} // END SysExec()

bool SysExec(String const &command, String const &argLine, const VectorMap<String, String> &Environ, String &OutStr)
{
	String ErrStr;
	return SysExec(command, argLine, Environ, OutStr, ErrStr);

} // END SysExec()

bool SysExec(String const &command, String const &argLine, String &OutStr)
{
	String ErrStr;
	return SysExec(command, argLine, Environment(), OutStr, ErrStr);

} // END SysExec()

bool SysExec(String const &command, String const &argLine, const VectorMap<String, String> &Environ)
{
	String OutStr, ErrStr;
	return SysExec(command, argLine, Environ, OutStr, ErrStr);
}

bool SysExec(String const &command, String const &argLine)
{
	String OutStr, ErrStr;
	return SysExec(command, argLine, Environment(), OutStr, ErrStr);

} // END SysExec()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SysStart(String const &command, String const &argLine, intptr_t *pid)
{
	return SysStart(command, argLine, Environment(), pid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SysStartAdmin(String const &password, String const &command, String const &argLine)
{
	return SysStartAdmin(password, command, argLine, Environment());
	
}

bool SysExecAdmin(String const &password, String const &command, String const &args)
{
	return SysExecAdmin(password, command, args, Environment());
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SysStartUser(String const &user, String const &password, String const &command, String const &argLine)
{
	return SysStartUser(user, password, command, argLine, Environment());
}

bool SysExecUser(String const &user, String const &password, String const &command, String const &argLine)
{
	return SysExecUser(user, password, command, argLine, Environment());
}

END_UPP_NAMESPACE
