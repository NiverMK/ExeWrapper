#include "DebugFunctions.h"
#include "../WinApiFunctions/WinApiFunctions.h"

#include <iostream>
#include <iomanip>
#include <fstream>

void DebugFunctions::PrintSelfMemoryBlock(const SIZE_T _start, const SIZE_T _lines, const SIZE_T _columns)
{
	if (!_lines || !_columns)
	{
		return;
	}

	HMODULE selfModule = GetModuleHandleW(L"ExeWrapper.exe");

	if (!selfModule)
	{
		return;
	}

	char* selfModulePtr = reinterpret_cast<char*>(selfModule);
	char* bytePtr = selfModulePtr + _start;

	for (SIZE_T i = 0; i < _lines; i++)
	{
		printf("%p   ", bytePtr);

		for (SIZE_T j = 0; j < _columns; j++)
		{
			printf("%02X ", static_cast<unsigned char>(*bytePtr));
			bytePtr++;
		}

		std::cout << std::endl;
	}
}

void DebugFunctions::PrintProcessMemoryBlock(
	const HANDLE _hProcess,
	const LPCVOID _imageBase,
	const SIZE_T _start,
	const SIZE_T _lines,
	const SIZE_T _columns)
{
	if (!_lines || !_columns)
	{
		return;
	}

	const char* bytePtr = reinterpret_cast<const char*>(_imageBase) + _start;
	unsigned char* buffer = new unsigned char[_lines * _columns];
	
	if (ReadProcessMemory(_hProcess, bytePtr, buffer, _lines * _columns, nullptr))
	{
		for (SIZE_T i = 0; i < _lines; i++)
		{
			printf("%p   ", bytePtr);

			for (SIZE_T j = 0; j < _columns; j++)
			{
				printf("%02X ", buffer[i * _columns + j]);
				bytePtr++;
			}

			std::cout << std::endl;
		}
	}

	delete[] buffer;
}

void DebugFunctions::PrintFileMemoryBlock(const wchar_t* _fileName, const SIZE_T _start, const SIZE_T _lines, const SIZE_T _columns)
{
	if (!_lines || !_columns)
	{
		return;
	}

	std::ifstream exeFile;
	exeFile.open(_fileName, std::ios::binary | std::ios::ate);

	if (exeFile.is_open())
	{
		std::streamsize fileSize = exeFile.tellg();

		if (_start + (_lines * _columns) > static_cast<SIZE_T>(fileSize))
		{
			return;
		}

		exeFile.seekg(0, std::ios::beg);

		char* buffer = new char[fileSize];

		if (exeFile.read(buffer, fileSize))
		{
			std::cout << "file size: " << fileSize << std::endl;

			char* bytePtr = buffer + _start;

			for (SIZE_T i = 0; i < _lines; i++)
			{
				printf("%p   ", bytePtr);

				for (SIZE_T j = 0; j < _columns; j++)
				{
					printf("%02X ", static_cast<unsigned char>(*bytePtr));
					bytePtr++;
				}
				std::cout << std::endl;
			}
		}

		exeFile.close();
		delete[] buffer;
	}
}