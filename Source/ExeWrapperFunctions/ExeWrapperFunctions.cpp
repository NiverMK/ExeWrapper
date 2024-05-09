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

bool ExeWrapperFunctions::CreateHollowProcess(HMODULE& pBaseAddr_, PROCESS_INFORMATION& pInfo_)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	si.cb = sizeof(si);

	wchar_t* pathToExeFile = new wchar_t[MAX_PATH];
	GetModuleFileName(NULL, pathToExeFile, MAX_PATH);

	std::wstring pathToExeFileStr(pathToExeFile);
	delete[] pathToExeFile;

	/* CreateProcess gives PROCESS_ALL_ACCESS rights for created process */
	if (!CreateProcess(nullptr, pathToExeFileStr.data(), nullptr, nullptr, false, CREATE_SUSPENDED | CREATE_NEW_CONSOLE, 0, 0, &si, &pi))
	{
		std::cout << "CreateProcess Error: " << GetLastError() << std::endl;
		return false;
	}

	//PPEB peb = WinApiFunctions::NTDLL_GetPEB(pi);
	PPEB peb = WinApiFunctions::CONTEXT_GetPEB(pi);
	HMODULE pBaseAddr = WinApiFunctions::GetProcessBaseAddress(pi, peb);

	/* Clean process memory */
	if (!WinApiFunctions::NtUnmapViewOfSection(pi.hProcess, pBaseAddr))
	{
		std::cout << "CreateProcessFromBytes::NtUnmapViewOfSection Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	pBaseAddr_ = pBaseAddr;
	pInfo_ = pi;

	return true;
}

LPVOID ExeWrapperFunctions::WriteProcessPages(const PROCESS_INFORMATION& _pInfo, const PIMAGE_NT_HEADERS32& _ntHeader, const HMODULE _pBaseAddr, const char* _buffer)
{
	LPVOID allocPtr = VirtualAllocEx(_pInfo.hProcess, _pBaseAddr, _ntHeader->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (!allocPtr)
	{
		std::cout << "WriteProcessPages::VirtualAllocEx Error: " << GetLastError() << std::endl;

		return nullptr;
	}

	if (!WriteProcessMemory(_pInfo.hProcess, allocPtr, _buffer, _ntHeader->OptionalHeader.SizeOfHeaders, 0))
	{
		std::cout << "WriteProcessPages::WriteProcessMemory Error: " << GetLastError() << std::endl;

		return nullptr;
	}

	PIMAGE_SECTION_HEADER sect = IMAGE_FIRST_SECTION(_ntHeader);

	for (int i = 0; i < _ntHeader->FileHeader.NumberOfSections; i++)
	{
		SIZE_T bytes;
		bool result = WriteProcessMemory(_pInfo.hProcess, PCHAR(allocPtr) + sect[i].VirtualAddress, PCHAR(_buffer) + sect[i].PointerToRawData, sect[i].SizeOfRawData, &bytes);

		if (!result)
		{
			std::cout << "WriteProcessPages::WriteProcessMemory at header index " << i << " Error: " << GetLastError() << std::endl;

			return nullptr;
		}

		DWORD prevProtect;
		result = VirtualProtectEx(_pInfo.hProcess, PCHAR(allocPtr) + sect[i].VirtualAddress, sect[i].Misc.VirtualSize, sect[i].Characteristics, &prevProtect);

		if (!result)
		{
			/* can't change protection of some sections. wtf? */
			std::cout << "WriteProcessPages::VirtualProtectEx at header index " << i << " Error: " << GetLastError() << std::endl;
			/*
			TerminateProcess(pi.hProcess, 0);
			return nullptr;
			*/
		}
	}

	return allocPtr;
}


bool ExeWrapperFunctions::Wow64_CreateProcess(char* _exeBytes)
{
	HMODULE pBaseAddr = NULL;
	PROCESS_INFORMATION pi;

	if (!CreateHollowProcess(pBaseAddr, pi))
	{
		return false;
	}

	char* buffer = _exeBytes;

	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(buffer);
	PIMAGE_NT_HEADERS32 ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS32>((reinterpret_cast<char*>(dosHeader) + dosHeader->e_lfanew));

	LPVOID allocPtr = WriteProcessPages(pi, ntHeader, pBaseAddr, buffer);

	if (!allocPtr)
	{
		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	WOW64_CONTEXT context = { CONTEXT_INTEGER };
	context.ContextFlags = CONTEXT_ALL;

	if (!Wow64GetThreadContext(pi.hThread, &context))
	{
		std::cout << "Wow64_CreateProcess::Wow64GetThreadContext Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	context.Eax = (DWORD)allocPtr + ntHeader->OptionalHeader.AddressOfEntryPoint;

	if (!Wow64SetThreadContext(pi.hThread, &context))
	{
		std::cout << "Wow64_CreateProcess::Wow64SetThreadContext Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	ResumeThread(pi.hThread);

	return true;
}

#if _WIN64
bool ExeWrapperFunctions::x64_CreateProcess(char* _exeBytes)
{
	HMODULE pBaseAddr = NULL;
	PROCESS_INFORMATION pi;

	if (!CreateHollowProcess(pBaseAddr, pi))
	{
		return false;
	}

	char* buffer = _exeBytes;

	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(buffer);
	PIMAGE_NT_HEADERS32 ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS32>((reinterpret_cast<char*>(dosHeader) + dosHeader->e_lfanew));

	LPVOID allocPtr = WriteProcessPages(pi, ntHeader, pBaseAddr, buffer);

	if (!allocPtr)
	{
		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	CONTEXT context = { CONTEXT_INTEGER };
	context.ContextFlags = CONTEXT_ALL;

	if (!GetThreadContext(pi.hThread, &context))
	{
		std::cout << "x64_CreateProcess::GetThreadContext Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	context.Rcx = reinterpret_cast<DWORD64>(allocPtr) + ntHeader->OptionalHeader.AddressOfEntryPoint;

	if (!SetThreadContext(pi.hThread, &context))
	{
		std::cout << "x64_CreateProcess::SetThreadContext Error: " << GetLastError() << std::endl;

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	ResumeThread(pi.hThread);

	return true;
}
#endif

bool ExeWrapperFunctions::RunWrappedProcess(char* _exeBytes)
{
#if _WIN64
	/* x64 app on x64 system */
	return x64_CreateProcess(_exeBytes);
#else
	if (BOOL isSelfWow = false; IsWow64Process(GetCurrentProcess(), &isSelfWow) && isSelfWow)
	{
		/* x32 app on x64 system */
		return Wow64_CreateProcess(_exeBytes);
	}
	else
	{
		/* x32 app on x32 system */
		std::cout << "x32 systems are not supported!" << std::endl;
	}
#endif

	return false;
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