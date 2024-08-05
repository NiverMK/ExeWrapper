#include "GroupIconResourceLibrary.h"
#include "../../DebugLibrary/DebugLibrary.h"

GroupIconResData::GroupIconResData(const GroupIconResData& _groupIconResData)
	: grpIconResourceName(_groupIconResData.grpIconResourceName)
	, grpIconResource(_groupIconResData.grpIconResource)
	, iconResources(_groupIconResData.iconResources)
{
	if (grpIconResource.length())
	{
		pGroupIconHeader = reinterpret_cast<PGRPICONHEADER>(grpIconResource.data());
		pGroupIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(grpIconResource.data() + sizeof(GRPICONHEADER));
	}
}

GroupIconResData::GroupIconResData(GroupIconResData&& _groupIconResData) noexcept
{
	grpIconResourceName = std::move(_groupIconResData.grpIconResourceName);
	grpIconResource = std::move(_groupIconResData.grpIconResource);
	iconResources = std::move(_groupIconResData.iconResources);

	if (grpIconResource.length())
	{
		pGroupIconHeader = reinterpret_cast<PGRPICONHEADER>(grpIconResource.data());
		pGroupIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(grpIconResource.data() + sizeof(GRPICONHEADER));
	}
}

GroupIconResData& GroupIconResData::operator=(const GroupIconResData& _groupIconResData)
{
	grpIconResourceName = _groupIconResData.grpIconResourceName;
	grpIconResource = _groupIconResData.grpIconResource;
	iconResources = _groupIconResData.iconResources;

	if (grpIconResource.length())
	{
		pGroupIconHeader = reinterpret_cast<PGRPICONHEADER>(grpIconResource.data());
		pGroupIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(grpIconResource.data() + sizeof(GRPICONHEADER));
	}

	return *this;
}

GroupIconResData& GroupIconResData::operator=(GroupIconResData&& _groupIconResData) noexcept
{
	grpIconResourceName = std::move(_groupIconResData.grpIconResourceName);
	grpIconResource = std::move(_groupIconResData.grpIconResource);
	iconResources = std::move(_groupIconResData.iconResources);

	if (grpIconResource.length())
	{
		pGroupIconHeader = reinterpret_cast<PGRPICONHEADER>(grpIconResource.data());
		pGroupIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(grpIconResource.data() + sizeof(GRPICONHEADER));
	}

	return *this;
}

GroupIconResourceGetter::GroupIconResourceGetter(const wchar_t* _pathToExe)
	: resGetter(_pathToExe)
{
}

bool GroupIconResourceGetter::ReadGrpIconResources(const bool readIcons, GroupIconResData& grpIconResData_) const
{
	if (!resGetter.IsValid())
	{
		return false;
	}

	std::vector<ResourceName> grpIconResources;

	if (!resGetter.GetResourcesByType(RT_GROUP_ICON, grpIconResources) || !grpIconResources.size())
	{
		/* there are no group icons in .exe file */
		return false;
	}

	std::string groupIconResource;

	/* get first RT_GROUP_ICON resource */
	if (!resGetter.GetResourceFromExeFile(grpIconResources.front(), RT_GROUP_ICON, groupIconResource))
	{
		return false;
	}

	GroupIconResData grpIconResData;
	grpIconResData.SetResourceName(grpIconResources.front());
	grpIconResData.SetGroupIconResourceData(groupIconResource);
	
	if (readIcons)
	{
		std::vector<std::string> iconResources;
		iconResources.reserve(grpIconResData.GetGroupIconHeader()->ImageCount);
	
		for (int index = 0; index < grpIconResData.GetGroupIconHeader()->ImageCount; index++)
		{
			iconResources.push_back({});
			if (!resGetter.GetResourceFromExeFile(grpIconResData.GetGroupIconDirEntry()[index].nId, RT_ICON, iconResources.back()))
			{
				return false;
			}
		}

		grpIconResData.SetIconResourcesData(iconResources);
	}
	
	grpIconResData_ = grpIconResData;

	return true;
}

GroupIconResourceSetter::GroupIconResourceSetter(const wchar_t* _pathToExe)
	: resSetter(_pathToExe)
{
}

bool GroupIconResourceSetter::SetGrpIconResources(const GroupIconResData& _grpIconResData)
{
	if (!resSetter.IsValid())
	{
		EW_LOG_FUNC(L"resSetter is invalid!");
		return false;
	}

	const PGRPICONHEADER groupIconHeader = _grpIconResData.GetGroupIconHeader();
	const PGRPICONDIRENTRY groupIconDirEntry = _grpIconResData.GetGroupIconDirEntry();
	const std::vector<std::string>& iconResources = _grpIconResData.GetIconResources();

	for (int index = 0; index < groupIconHeader->ImageCount; index++)
	{
		if (!resSetter.SetResourceIntoExeFile(groupIconDirEntry[index].nId, RT_ICON, iconResources[index]))
		{
			return false;
		}
	}

	if (!resSetter.SetResourceIntoExeFile(_grpIconResData.GetResourceName(), RT_GROUP_ICON, _grpIconResData.GetGroupIconResource()))
	{
		return false;
	}

	return true;
}

bool GroupIconResourceSetter::CleanGrpIconResources(const GroupIconResData& _grpIconResData)
{
	if (!resSetter.IsValid())
	{
		EW_LOG_FUNC(L"resSetter is invalid!");
		return false;
	}

	const PGRPICONHEADER groupIconHeader = _grpIconResData.GetGroupIconHeader();
	const PGRPICONDIRENTRY groupIconDirEntry = _grpIconResData.GetGroupIconDirEntry();
	const std::vector<std::string>& iconResources = _grpIconResData.GetIconResources();

	for (int index = 0; index < groupIconHeader->ImageCount; index++)
	{
		if (!resSetter.SetResourceIntoExeFile(groupIconDirEntry[index].nId, RT_ICON, std::string()))
		{
			return false;
		}
	}

	if (!resSetter.SetResourceIntoExeFile(_grpIconResData.GetResourceName(), RT_GROUP_ICON, std::string()))
	{
		return false;
	}

	return true;
}