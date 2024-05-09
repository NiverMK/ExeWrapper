#pragma once

#include <windows.h>
#include <xstring>

namespace ExeWrapperFunctions
{
	bool GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& _outBytes);

	bool CreateHollowProcess(HMODULE& _pBaseAddr_, PROCESS_INFORMATION& _pInfo_);
	LPVOID WriteProcessPages(const PROCESS_INFORMATION& _pInfo, const PIMAGE_NT_HEADERS32& _ntHeader, const HMODULE _pBaseAddr, const char* _buffer);

	bool Wow64_CreateProcess(char* _exeBytes);

#if _WIN64
	bool x64_CreateProcess(char* _exeBytes);
#endif

	bool RunWrappedProcess(char* _exeBytes);

	bool WrapExeFile();
}