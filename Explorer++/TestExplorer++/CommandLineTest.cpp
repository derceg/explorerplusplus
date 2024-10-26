// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CommandLine.h"
#include <gtest/gtest.h>

using namespace testing;

class CommandLineTest : public Test
{
protected:
	CommandLine::Settings ParseCommandLine(const std::wstring &commandLine)
	{
		CommandLine::Settings commandLineSettings;
		ParseCommandLineHelper(commandLine, commandLineSettings);
		return commandLineSettings;
	}

private:
	void ParseCommandLineHelper(const std::wstring &commandLine,
		CommandLine::Settings &outputCommandLineSettings)
	{
		auto commandLineInfo = CommandLine::Parse(commandLine);
		const auto *commandLineSettings = std::get_if<CommandLine::Settings>(&commandLineInfo);
		ASSERT_THAT(commandLineSettings, NotNull());
		outputCommandLineSettings = *commandLineSettings;
	}
};

TEST_F(CommandLineTest, Directories)
{
	auto commandLineSettings = ParseCommandLine(L"explorer++.exe c:\\path1 e:\\path2");
	EXPECT_THAT(commandLineSettings.directories, ElementsAre(L"c:\\path1", L"e:\\path2"));

	commandLineSettings =
		ParseCommandLine(LR"(explorer++.exe "c:\path with spaces 1" "f:\path with spaces 2")");
	EXPECT_THAT(commandLineSettings.directories,
		ElementsAre(L"c:\\path with spaces 1", L"f:\\path with spaces 2"));
}

TEST_F(CommandLineTest, FilesToSelect)
{
	auto commandLineSettings = ParseCommandLine(L"explorer++.exe --select c:\\windows\\system32");
	EXPECT_THAT(commandLineSettings.filesToSelect, ElementsAre(L"c:\\windows\\system32"));

	// This option can appear multiple times. In that situation, each path should be stored.
	commandLineSettings = ParseCommandLine(
		L"explorer++.exe --select c:\\windows\\system32 --select h:\\project\\file");
	EXPECT_THAT(commandLineSettings.filesToSelect,
		ElementsAre(L"c:\\windows\\system32", L"h:\\project\\file"));
}

TEST_F(CommandLineTest, Options)
{
	auto commandLineSettings = ParseCommandLine(L"explorer++.exe");
	EXPECT_FALSE(commandLineSettings.enableLogging);

	commandLineSettings = ParseCommandLine(L"explorer++.exe --enable-logging");
	EXPECT_TRUE(commandLineSettings.enableLogging);

	commandLineSettings = ParseCommandLine(L"explorer++.exe");
	EXPECT_THAT(commandLineSettings.enableFeatures, IsEmpty());

	commandLineSettings = ParseCommandLine(L"explorer++.exe --enable-features Plugins");
	EXPECT_THAT(commandLineSettings.enableFeatures, ElementsAre(Feature::Plugins));
}

TEST_F(CommandLineTest, CrashedData)
{
	CrashedData crashedData;
	crashedData.processId = 6821;
	crashedData.threadId = 83;
	crashedData.exceptionPointersAddress = 0x7f348129;
	crashedData.eventName = L"UniqueEventName";
	auto commandLineSettings = ParseCommandLine(std::format(L"explorer++.exe {} {}",
		CommandLine::APPLICATION_CRASHED_ARGUMENT, FormatCrashedDataForCommandLine(crashedData)));
	EXPECT_EQ(commandLineSettings.crashedData, crashedData);
}
