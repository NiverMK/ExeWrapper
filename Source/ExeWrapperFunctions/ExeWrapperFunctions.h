#pragma once

#include <xstring>

namespace ExeWrapperFunctions
{
	bool GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& _outBytes);

	bool CreateProcessFromBytes(char* _exeBytes);

	bool WrapExeFile();
}