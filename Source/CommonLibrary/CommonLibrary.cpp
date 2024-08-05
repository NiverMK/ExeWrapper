#include "CommonLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"

#include <windows.h>

bool CommonLibrary::GetSelfPath(std::filesystem::path& path_)
{
	wchar_t* pathToSelf = new wchar_t[MAX_PATH];

	if (const DWORD result = GetModuleFileName(NULL, pathToSelf, MAX_PATH); !result || result == MAX_PATH)
	{
		EW_LOG_WIN_ERROR(L"Path %s is too long. Possible loss of data!", pathToSelf);

		delete[] pathToSelf;
		return false;
	}

	path_ = std::filesystem::path(pathToSelf);
	delete[] pathToSelf;

	return true;
}

bool CommonLibrary::ConvertMultiByteToWideChar(const char* _pString, bool _isLaunchArg, std::wstring& result_)
{
	const UINT codePage = _isLaunchArg ? CP_ACP : CP_OEMCP;
	const int wcharsAmount = MultiByteToWideChar(codePage, NULL, _pString, -1, nullptr, 0);

	if (wcharsAmount > 0)
	{
		const int copyCharsAmount = strlen(_pString);
		const int copyWcharsAmount = wcharsAmount - 1;

		std::wstring result;
		result.resize(copyWcharsAmount);

		const int resultWcharsAmount = MultiByteToWideChar(codePage, NULL, _pString, copyCharsAmount, result.data(), copyWcharsAmount);

		if (resultWcharsAmount == copyWcharsAmount)
		{
			result_ = std::move(result);
			return true;
		}
	}

	return false;
}