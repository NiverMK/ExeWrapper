#include "WinapiFunctions/WinApiFunctions.h"
#include "ExeWrapperFunctions/ExeWrapperFunctions.h"
#include "../Resources/resource.h"

#include <iostream>

int main()
{
	std::string unwrappedExe = WinApiFunctions::UnwrapResourceFromExeFile(nullptr, IDR_BIN1, IDR_BIN1_TYPE);

	if (unwrappedExe.length() == 1)
	{
		std::cout << "There is no wrapped .exe; starting wrapping procedure" << std::endl;

		ExeWrapperFunctions::WrapExeFile();

		system("pause");
	}
	else
	{
		/* wrapped .exe was found; trying to launch it */
		if (!ExeWrapperFunctions::RunWrappedProcess(unwrappedExe.data()))
		{
			system("pause");
		}
	}

	return 0;
}
