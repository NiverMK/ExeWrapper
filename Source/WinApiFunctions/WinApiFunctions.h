#pragma once

#include "WinApiStructs.h"

#include <string>

namespace WinApiFunctions
{
	PPEB NTDLL_GetPEB(const PROCESS_INFORMATION& _processInfo);
	PPEB CONTEXT_GetPEB(const PROCESS_INFORMATION& _processInfo);

	/* Get base address of process */
	HMODULE GetProcessBaseAddress(const PROCESS_INFORMATION& _processInfo, const PPEB _peb);

	/* Clean process stack */
	bool NtUnmapViewOfSection(const HANDLE _processHandle, const PVOID _baseAddress);

	bool DuplicateSelfExeFile(const wchar_t* _newFileName, std::wstring& newExePath_);

	bool WrapResourceIntoExeFile(const wchar_t* _pathToExeFile, const int _resourceName, const wchar_t* _resourceType, const std::string& _bytes);

	/* if _pathToExeFile == nullptr, then resource will be unwrapped from current process */
	std::string UnwrapResourceFromExeFile(const wchar_t* _pathToExeFile, const int _resourceName, const wchar_t* _resourceType);
}