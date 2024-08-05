#include "ResourcesLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"

std::vector<ResourceName> ResourceGetter::ResourcesByType;

ResourceName::ResourceName(const wchar_t* _resName)
{
	if (resNamePtr == _resName)
	{
		return;
	}

	if (IS_INTRESOURCE(_resName))
	{
		resNamePtr = const_cast<wchar_t*>(_resName);
	}
	else if (_resName[0] == L'#')
	{
		uint32_t resValue;
		swscanf_s(_resName + 1, L"%u", &resValue);

		resNamePtr = reinterpret_cast<wchar_t*>(resValue);
	}
	else
	{
		const size_t wcharsAmount = wcslen(_resName) + 1;
		resNamePtr = new wchar_t[wcharsAmount];
		wcscpy_s(resNamePtr, wcharsAmount, _resName);
		resNamePtr[wcharsAmount - 1] = L'\0';
	}
}

ResourceName::ResourceName(const ResourceName& _resName)
	: ResourceName(_resName.resNamePtr)
{
}

ResourceName::ResourceName(ResourceName&& _resName) noexcept
{
	resNamePtr = _resName.resNamePtr;
	_resName.resNamePtr = nullptr;
}

ResourceName::~ResourceName()
{
	if (!IS_INTRESOURCE(resNamePtr))
	{
		delete[] resNamePtr;
	}
}

ResourceName& ResourceName::operator=(const ResourceName& _resName)
{
	if (resNamePtr == _resName.resNamePtr)
	{
		return *this;
	}

	ResourceName obj(_resName);
	*this = std::move(obj);

	return *this;
}

ResourceName& ResourceName::operator=(ResourceName&& _resName) noexcept
{
	resNamePtr = _resName.resNamePtr;
	_resName.resNamePtr = nullptr;

	return *this;
}

ResourceGetter::ResourceGetter(const wchar_t* _pathToExe)
{
	if (_pathToExe)
	{
		pathToExeFile = std::wstring(_pathToExe);

		hExe = LoadLibrary(_pathToExe);

		if (hExe == NULL)
		{
			EW_LOG_WIN_ERROR(L"Failed to load library %s!", _pathToExe);
		}
	}
}

ResourceGetter::~ResourceGetter()
{
	ReleaseLibrary();
}

BOOL ResourceGetter::EnumNamesFunc(HMODULE _hModule, LPCTSTR _lpType, LPTSTR _lpName, LONG _lParam)
{
	ResourcesByType.push_back(ResourceName(_lpName));

	return true;
}

DWORD ResourceGetter::GetResourceSize(const int _resourceName, const wchar_t* _resourceType) const
{
	return GetResourceSize(MAKEINTRESOURCE(_resourceName), _resourceType);
}

DWORD ResourceGetter::GetResourceSize(const wchar_t* _resourceName, const wchar_t* _resourceType) const
{
	HRSRC hRes = FindResource(hExe, _resourceName, _resourceType);

	if (!hRes)
	{
		EW_LOG_WIN_ERROR(L"Failed to find resource %s in %s!", _resourceName, pathToExeFile.data());

		return 0;
	}

	return SizeofResource(hExe, hRes);
}

bool ResourceGetter::GetResourcesByType(LPCWSTR _lpType, std::vector<ResourceName>& resources_) const
{
	if (!EnumResourceNames(hExe, _lpType, (ENUMRESNAMEPROC)EnumNamesFunc, NULL))
	{
		EW_LOG_WIN_ERROR(L"Failed to get resources of type %lld in %s!", (uint64_t)_lpType, pathToExeFile.data());

		return false;
	}

	resources_ = std::move(ResourcesByType);

	return true;
}

bool ResourceGetter::GetResourceFromExeFile(const int _resourceName, const wchar_t* _resourceType, std::string& outRes_) const
{
	return GetResourceFromExeFile(MAKEINTRESOURCE(_resourceName), _resourceType, outRes_);
}

bool ResourceGetter::GetResourceFromExeFile(const wchar_t* _resourceName, const wchar_t* _resourceType, std::string& outRes_) const
{
	HRSRC hRes = FindResource(hExe, _resourceName, _resourceType);

	if (!hRes)
	{
		EW_LOG_WIN_ERROR(L"Failed to find resource %s in %s!", _resourceName, pathToExeFile.data());

		return false;
	}

	HGLOBAL hResourceLoaded = LoadResource(hExe, hRes);

	if (!hResourceLoaded)
	{
		EW_LOG_WIN_ERROR(L"Failed to load resource %s in %s!", _resourceName, pathToExeFile.data());

		return false;
	}

	void* resLock = LockResource(hResourceLoaded);

	if (!resLock)
	{
		EW_LOG_WIN_ERROR(L"Failed to lock resource %s in %s!", _resourceName, pathToExeFile.data());

		return false;
	}

	DWORD dwResSize = SizeofResource(hExe, hRes);
	outRes_ = std::string(reinterpret_cast<char*>(resLock), dwResSize);

	return true;
}

bool ResourceGetter::IsValid() const
{
	if (pathToExeFile.size())
	{
		return hExe != NULL;
	}

	/* if pathToExeFile is empty and hExe == NULL, it's valid ResourceGetter of self resources! */
	return true;
}

bool ResourceGetter::ReleaseLibrary()
{
	if (hExe)
	{
		if (FreeLibrary(hExe))
		{
			hExe = NULL;
			pathToExeFile.clear();

			return true;
		}
		else
		{
			EW_LOG_WIN_ERROR(L"Failed to free library %s!", pathToExeFile.data());

			return false;
		}
	}

	return false;
}

ResourceSetter::ResourceSetter(const wchar_t* _pathToExe)
	: pathToExeFile(_pathToExe)
{
	if (_pathToExe)
	{
		hUpdateRes = BeginUpdateResource(_pathToExe, FALSE);

		if (!hUpdateRes)
		{
			EW_LOG_WIN_ERROR(L"Failed to begin update resource %s!", pathToExeFile.data());
		}
	}
}

ResourceSetter::~ResourceSetter()
{
	ReleaseHandle();
}

bool ResourceSetter::SetResourceIntoExeFile(const int _resourceName, const wchar_t* _resourceType, const std::string& _bytes)
{
	return SetResourceIntoExeFile(MAKEINTRESOURCE(_resourceName), _resourceType, _bytes);
}

bool ResourceSetter::SetResourceIntoExeFile(const wchar_t* _resourceName, const wchar_t* _resourceType, const std::string& _bytes)
{
	DWORD bytesAmount = _bytes.length();
	LPVOID bytesPtr = bytesAmount > 0 ? const_cast<LPVOID>((const LPVOID)_bytes.data()) : nullptr;

	if (!UpdateResource(hUpdateRes, _resourceType,
		_resourceName,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		bytesPtr,
		bytesAmount))
	{
		EW_LOG_WIN_ERROR(L"Failed to update resource %s!", pathToExeFile.data());

		return false;
	}

	return true;
}

bool ResourceSetter::IsValid() const
{
	return hUpdateRes != NULL;
}

bool ResourceSetter::ReleaseHandle()
{
	if (hUpdateRes)
	{
		if (!EndUpdateResource(hUpdateRes, FALSE))
		{
			EW_LOG_WIN_ERROR(L"Failed to end update resource %s!", pathToExeFile.data());

			return false;
		}

		hUpdateRes = NULL;
		pathToExeFile.clear();

		return true;
	}

	return false;
}
