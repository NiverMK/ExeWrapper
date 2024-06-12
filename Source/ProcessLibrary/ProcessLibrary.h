#pragma once

#include <windows.h>

class ProcessLibrary
{
public:
	static bool CreateProcessFromRam(char* _exeBytes);

protected:
	static bool CreateHollowProcess(HMODULE& _pBaseAddr_, PROCESS_INFORMATION& _pInfo_);
	static LPVOID WriteProcessPages(const PROCESS_INFORMATION& _pInfo, const PIMAGE_NT_HEADERS32& _ntHeader, const HMODULE _pBaseAddr, const char* _buffer);

	static bool Wow64_CreateProcess(char* _exeBytes);

#if _WIN64
	static bool x64_CreateProcess(char* _exeBytes);
#endif

	/* Clean process stack */
	static bool NtUnmapViewOfSection(const HANDLE _processHandle, const PVOID _baseAddress);
};