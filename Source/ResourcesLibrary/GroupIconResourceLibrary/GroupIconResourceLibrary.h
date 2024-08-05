#pragma once

#include "../ResourcesLibrary.h"
#include "../ResourcesLibStructs.h"

struct GroupIconResData
{
	GroupIconResData() {}
	GroupIconResData(const GroupIconResData& _groupIconResData);
	GroupIconResData(GroupIconResData&& _groupIconResData) noexcept;

	GroupIconResData& operator=(const GroupIconResData& _groupIconResData);
	GroupIconResData& operator=(GroupIconResData&& _groupIconResData) noexcept;

	void SetResourceName(const ResourceName& _grpIconResourceName) { grpIconResourceName = _grpIconResourceName; }
	void SetIconResourcesData(const std::vector<std::string>& _iconResources) { iconResources = _iconResources; }
	void SetGroupIconResourceData(const std::string& _grpIconResBytes)
	{ 
		grpIconResource = _grpIconResBytes;

		if (grpIconResource.length())
		{
			pGroupIconHeader = reinterpret_cast<PGRPICONHEADER>(grpIconResource.data());
			pGroupIconDirEntry = reinterpret_cast<PGRPICONDIRENTRY>(grpIconResource.data() + sizeof(GRPICONHEADER));
		}
	}

	bool HasGroupData() const { return grpIconResource.length(); }
	bool HasIcons() const { return HasGroupData() && iconResources.size(); }

	const ResourceName& GetResourceName() const { return grpIconResourceName; }
	const std::string& GetGroupIconResource() const { return grpIconResource; }
	const std::vector<std::string>& GetIconResources() const { return iconResources; }

	const PGRPICONHEADER GetGroupIconHeader() const  { return pGroupIconHeader; }
	const PGRPICONDIRENTRY GetGroupIconDirEntry() const { return pGroupIconDirEntry; }

	PGRPICONHEADER GetGroupIconHeader() { return pGroupIconHeader; }
	PGRPICONDIRENTRY GetGroupIconDirEntry() { return pGroupIconDirEntry; }

protected:
	ResourceName grpIconResourceName;
	std::string grpIconResource;
	std::vector<std::string> iconResources;

private:
	PGRPICONHEADER pGroupIconHeader = nullptr;
	PGRPICONDIRENTRY pGroupIconDirEntry = nullptr;
};

class GroupIconResourceGetter
{
public:
	GroupIconResourceGetter(const wchar_t* _pathToExe);

	bool Release() { return resGetter.ReleaseLibrary(); }

	bool ReadGrpIconResources(const bool readIcons, GroupIconResData& grpIconResData_) const;

	bool IsValid() const { return resGetter.IsValid(); }

protected:
	ResourceGetter resGetter;
};

class GroupIconResourceSetter
{
public:
	GroupIconResourceSetter(const wchar_t* _pathToExe);

	bool Release() { return resSetter.ReleaseHandle(); }

	bool SetGrpIconResources(const GroupIconResData& _grpIconResData);
	bool CleanGrpIconResources(const GroupIconResData& _grpIconResData);

	bool IsValid() const { return resSetter.IsValid(); }

protected:
	ResourceSetter resSetter;
};