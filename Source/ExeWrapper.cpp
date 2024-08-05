#include "ExeWrapperLibrary/ExeWrapperLibrary.h"
#include "DebugLibrary/DebugLibrary.h"
#include "CmdLineArguments/CmdLineArgumentsKeeper.h"

int main(int argc, char* argv[])
{
	CmdLineArgumentsKeeper& cmdParser = CmdLineArgumentsKeeper::GetCmdLaunchParamsKeeper();
	cmdParser.ParseCmdParams(argc, argv);

	const bool hasUnwrapCmd = cmdParser.HasLaunchParam<CLA_Unwrap>();
	const bool hasWrapCmd = cmdParser.HasLaunchParam<CLA_Wrap>();
	const bool hasWrappedBin = ExeWrapperFunctions::HasWrappedExe();

	const bool startWrapping = !hasWrappedBin && (hasWrapCmd || !hasUnwrapCmd);
	const bool startUnwrapping = hasUnwrapCmd && !hasWrapCmd;

	if (startWrapping)
	{
		EW_LOG(L"Starting wrapping!");

		if (ExeWrapperFunctions::WrapExeFile())
		{
			EW_LOG(L"Wrapping was successful!");
		}

		system("pause");
	}
	else if (startUnwrapping)
	{
		EW_LOG(L"Starting unwrapping!");

		if (ExeWrapperFunctions::UnwrapExeFile())
		{
			EW_LOG(L"Unwrapping was successful!");
		}

		system("pause");
	}
	else
	{
		ExeWrapperFunctions::RunWrappedProcess();

	}

	return 0;
}
