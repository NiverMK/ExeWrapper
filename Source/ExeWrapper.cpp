#include "ExeWrapperLibrary/ExeWrapperLibrary.h"
#include "ResourcesLibrary/ResourcesLibrary.h"
#include "DebugLibrary/DebugLibrary.h"
#include "../Resources/resource.h"

#define WRAP_NEW_EXE "--wrap"
#define DEFAULT_BIN_SIZE 1

int main(int argc, char* argv[])
{
	bool startWrapping = false;

	for (int index = 0; index < argc; index++)
	{
		std::string str = argv[index];

		if (strstr(argv[index], WRAP_NEW_EXE))
		{
			EW_LOG(L"Launch param WRAP_NEW_EXE was found; starting wrapping procedure");

			startWrapping = true;
			break;
		}
	}

	if (!startWrapping)
	{
		ResourceGetter binGetter{ nullptr };
		DWORD binSize = binGetter.GetResourceSize(IDR_BIN1, IDR_BIN1_TYPE);

		if (binSize == DEFAULT_BIN_SIZE)
		{
			EW_LOG(L"There is no wrapped .exe; starting wrapping procedure");

			startWrapping = true;
		}
	}

	if (startWrapping)
	{
		if (ExeWrapperFunctions::WrapExeFile())
		{
			EW_LOG(L"Wrapping was successful!");
		}

		system("pause");
	}
	else
	{
		/* wrapped .exe was found; trying to launch it */
		if (!ExeWrapperFunctions::RunWrappedProcess())
		{
			system("pause");
		}
	}

	return 0;
}
