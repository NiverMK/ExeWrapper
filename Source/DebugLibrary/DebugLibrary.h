#pragma once

#include <windows.h>
#include <iostream>

#define EW_LOG(message, ...)			wprintf(message, __VA_ARGS__);\
										std::wcout << std::endl;

#define EW_LOG_FUNC(message, ...)		std::wcout << __FUNCTIONW__ << L": ";\
										wprintf(message, __VA_ARGS__);\
										std::wcout << std::endl;

#define EW_LOG_WIN_ERROR(message, ...)	std::wcout << __FUNCTIONW__ << L" (Error " << GetLastError() << L"): ";\
										wprintf(message, __VA_ARGS__); std::wcout << std::endl;

namespace DebugLibrary
{
	void PrintSelfMemoryBlock(const SIZE_T _start, const SIZE_T _lines, const SIZE_T _columns = 16);
	void PrintProcessMemoryBlock(
		const HANDLE _hProcess,
		const LPCVOID _imageBase,
		const SIZE_T _start,
		const SIZE_T _lines,
		const SIZE_T _columns = 16);

	void PrintFileMemoryBlock(const wchar_t* _fileName, const SIZE_T _start, const SIZE_T _lines, const SIZE_T _columns = 16);
}