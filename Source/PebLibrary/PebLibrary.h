#pragma once

#include "PebLibStructs.h"

namespace PebLibrary
{
	PPEB NTDLL_GetPEB(const PROCESS_INFORMATION& _processInfo);
	PPEB CONTEXT_GetPEB(const PROCESS_INFORMATION& _processInfo);

	HMODULE GetProcessBaseAddress(const PROCESS_INFORMATION& _processInfo, const PPEB _peb);
}