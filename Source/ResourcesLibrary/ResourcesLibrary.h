#pragma once

#include <string>
#include <vector>
#include <windows.h>

struct ResourceName
{
	ResourceName(const wchar_t* _resName = nullptr);
	ResourceName(const ResourceName& _resName);
	ResourceName(ResourceName&& _resName) noexcept;
	~ResourceName();

	ResourceName& operator=(const ResourceName& _resName);
	ResourceName& operator=(ResourceName&& _resName) noexcept;

	operator const wchar_t*() const { return resNamePtr; }

protected:
	wchar_t* resNamePtr = nullptr;
};

class ResourceGetter
{
public:
	explicit ResourceGetter(const wchar_t* _pathToExe);
	~ResourceGetter();

	bool GetResourcesByType(LPCWSTR _lpType, std::vector<ResourceName>& resources_) const;

	DWORD GetResourceSize(const int _resourceName, const wchar_t* _resourceType) const;
	DWORD GetResourceSize(const wchar_t* _resourceName, const wchar_t* _resourceType) const;

	bool GetResourceFromExeFile(const int _resourceName, const wchar_t* _resourceType, std::string& outRes_) const;
	bool GetResourceFromExeFile(const wchar_t* _resourceName, const wchar_t* _resourceType, std::string& outRes_) const;

	bool IsValid() const;

	bool ReleaseLibrary();

protected:
	static BOOL EnumNamesFunc(HMODULE _hModule, LPCTSTR _lpType, LPTSTR _lpName, LONG _lParam);

	std::wstring pathToExeFile;
	HINSTANCE hExe = NULL;

	static std::vector<ResourceName> ResourcesByType;
};

class ResourceSetter
{
public:
	explicit ResourceSetter(const wchar_t* _pathToExe);
	~ResourceSetter();

	bool SetResourceIntoExeFile(const int _resourceName, const wchar_t* _resourceType, const std::string& _bytes);
	bool SetResourceIntoExeFile(const wchar_t* _resourceName, const wchar_t* _resourceType, const std::string& _bytes);

	bool IsValid() const;
	bool ReleaseHandle();

protected:
	std::wstring pathToExeFile;
	HANDLE hUpdateRes = NULL;
};
