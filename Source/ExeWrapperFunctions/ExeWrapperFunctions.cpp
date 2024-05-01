#include "ExeWrapperFunctions.h"
#include "../WinapiFunctions/WinApiFunctions.h"
#include "../DebugFunctions/DebugFunctions.h"
#include "../../Resources/resource.h"

#include <iostream>
#include <fstream>

bool ExeWrapperFunctions::GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& _outBytes)
{
	std::ifstream exeFile;
	exeFile.open(_pathToExeFile, std::ios::binary | std::ios::ate);

	if (!exeFile.is_open())
	{
		std::cout << "GetExeFileBytes::open Cannot open the file!" << std::endl;
		return false;
	}

	std::streamsize fileSize = exeFile.tellg();
	exeFile.seekg(0, std::ios::beg);

	_outBytes.resize(fileSize);

	if (!exeFile.read(_outBytes.data(), fileSize))
	{
		std::cout << "GetExeFileBytes::read Cannot read the file!" << std::endl;
		return true;
	};

	return true;
}

bool ExeWrapperFunctions::CreateProcessFromBytes(char* _exeBytes)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof si };

	wchar_t* pathToExeFile = new wchar_t[MAX_PATH];
	GetModuleFileName(NULL, pathToExeFile, MAX_PATH);

	std::wstring pathToExeFileStr(pathToExeFile);
	delete[] pathToExeFile;

	/* CreateProcess gives PROCESS_ALL_ACCESS rights for created process */
	if (!CreateProcess(nullptr, pathToExeFileStr.data(), nullptr, nullptr, false, CREATE_SUSPENDED, 0, 0, &si, &pi))
	{
		std::cout << "CreateProcess Error: " << GetLastError() << std::endl;
		return false;
	}

	CONTEXT context = { CONTEXT_INTEGER };
	context.ContextFlags = CONTEXT_ALL;
	GetThreadContext(pi.hThread, &context);

	//PPEB peb = WinApiFunctions::NTDLL_GetPEB(pi);
	PPEB peb = WinApiFunctions::CONTEXT_GetPEB(pi);
	HMODULE pBaseAddr = WinApiFunctions::NTDLL_GetProcessBaseAddress(pi, peb);

	/* Clean process memory */
	WinApiFunctions::NtUnmapViewOfSection(pi.hProcess, pBaseAddr);

	char* buffer = _exeBytes;

	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(buffer);
	PIMAGE_NT_HEADERS32 ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS32>((reinterpret_cast<char*>(dosHeader) + dosHeader->e_lfanew));

	LPVOID allocPtr = VirtualAllocEx(pi.hProcess, pBaseAddr, ntHeader->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (!allocPtr)
	{
		std::cout << "CreateProcessFromBytes::VirtualAllocEx Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	if (!WriteProcessMemory(pi.hProcess, allocPtr, buffer, ntHeader->OptionalHeader.SizeOfHeaders, 0))
	{
		std::cout << "CreateProcessFromBytes::WriteProcessMemory Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	PIMAGE_SECTION_HEADER sect = IMAGE_FIRST_SECTION(ntHeader);

	for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		SIZE_T bytes;
		bool result = WriteProcessMemory(pi.hProcess, PCHAR(allocPtr) + sect[i].VirtualAddress, PCHAR(buffer) + sect[i].PointerToRawData, sect[i].SizeOfRawData, &bytes);

		if (!result)
		{
			std::cout << "WriteProcessMemory at header index " << i << " Error: " << GetLastError() << std::endl;

			TerminateProcess(pi.hProcess, 0);
			return false;
		}

		DWORD prevProtect;
		result = VirtualProtectEx(pi.hProcess, PCHAR(allocPtr) + sect[i].VirtualAddress, sect[i].Misc.VirtualSize, sect[i].Characteristics, &prevProtect);

		if (!result)
		{
			/* can't change protection of some sections. wtf? */
			std::cout << "VirtualProtectEx at header index " << i << " Error: " << GetLastError() << std::endl;
			/*
			TerminateProcess(pi.hProcess, 0);
			return false;
			*/
		}
	}

#if _WIN64
	context.Rcx = (DWORD64)allocPtr + ntHeader->OptionalHeader.AddressOfEntryPoint;
#else
	context.Eax = (DWORD)allocPtr + ntHeader->OptionalHeader.AddressOfEntryPoint;
#endif

	if (!SetThreadContext(pi.hThread, &context))
	{
		std::cout << "SetThreadContext Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}
	
	ResumeThread(pi.hThread);

	return true;
}

bool ExeWrapperFunctions::WrapExeFile()
{
	std::cout << "Drag here .exe file" << std::endl;

	std::wstring pathToExeFile;
	std::getline(std::wcin, pathToExeFile);

	if (pathToExeFile.length())
	{
		if (pathToExeFile.data()[0] == L'\"')
		{
			pathToExeFile = pathToExeFile.substr(1, pathToExeFile.length() - 1);
		}
		if (pathToExeFile.data()[pathToExeFile.length() - 1] == L'\"')
		{
			pathToExeFile = pathToExeFile.substr(0, pathToExeFile.length() - 1);
		}
	}
	else
	{
		return false;
	}

	std::string exeBytes;
	if (!GetExeFileBytes(pathToExeFile, exeBytes))
	{
		return false;
	}

	const size_t pos = pathToExeFile.rfind(L"\\");
	const std::wstring newExeFileName = pathToExeFile.substr(pos + 1);
	std::wstring newExeFilePath;

	if (!WinApiFunctions::DuplicateSelfExeFile(newExeFileName.c_str(), newExeFilePath))
	{
		return false;
	}

	if (!WinApiFunctions::WrapResourceIntoExeFile(newExeFilePath.c_str(), IDR_BIN1, IDR_BIN1_TYPE, exeBytes))
	{
		return false;
	}

	std::cout << "Wrapping was successful! " << std::endl;

	return true;
}