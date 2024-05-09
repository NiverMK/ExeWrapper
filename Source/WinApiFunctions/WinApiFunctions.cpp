#include "WinApiFunctions.h"

#include <iostream>

typedef NTSTATUS(NTAPI *NtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG, PULONG);
typedef NTSTATUS(NTAPI *NtUnmapViewOfSectionFunc)(HANDLE, PVOID);

static HMODULE hNTDLL = NULL;

PPEB WinApiFunctions::NTDLL_GetPEB(const PROCESS_INFORMATION& _processInfo)
{
	if (!hNTDLL)
	{
		hNTDLL = GetModuleHandle(L"ntdll.dll");

		if (!hNTDLL)
		{
			return nullptr;
		}
	}

	FARPROC functionPtr = GetProcAddress(hNTDLL, "NtQueryInformationProcess");

	if (!functionPtr)
	{
		return nullptr;
	}

	NtQueryInformationProcess NtQueryInformationProcessPtr = reinterpret_cast<NtQueryInformationProcess>(functionPtr);

	PROCESS_BASIC_INFORMATION pbi;
	ZeroMemory(&pbi, sizeof(pbi));

	const NTSTATUS status = NtQueryInformationProcessPtr(_processInfo.hProcess, 0, &pbi, sizeof(pbi), NULL);

	return pbi.PebBaseAddress;
}

PPEB WinApiFunctions::CONTEXT_GetPEB(const PROCESS_INFORMATION& _processInfo)
{
#if _WIN64
	/* is process x32 running on x64 */
	int isWow = false;
	if (!IsWow64Process(_processInfo.hProcess, &isWow))
	{
		return nullptr;
	}

	if (isWow)
	{
		WOW64_CONTEXT ctx;
		ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;

		if (Wow64GetThreadContext(_processInfo.hThread, &ctx))
		{
			/* ctx.Ebx - PEB adress. Process base adress is located ((char*)ctx.Ebx + 0x08) for x32 */
			return reinterpret_cast<PPEB>(static_cast<uint64_t>(ctx.Ebx));
		}
	}
	else
	{
		CONTEXT ctx;
		ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;

		if (GetThreadContext(_processInfo.hThread, &ctx))
		{
			/* ctx.Rdx - PEB adress. Base adress of suspended process is located ((char*)ctx.Rdx + 0x10) for x64 */
			return reinterpret_cast<PPEB>(ctx.Rdx);
		}
	}
#else
	BOOL isSelfWow = false;
	BOOL isProcWow = false;
	if (!IsWow64Process(_processInfo.hProcess, &isProcWow) || !IsWow64Process(GetCurrentProcess(), &isSelfWow))
	{
		return nullptr;
	}

	if (isSelfWow)
	{
		/* we are running on x64 system */
		if (isProcWow)
		{
			WOW64_CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;

			if (Wow64GetThreadContext(_processInfo.hThread, &ctx))
			{
				/* ctx.Eax - PEB adress. Process base adress is located ((char*)ctx.Eax + 0x08) for x32 */
				return reinterpret_cast<PPEB>(ctx.Ebx);
			}
		}
		else
		{
			/* wow process cannot request context of non-wow process */
			return nullptr;
		}
	}
	else
	{
		/* we are running on x32 system */
		if (!isProcWow)
		{
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;

			if (GetThreadContext(_processInfo.hThread, &ctx))
			{
				/* ctx.Eax - PEB adress. Base adress of suspended process is located ((char*)ctx.Eax + 0x08) for x32 */
				return reinterpret_cast<PPEB>(ctx.Eax);
			}
		}
	}
#endif

	return nullptr;
}

HMODULE WinApiFunctions::GetProcessBaseAddress(const PROCESS_INFORMATION& _processInfo, const PPEB _peb)
{
	/* x64 - char* +0x10 , x86 char* +0x08 */
	LPCVOID imageBasePtr = &_peb->ProcessBaseAddress;
	LPVOID imageBase = nullptr;

	/* read process address stored in pbi.PebBaseAddress->ProcessBaseAdress */
	ReadProcessMemory(_processInfo.hProcess, imageBasePtr, &imageBase, sizeof(LPVOID), nullptr);

	return static_cast<HMODULE>(imageBase);
}

bool WinApiFunctions::NtUnmapViewOfSection(const HANDLE _processHandle, const PVOID _baseAddress)
{
	if (!hNTDLL)
	{
		hNTDLL = GetModuleHandleW(L"ntdll.dll");

		if (!hNTDLL)
		{
			return false;
		}
	}

	FARPROC functionPtr = GetProcAddress(hNTDLL, "NtUnmapViewOfSection");

	if (!functionPtr)
	{
		return false;
	}

	NtUnmapViewOfSectionFunc NtUnmapViewOfSectionPtr = reinterpret_cast<NtUnmapViewOfSectionFunc>(functionPtr);

	return NtUnmapViewOfSectionPtr(_processHandle, _baseAddress) == STATUS_SUCCESS;
}

bool WinApiFunctions::DuplicateSelfExeFile(const wchar_t* _newFileName, std::wstring& newExePath_)
{
	if (!_newFileName)
	{
		std::cout << "DuplicateSelfExeFile New file name is empty!" << std::endl;

		return false;
	}

	wchar_t* pathToFile = new wchar_t[MAX_PATH];
	GetModuleFileName(NULL, pathToFile, MAX_PATH);

	const std::wstring pathToFileStr(pathToFile);
	delete[] pathToFile;

	if (!CopyFile(pathToFileStr.c_str(), _newFileName, true))
	{
		std::cout << "DuplicateSelfExeFile::CopyFileW File with selected name already exists!" << std::endl;

		return false;
	}

	const size_t pos = pathToFileStr.rfind(L"\\");
	std::wstring outNewExePath = pathToFileStr.substr(0, pos + 1) + _newFileName;

	DWORD fileAttr = GetFileAttributes(outNewExePath.c_str());

	if (fileAttr == 0xffffffff)
	{
		/* copied file wasn't found. if we are running program under visual studio check it in project's folder */
#if _WIN64
	#if _DEBUG
		const size_t projectFolderPos = pathToFileStr.rfind(L"x64\\Debug");
	#else
		const size_t projectFolderPos = pathToFileStr.rfind(L"x64\\Release");
	#endif
#else
	#if _DEBUG
		const size_t projectFolderPos = pathToFileStr.rfind(L"Debug");
	#else
		const size_t projectFolderPos = pathToFileStr.rfind(L"Release");
	#endif
#endif
		outNewExePath = pathToFileStr.substr(0, projectFolderPos) + _newFileName;

		fileAttr = GetFileAttributes(outNewExePath.c_str());

		if (fileAttr == 0xffffffff)
		{
			std::cout << "DuplicateSelfExeFile Unable to find copied .exe file" << std::endl;

			return false;
		}
	}

	newExePath_ = outNewExePath;

	return true;
}

bool WinApiFunctions::WrapResourceIntoExeFile(const wchar_t* _pathToExeFile, const int _resourceName, const wchar_t* _resourceType, const std::string& _bytes)
{
	HANDLE hUpdateRes = BeginUpdateResource(_pathToExeFile, FALSE);

	if (!hUpdateRes)
	{
		std::cout << "WrapExeFileIntoResources::BeginUpdateResource Error: " << GetLastError() << std::endl;
		return false;
	}

	if (!UpdateResource(hUpdateRes, _resourceType, 
		MAKEINTRESOURCE(_resourceName),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), 
		const_cast<void*>((const void*)_bytes.data()), 
		static_cast<DWORD>(_bytes.length())))
	{
		std::cout << "WrapExeFileIntoResources::UpdateResource Error: " << GetLastError() << std::endl;
		return false;
	}

	if (!EndUpdateResource(hUpdateRes, FALSE))
	{
		std::cout << "WrapExeFileIntoResources::EndUpdateResource Error: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

std::string WinApiFunctions::UnwrapResourceFromExeFile(const wchar_t* _pathToExeFile, const int _resourceName, const wchar_t* _resourceType)
{
	std::string outStr;
	HMODULE hExe = NULL;

	if (_pathToExeFile)
	{
		hExe = LoadLibrary(_pathToExeFile);

		if (!hExe)
		{
			std::cout << "UnwrapResourcesFromExeFile::LoadLibrary Error: " << GetLastError() << std::endl;
			return outStr;
		}
	}

	HRSRC hRes = FindResource(hExe, MAKEINTRESOURCE(_resourceName), _resourceType);

	if (!hRes)
	{
		std::cout << "UnwrapResourcesFromExeFile::FindResource Error: " << GetLastError() << std::endl;
		return outStr;
	}

	HGLOBAL hResourceLoaded = LoadResource(hExe, hRes);

	if (!hResourceLoaded)
	{
		std::cout << "UnwrapResourcesFromExeFile::LoadResource Error: " << GetLastError() << std::endl;
		return outStr;
	}

	void* resLock = LockResource(hResourceLoaded);

	if (!resLock)
	{
		std::cout << "UnwrapResourcesFromExeFile::LockResource Error: " << GetLastError() << std::endl;
		return outStr;
	}

	DWORD dwResSize = SizeofResource(hExe, hRes);
	outStr = std::string(reinterpret_cast<char*>(resLock), dwResSize);

	if (hExe && !FreeLibrary(hExe))
	{
		std::cout << "UnwrapResourcesFromExeFile::FreeLibrary Error: " << GetLastError() << std::endl;
	}

	return outStr;
}