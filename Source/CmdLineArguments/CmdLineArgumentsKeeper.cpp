#include "CmdLineArgumentsKeeper.h"
#include "../CommonLibrary/CommonLibrary.h"
#include "../DebugLibrary/DebugLibrary.h"

#include <windows.h>

std::map<size_t, CmdLineArgumentBase*> CmdLineArgumentsKeeper::cmdLaunchParams;

bool CLA_Wrap::ParseCommandLine(int argc, char* argv[], std::vector<bool>& _result_)
{
	bool hasFlag = false;

	for (int index = 0; index < argc; index++)
	{
		std::wstring command;
		if (!CommonLibrary::ConvertMultiByteToWideChar(argv[index], true, command))
		{
			EW_LOG_WIN_ERROR(L"Something went wrong! Skipping argv %hs", argv[index]);
			continue;
		}

		if (hasFlag)
		{
			path = std::move(command);
			break;
		}
		else if (!wcscmp(command.data(), wrapCmdName.data()))
		{
			hasFlag = true;
			_result_[index] = true;
		}
	}

	return hasFlag;
}

bool CLA_Unwrap::ParseCommandLine(int argc, char* argv[], std::vector<bool>& _result_)
{
	bool hasFlag = false;

	for (int index = 0; index < argc; index++)
	{
		std::wstring command;
		if (!CommonLibrary::ConvertMultiByteToWideChar(argv[index], true, command))
		{
			EW_LOG_WIN_ERROR(L"Something went wrong! Skipping argv %hs", argv[index]);
			continue;
		}

		if (hasFlag)
		{
			path = std::move(command);
			break;
		}
		else if (!wcscmp(command.data(), unwrapCmdName.data()))
		{
			hasFlag = true;
			_result_[index] = true;
		}
	}

	return hasFlag;
}

CmdLineArgumentsKeeper& CmdLineArgumentsKeeper::GetCmdLaunchParamsKeeper()
{
	static CmdLineArgumentsKeeper selfObject;

	return selfObject;
}

void CmdLineArgumentsKeeper::ParseCmdParams(int argc, char* argv[])
{
	std::vector<bool> cmdInfo(argc, false);

	for (CmdLineArgumentBase* cmdParam : paramsArray)
	{
		if (cmdParam->ParseCommandLine(argc, argv, cmdInfo))
		{
			cmdLaunchParams.insert({ cmdParam->GetHash(),cmdParam });
		}
	}

	/* adding .exe path with quotes into processLaunchArgs.
	otherwise in launched process argv[0] will be empty if argc > 1 */
	std::wstring command;
	CommonLibrary::ConvertMultiByteToWideChar(argv[0], true, command);
	processLaunchArgs.append(L"\"");
	processLaunchArgs.append(command);
	processLaunchArgs.append(L"\"");

	for (int index = 1; index < argc; index++)
	{
		if (!cmdInfo[index])
		{
			CommonLibrary::ConvertMultiByteToWideChar(argv[index], true, command);
			processLaunchArgs.append(L" ");
			processLaunchArgs.append(command);
		}
	}
}
