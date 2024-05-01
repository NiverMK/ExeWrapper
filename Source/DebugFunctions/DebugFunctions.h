#pragma once

#include <windows.h>

namespace DebugFunctions
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