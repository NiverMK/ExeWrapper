#include "PebLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"

#include <stdint.h>

typedef NTSTATUS(NTAPI* NtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG, PULONG);

PPEB PebLibrary::NTDLL_GetPEB(const PROCESS_INFORMATION& _processInfo)
{
	HMODULE hNTDLL = GetModuleHandle(L"ntdll.dll");

	if (!hNTDLL)
	{
		EW_LOG_WIN_ERROR(L"Can't find loaded ntdll.dll!");
		return nullptr;
	}

	FARPROC functionPtr = GetProcAddress(hNTDLL, "NtQueryInformationProcess");

	if (!functionPtr)
	{
		EW_LOG_WIN_ERROR(L"Can't find NtQueryInformationProcess function!");
		return nullptr;
	}

	NtQueryInformationProcess NtQueryInformationProcessPtr = reinterpret_cast<NtQueryInformationProcess>(functionPtr);

	PROCESS_BASIC_INFORMATION pbi;
	ZeroMemory(&pbi, sizeof(pbi));

	const NTSTATUS status = NtQueryInformationProcessPtr(_processInfo.hProcess, PROCESSINFOCLASS::ProcessBasicInformation, &pbi, sizeof(pbi), NULL);

	return pbi.PebBaseAddress;
}

PPEB PebLibrary::CONTEXT_GetPEB(const PROCESS_INFORMATION& _processInfo)
{
#if _WIN64
	/* is process x32 running on x64 */
	int isWow = false;
	if (!IsWow64Process(_processInfo.hProcess, &isWow))
	{
		EW_LOG_WIN_ERROR(L"IsWow64Process won't execute!");
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
		EW_LOG_WIN_ERROR(L"IsWow64Process won't execute!");
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
			EW_LOG(L"Wow64 Process can't request context of non-wow64 process!");
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

HMODULE PebLibrary::GetProcessBaseAddress(const PROCESS_INFORMATION& _processInfo, const PPEB _peb)
{
	/* x64 - char* +0x10 , x86 char* +0x08 */
	LPCVOID imageBasePtr = &_peb->ProcessBaseAddress;
	LPVOID imageBase = nullptr;

	/* read process address stored in pbi.PebBaseAddress->ProcessBaseAdress */
	if (!ReadProcessMemory(_processInfo.hProcess, imageBasePtr, &imageBase, sizeof(LPVOID), nullptr))
	{
		EW_LOG_WIN_ERROR(L"Can't read process memory!");
	}

	return static_cast<HMODULE>(imageBase);
}