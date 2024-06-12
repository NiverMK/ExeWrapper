#include "ExeWrapperLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"
#include "../../Resources/resource.h"
#include "../ProcessLibrary/ProcessLibrary.h"
#include "../ResourcesLibrary/ResourcesLibrary.h"
#include "../ResourcesLibrary/ResourcesLibStructs.h"

#include <sstream>
#include <fstream>

bool ExeWrapperFunctions::WrapExeFile()
{
	EW_LOG(L"Drag here .exe file");

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
	std::filesystem::path newExeFilePath;

	if (!DuplicateSelfExeFile(newExeFileName.data(), newExeFilePath))
	{
		return false;
	}
	
	{
		ResourceSetter newExeResourceSetter{ newExeFilePath.wstring().data() };

		if (!newExeResourceSetter.IsValid())
		{
			return false;
		}

		if (!newExeResourceSetter.SetResourceIntoExeFile(IDR_BIN1, IDR_BIN1_TYPE, exeBytes))
		{
			return false;
		}
	}

	if (!TransferIcons(pathToExeFile.data(), newExeFilePath.wstring().data()))
	{
		return false;
	}

	return true;
}

bool ExeWrapperFunctions::RunWrappedProcess()
{
	ResourceGetter newExeResourceSetter{ nullptr };

	std::string exeBytes;
	if (!newExeResourceSetter.GetResourceFromExeFile(IDR_BIN1, IDR_BIN1_TYPE, exeBytes))
	{
		return false;
	}

	return ProcessLibrary::CreateProcessFromRam(exeBytes.data());
}

bool ExeWrapperFunctions::GetExeFileBytes(const std::wstring& _pathToExeFile, std::string& outBytes_)
{
	std::ifstream exeFile;
	exeFile.open(_pathToExeFile, std::ios::binary | std::ios::ate);

	if (!exeFile.is_open())
	{
		EW_LOG_FUNC(L"Cannot open the file!");

		return false;
	}

	std::streamsize fileSize = exeFile.tellg();
	exeFile.seekg(0, std::ios::beg);

	outBytes_.resize(fileSize);

	if (!exeFile.read(outBytes_.data(), fileSize))
	{
		EW_LOG_FUNC(L"Cannot read the file!");

		return true;
	};

	return true;
}

bool ExeWrapperFunctions::TransferIcons(const wchar_t* _sourceExeFile, const wchar_t* _destExeFile)
{
	/* get image resources info from source .exe */
	std::string sourceGrpIconResBytes;
	std::vector<std::string> sourceIconResources;

	PGRPICONHEADER pSrcGrpIconHeader = nullptr;
	PGRPICONDIRENTRY pSrcGrpIconDirEntry = nullptr;

	{
		ResourceGetter sourceExeFileGetter(_sourceExeFile);

		if (!sourceExeFileGetter.IsValid())
		{
			return false;
		}

		std::vector<ResourceName> grpIconResources;

		if (!sourceExeFileGetter.GetResourcesByType(RT_GROUP_ICON, grpIconResources) || !grpIconResources.size())
		{
			/* there are no icons in .exe file */
			return false;
		}

		/* get first RT_GROUP_ICON resource */
		if (!sourceExeFileGetter.GetResourceFromExeFile(grpIconResources[0], RT_GROUP_ICON, sourceGrpIconResBytes))
		{
			return false;
		}

		pSrcGrpIconHeader = reinterpret_cast<PGRPICONHEADER>(sourceGrpIconResBytes.data());
		pSrcGrpIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(sourceGrpIconResBytes.data() + sizeof(GRPICONHEADER));

		sourceIconResources.reserve(pSrcGrpIconHeader->ImageCount);

		for (int index = 0; index < pSrcGrpIconHeader->ImageCount; index++)
		{
			sourceIconResources.push_back({});
			if (!sourceExeFileGetter.GetResourceFromExeFile(pSrcGrpIconDirEntry[index].nId, RT_ICON, sourceIconResources.back()))
			{
				return false;
			}
		}
	}

	/* get group icons info from destination .exe file */
	std::string destGrpIconResBytes;
	ResourceName destGrpIconResource;

	PGRPICONHEADER pDestGrpIconHeader = nullptr;
	PGRPICONDIRENTRY pDestGrpIconDirEntry = nullptr;

	{
		ResourceGetter destExeFileGetter(_destExeFile);
		std::vector<ResourceName> destGrpIconResources;

		if (!destExeFileGetter.GetResourcesByType(RT_GROUP_ICON, destGrpIconResources) || !destGrpIconResources.size())
		{
			/* there are no icons in .exe file */
			return false;
		}

		destGrpIconResource = destGrpIconResources.front();

		if (!destExeFileGetter.GetResourceFromExeFile(destGrpIconResource, RT_GROUP_ICON, destGrpIconResBytes))
		{
			return false;
		}

		pDestGrpIconHeader = reinterpret_cast<PGRPICONHEADER>(destGrpIconResBytes.data());
		pDestGrpIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(destGrpIconResBytes.data() + sizeof(GRPICONHEADER));

		/* free .exe before writing into it */
	}

	/* transfer icons */
	{
		std::string newDestGrpIconResBytes;
		newDestGrpIconResBytes.reserve(sourceGrpIconResBytes.size());
		newDestGrpIconResBytes.append(destGrpIconResBytes.data(), sizeof(GRPICONHEADER));
		PGRPICONHEADER pNewDestGrpIconHeader = reinterpret_cast<PGRPICONHEADER>(newDestGrpIconResBytes.data());
		pNewDestGrpIconHeader->ImageCount = 0;

		ResourceSetter destSetter(_destExeFile);

		if (!destSetter.IsValid())
		{
			return false;
		}

		int commonIconsAmount = pSrcGrpIconHeader->ImageCount < pDestGrpIconHeader->ImageCount 
			? pSrcGrpIconHeader->ImageCount 
			: pDestGrpIconHeader->ImageCount;

		/* first - wrap common amount of icons in dest exe */
		for (int index = 0; index < commonIconsAmount; index++)
		{
			if (!destSetter.SetResourceIntoExeFile(pDestGrpIconDirEntry[index].nId, RT_ICON, sourceIconResources[index]))
			{
				return false;
			}

			/* update PGRPICONHEADER with new info and old id */
			GRPICONDIRENTRY grpIconDirEntry = pSrcGrpIconDirEntry[index];
			grpIconDirEntry.nId = pDestGrpIconDirEntry[index].nId;
			newDestGrpIconResBytes.append(static_cast<char*>(static_cast<void*>(&grpIconDirEntry)), sizeof(grpIconDirEntry));
			pNewDestGrpIconHeader->ImageCount++;
		}

		/* second - try to balance images amount */
		if (pSrcGrpIconHeader->ImageCount < pDestGrpIconHeader->ImageCount)
		{
			/* delete extra icons */
			for (int index = commonIconsAmount; index < pDestGrpIconHeader->ImageCount; index++)
			{
				destSetter.SetResourceIntoExeFile(pDestGrpIconDirEntry[index].nId, RT_ICON, std::string());
			}
		}
		else if (pSrcGrpIconHeader->ImageCount > pDestGrpIconHeader->ImageCount)
		{
			/* try to add next icons into resources */
			for (int index = commonIconsAmount; index < pSrcGrpIconHeader->ImageCount; index++)
			{
				GRPICONDIRENTRY grpIconDirEntry = pSrcGrpIconDirEntry[index];
				grpIconDirEntry.nId = pDestGrpIconDirEntry[index - 1].nId + 1;

				if (!destSetter.SetResourceIntoExeFile(grpIconDirEntry.nId, RT_ICON, sourceIconResources[index]))
				{					
					EW_LOG_FUNC(L"Can't add extra icon with Id %u", grpIconDirEntry.nId);
					break;
				}

				newDestGrpIconResBytes.append(static_cast<char*>(static_cast<void*>(&grpIconDirEntry)), sizeof(grpIconDirEntry));
				pNewDestGrpIconHeader->ImageCount++;
			}
		}

		/* third - write updated PGRPICONHEADER */
		if (!destSetter.SetResourceIntoExeFile(destGrpIconResource, RT_GROUP_ICON, newDestGrpIconResBytes))
		{
			return false;
		}
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

	wchar_t* pathToFile = new wchar_t[MAX_PATH];
	GetModuleFileName(NULL, pathToFile, MAX_PATH);

	const std::wstring pathToFileStr(pathToFile);
	delete[] pathToFile;

	if (!CopyFile(pathToFileStr.data(), _newFileName, true))
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