#pragma once

#include <string>
#include <filesystem>

class ExeWrapperFunctions
{
public:
	static bool WrapExeFile();

	static bool RunWrappedProcess();

protected:
	static bool GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& outBytes_);

	static bool TransferIcons(const wchar_t* _sourceExeFile, const wchar_t* _destExeFile);

	static bool DuplicateSelfExeFile(const wchar_t* _newFileName, std::filesystem::path& newExePath_);
};