// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandPromptProfile.h"

namespace
{

std::optional<std::filesystem::path> GetCommandPromptPath()
{
	wchar_t systemDirectory[MAX_PATH];
	UINT systemDirectoryLength = GetSystemDirectory(systemDirectory, std::size(systemDirectory));

	if (systemDirectoryLength == 0 || systemDirectoryLength >= std::size(systemDirectory))
	{
		return std::nullopt;
	}

	return std::filesystem::path(systemDirectory) / L"cmd.exe";
}

std::wstring QuoteArgument(const std::filesystem::path &argument)
{
	return L"\"" + argument.wstring() + L"\"";
}

bool IsBatchFile(const std::filesystem::path &path)
{
	auto extension = path.extension().wstring();
	return _wcsicmp(extension.c_str(), L".bat") == 0 || _wcsicmp(extension.c_str(), L".cmd") == 0;
}

TerminalProcessLaunchInfo BuildProcessLaunchInfo(const std::filesystem::path &cmdPath,
	const std::filesystem::path &directory, std::wstring command)
{
	TerminalProcessLaunchInfo process;
	process.application = cmdPath;
	process.commandLine = QuoteArgument(cmdPath);
	process.workingDirectory = directory;

	if (PathIsUNC(directory.c_str()))
	{
		process.commandLine += L" " + command + L" pushd " + QuoteArgument(directory) + L" & ";
		process.workingDirectory = cmdPath.parent_path();
	}
	else
	{
		process.commandLine += L" " + command + L" ";
	}

	return process;
}

}

std::optional<TerminalLaunchRequest> CommandPromptProfile::CreateInteractive(
	const std::filesystem::path &directory)
{
	auto cmdPath = GetCommandPromptPath();

	if (!cmdPath)
	{
		return std::nullopt;
	}

	TerminalLaunchRequest request;
	request.initialDirectory = directory;
	request.process = BuildProcessLaunchInfo(*cmdPath, directory, L"/K");
	request.process.commandLine += L"prompt $E]9;9;$P$E\\$P$G";
	request.exitBehavior = TerminalExitBehavior::KeepTabOpen;
	return request;
}

std::optional<TerminalLaunchRequest> CommandPromptProfile::CreateBatchFile(
	const std::filesystem::path &itemPath, const std::wstring &parameters,
	const std::filesystem::path &workingDirectory)
{
	if (!IsBatchFile(itemPath))
	{
		return std::nullopt;
	}

	auto batchPath = itemPath;

	if (batchPath.is_relative() && !workingDirectory.empty())
	{
		batchPath = workingDirectory / batchPath;
	}

	batchPath = batchPath.lexically_normal();
	auto batchDirectory = batchPath.parent_path();

	if (batchDirectory.empty())
	{
		return std::nullopt;
	}

	auto cmdPath = GetCommandPromptPath();

	if (!cmdPath)
	{
		return std::nullopt;
	}

	TerminalLaunchRequest request;
	request.initialDirectory = batchDirectory;
	request.process = BuildProcessLaunchInfo(*cmdPath, batchDirectory, L"/C");
	request.process.commandLine += L"call " + QuoteArgument(batchPath);

	if (!parameters.empty())
	{
		request.process.commandLine += L" " + parameters;
	}

	request.exitBehavior = TerminalExitBehavior::CloseTab;
	return request;
}
