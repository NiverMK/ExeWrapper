#pragma once

#include <map>
#include <string>
#include <vector>

static const std::wstring emptyString;
constexpr std::wstring_view wrapCmdName = L"--wrap";
constexpr std::wstring_view unwrapCmdName = L"--unwrap";

constexpr size_t WstringHash(const std::wstring_view& _str)
{
	size_t hash = 5381ULL;

	for (size_t index = 0; index < _str.length(); index++)
	{
		const size_t wchar = _str.data()[index];
		hash = hash * 33ULL + wchar;
	}

	return hash;
}

struct CmdLineArgumentBase
{
	virtual constexpr size_t GetHash() const { return std::string::npos; }
	virtual bool ParseCommandLine(int argc, char* argv[], std::vector<bool>& _result_) { return false; };
	virtual const std::wstring& GetValue() { return emptyString; }
};

template<size_t hash>
struct CmdLineArgument : public CmdLineArgumentBase
{
	static size_t Static_GetHash() { return hash; }
	constexpr virtual size_t GetHash() const override { return hash; }
};

struct CLA_Wrap final : public CmdLineArgument<WstringHash(wrapCmdName)>
{
	virtual bool ParseCommandLine(int argc, char* argv[], std::vector<bool>& _result_) override;
	virtual const std::wstring& GetValue() override { return path; }

protected:
	std::wstring path;
};

struct CLA_Unwrap final : public CmdLineArgument<WstringHash(unwrapCmdName)>
{
	virtual bool ParseCommandLine(int argc, char* argv[], std::vector<bool>& _result_) override;
	virtual const std::wstring& GetValue() override { return path; }

protected:
	std::wstring path;
};

struct CmdLineArgumentsKeeper
{
public:
	static CmdLineArgumentsKeeper& GetCmdLaunchParamsKeeper();
	
	void ParseCmdParams(int argc, char* argv[]);
	
	template<typename T>
	bool HasLaunchParam()
	{
		return cmdLaunchParams.find(T::Static_GetHash()) != cmdLaunchParams.end();
	}

	template<typename T>
	const std::wstring& GetParamValue()
	{
		auto param = cmdLaunchParams.find(T::Static_GetHash());

		return param != cmdLaunchParams.end() ? param->second->GetValue() : emptyString;
	}

	const std::wstring& GetProcessLaunchArgs() const { return processLaunchArgs; }

protected:
	constexpr CmdLineArgumentsKeeper()
	{
		paramsArray = { &claWrap, &claUnwrap };
	}

	/* std::map hasn't constexpr constructor */
	static std::map<size_t, CmdLineArgumentBase*> cmdLaunchParams;
	std::wstring processLaunchArgs;

private:
	std::vector<CmdLineArgumentBase*> paramsArray;
	CLA_Wrap claWrap;
	CLA_Unwrap claUnwrap;
};