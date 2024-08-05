#pragma once

#include <filesystem>
#include <string>

namespace CommonLibrary
{
	bool GetSelfPath(std::filesystem::path& path_);

	bool ConvertMultiByteToWideChar(const char* _pString, bool _isLaunchArg, std::wstring& result_);
}