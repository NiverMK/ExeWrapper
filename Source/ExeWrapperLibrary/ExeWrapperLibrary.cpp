#include "ExeWrapperLibrary.h"
#include "../../Resources/resource.h"
#include "../ProcessLibrary/ProcessLibrary.h"
#include "../CmdLineArguments/CmdLineArgumentsKeeper.h"
#include "../ResourcesLibrary/GroupIconResourceLibrary/GroupIconResourceLibrary.h"
#include "../CommonLibrary/CommonLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"

#include <fstream>

bool ExeWrapperFunctions::WrapExeFile()
{
	CmdLineArgumentsKeeper& cmdKeeper = CmdLineArgumentsKeeper::GetCmdLaunchParamsKeeper();
	const bool hasWrapCmd = cmdKeeper.HasLaunchParam<CLA_Wrap>();
	const std::wstring& wrapCmdValue = cmdKeeper.GetParamValue<CLA_Wrap>();

	std::filesystem::path pathToExeFile;

	if (hasWrapCmd && !wrapCmdValue.empty())
	{
		pathToExeFile = std::filesystem::path(wrapCmdValue);
	}
	else
	{
		EW_LOG(L"Drag here .exe file");
		RequestPath(pathToExeFile);
	}

	if (pathToExeFile.empty())
	{
		EW_LOG_FUNC(L"Path is empty!");
		return false;
	}

	const std::wstring newExeFileName = pathToExeFile.filename();

	if (newExeFileName.empty())
	{
		EW_LOG_FUNC(L"Path %s hasn't filename!", pathToExeFile.wstring().data());
		return false;
	}

	std::filesystem::path newExeFilePath;

	if (!DuplicateSelfExeFile(newExeFileName.data(), newExeFilePath))
	{
		return false;
	}

	if (!TransferIcons(pathToExeFile, newExeFilePath))
	{
		RemoveFile(newExeFilePath);
		return false;
	}
	
	{
		ResourceSetter newExeResourceSetter{ newExeFilePath.wstring().data() };

		if (!newExeResourceSetter.IsValid())
		{
			RemoveFile(newExeFilePath);
			return false;
		}

		std::string exeBytes;
		if (!GetExeFileBytes(pathToExeFile, exeBytes))
		{
			RemoveFile(newExeFilePath);
			return false;
		}

		if (!newExeResourceSetter.SetResourceIntoExeFile(IDR_BIN1, IDR_BIN1_TYPE, exeBytes))
		{
			RemoveFile(newExeFilePath);
			return false;
		}
	}

	EW_LOG(L"Wrapped .exe file saved in %s", newExeFilePath.wstring().data());

	return true;
}

bool ExeWrapperFunctions::UnwrapExeFile()
{
	if (!HasWrappedExe())
	{
		EW_LOG_FUNC(L"There is no wrapped .exe!");
		return false;
	}

	CmdLineArgumentsKeeper& cmdKeeper = CmdLineArgumentsKeeper::GetCmdLaunchParamsKeeper();

	std::filesystem::path pathToUnwrappedExe;

	if (const std::wstring& wrapCmdValue = cmdKeeper.GetParamValue<CLA_Unwrap>(); !wrapCmdValue.empty())
	{
		pathToUnwrappedExe = std::filesystem::path(wrapCmdValue);
	}
	else
	{
		EW_LOG(L"Drag here folder");
		RequestPath(pathToUnwrappedExe);
	}

	if (pathToUnwrappedExe.empty())
	{
		EW_LOG_FUNC(L"Path is empty!");
		return false;
	}

	ResourceGetter newExeResourceSetter{ nullptr };

	std::string exeBytes;
	if (!newExeResourceSetter.GetResourceFromExeFile(IDR_BIN1, IDR_BIN1_TYPE, exeBytes))
	{
		return false;
	}

	std::filesystem::path pathToSelf;
	if (!CommonLibrary::GetSelfPath(pathToSelf))
	{
		return false;
	}

	pathToUnwrappedExe /= pathToSelf.filename();

	if (pathToSelf == pathToUnwrappedExe)
	{
		EW_LOG_FUNC(L"File (%s) already exists!", pathToUnwrappedExe.wstring().data());
		return false;
	}

	std::ofstream exeFile(pathToUnwrappedExe, std::ios::binary);
	exeFile << exeBytes;

	EW_LOG(L"Unwrapped .exe file saved in %s", pathToUnwrappedExe.wstring().data());

	return true;
}

bool ExeWrapperFunctions::RunWrappedProcess()
{
	ResourceGetter newExeResourceGetter{ nullptr };

	std::string exeBytes;
	if (!newExeResourceGetter.GetResourceFromExeFile(IDR_BIN1, IDR_BIN1_TYPE, exeBytes))
	{
		return false;
	}

	CmdLineArgumentsKeeper& cmdKeeper = CmdLineArgumentsKeeper::GetCmdLaunchParamsKeeper();
	std::wstring launchArgs = cmdKeeper.GetProcessLaunchArgs();

	EW_LOG(L"Running process with launch args '%s'", launchArgs.data());

	return ProcessLibrary::CreateProcessFromRam(exeBytes.data(), launchArgs.data());
}

bool ExeWrapperFunctions::HasWrappedExe()
{
	ResourceGetter binGetter{ nullptr };
	DWORD binSize = binGetter.GetResourceSize(IDR_BIN1, IDR_BIN1_TYPE);

	return binSize > DEFAULT_BIN1_SIZE;
}

bool ExeWrapperFunctions::RequestPath(std::filesystem::path& path_)
{
	std::string pathToExeFile;
	std::getline(std::cin, pathToExeFile);

	if (!pathToExeFile.length())
	{
		EW_LOG_FUNC(L"Path is empty!");
		return false;
	}
	else if (pathToExeFile.length() > 2)
	{
		if (pathToExeFile.data()[0] == '\"')
		{
			pathToExeFile = pathToExeFile.substr(1, pathToExeFile.length() - 1);
		}
		if (pathToExeFile.data()[pathToExeFile.length() - 1] == '\"')
		{
			pathToExeFile = pathToExeFile.substr(0, pathToExeFile.length() - 1);
		}
	}

	std::wstring wPathToExeFile;
	if (!CommonLibrary::ConvertMultiByteToWideChar(pathToExeFile.data(), false, wPathToExeFile))
	{
		EW_LOG_WIN_ERROR(L"Can't convert the path %hs from chars to wchars!", pathToExeFile.data());
		return false;
	}

	path_ = std::filesystem::path(wPathToExeFile);

	return true;
}

bool ExeWrapperFunctions::GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& outBytes_)
{
	std::ifstream exeFile;
	exeFile.open(_pathToExeFile, std::ios::binary | std::ios::ate);

	if (!exeFile.is_open())
	{
		EW_LOG_FUNC(L"Can't open the file!");

		return false;
	}

	std::streamsize fileSize = exeFile.tellg();
	exeFile.seekg(0, std::ios::beg);

	outBytes_.resize(fileSize);

	if (!exeFile.read(outBytes_.data(), fileSize))
	{
		EW_LOG_FUNC(L"Can't read the file!");

		return true;
	};

	return true;
}

bool ExeWrapperFunctions::TransferIcons(const std::filesystem::path& _sourceExeFile, const std::filesystem::path& _destExeFile)
{
	GroupIconResData srcGrpIconResData;
	{
		/* get image resources info from source .exe */
		GroupIconResourceGetter srcExeResGetter(_sourceExeFile.wstring().data());

		if (!srcExeResGetter.IsValid())
		{
			return false;
		}

		if (!srcExeResGetter.ReadGrpIconResources(true, srcGrpIconResData))
		{
			/* source .exe hasn't icons. Remove them in dest .exe */
			EW_LOG(L".exe file %s hasn't icon resources. Removing them from .exe %s", _sourceExeFile.wstring().data(), _destExeFile.wstring().data());
		}
	}

	GroupIconResData destGrpIconResData;
	{
		/* get group icons info from destination .exe file */
		GroupIconResourceGetter destExeResGetter(_destExeFile.wstring().data());

		if (!destExeResGetter.IsValid())
		{
			return false;
		}

		if (!destExeResGetter.ReadGrpIconResources(false, destGrpIconResData))
		{
			EW_LOG(L".exe file %s hasn't icon resources", _destExeFile.wstring().data());
		}

		/* free .exe before writing into it */
		if (!destExeResGetter.Release())
		{
			return false;
		}

		if (destGrpIconResData.HasGroupData() && destGrpIconResData.GetGroupIconHeader()->ImageCount)
		{
			/* remove old icons */
			GroupIconResourceSetter destExeResCleaner(_destExeFile.wstring().data());

			if (!destExeResCleaner.IsValid())
			{
				return false;
			}

			destExeResCleaner.CleanGrpIconResources(destGrpIconResData);
		}
	}

	if (!srcGrpIconResData.HasIcons())
	{
		/* there are no icons to transfer */
		return true;
	}

	GroupIconResData newGrpIconResData = std::move(srcGrpIconResData);

	if (destGrpIconResData.HasIcons())
	{
		/* use base icons ids. if destGrpIconResData is empty, use ids from srcGrpIconResData */
		newGrpIconResData.SetResourceName(destGrpIconResData.GetResourceName());

		for (int index = 0; index < newGrpIconResData.GetGroupIconHeader()->ImageCount; index++)
		{
			newGrpIconResData.GetGroupIconDirEntry()[index].nId = destGrpIconResData.GetGroupIconDirEntry()->nId + index;
		}
	}

	GroupIconResourceSetter srcExeResGetter(_destExeFile.wstring().data());

	if (!srcExeResGetter.IsValid())
	{
		return false;
	}

	if (!srcExeResGetter.SetGrpIconResources(newGrpIconResData))
	{
		return false;
	}

	return true;
}

bool ExeWrapperFunctions::DuplicateSelfExeFile(const wchar_t* _newFileName, std::filesystem::path& newExePath_)
{
	if (!_newFileName)
	{
		EW_LOG_FUNC(L"New file name is empty!");

		return false;
	}

	std::filesystem::path pathToSelf;
	if (!CommonLibrary::GetSelfPath(pathToSelf))
	{
		return false;
	}

	if (!CopyFile(pathToSelf.wstring().data(), _newFileName, true))
	{
		EW_LOG_FUNC(L"File with selected name already exists!");

		return false;
	}

	const std::filesystem::path currentPath = std::filesystem::current_path();
	std::filesystem::path pathToExeFile = currentPath / _newFileName;

	DWORD fileAttr = GetFileAttributes(pathToExeFile.wstring().data());

	if (fileAttr == 0xffffffff)
	{
		EW_LOG_FUNC(L"Unable to find copied .exe file");

		return false;
	}

	newExePath_ = std::move(pathToExeFile);

	return true;
}

bool ExeWrapperFunctions::RemoveFile(const std::filesystem::path& _path)
{
	if (!std::filesystem::remove(_path))
	{
		EW_LOG_FUNC(L"Can't remove the file %s!", _path.wstring().data());
		return false;
	}

	return true;
}