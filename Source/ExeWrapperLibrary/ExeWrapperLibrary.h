#pragma once

#include <string>
#include <filesystem>

class ExeWrapperFunctions
{
public:
	static bool WrapExeFile();
	static bool UnwrapExeFile();
	static bool RunWrappedProcess();

	static bool HasWrappedExe();

protected:
	static bool RequestPath(std::filesystem::path& path_);

	static bool GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& outBytes_);

	static bool TransferIcons(const std::filesystem::path& _sourceExeFile, const std::filesystem::path& _destExeFile);

	static bool DuplicateSelfExeFile(const wchar_t* _newFileName, std::filesystem::path& newExePath_);
	static bool RemoveFile(const std::filesystem::path& _path);
};