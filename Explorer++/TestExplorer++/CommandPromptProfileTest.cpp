// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CommandPromptProfile.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(CommandPromptProfileTest, InteractiveRequest)
{
	std::filesystem::path directory = LR"(C:\Working Directory)";
	auto request = CommandPromptProfile::CreateInteractive(directory);

	ASSERT_TRUE(request);
	EXPECT_EQ(request->initialDirectory, directory);
	EXPECT_EQ(request->process.workingDirectory, directory);
	EXPECT_EQ(request->exitBehavior, TerminalExitBehavior::KeepTabOpen);
	EXPECT_THAT(request->process.commandLine, HasSubstr(L" /K "));
	EXPECT_THAT(request->process.commandLine, HasSubstr(L"prompt $E]9;9;$P$E\\$P$G"));
}

TEST(CommandPromptProfileTest, BatchFileRequest)
{
	std::filesystem::path workingDirectory = LR"(C:\Working Directory)";
	auto request =
		CommandPromptProfile::CreateBatchFile(L"script.cmd", L"first second", workingDirectory);

	ASSERT_TRUE(request);
	EXPECT_EQ(request->initialDirectory, workingDirectory);
	EXPECT_EQ(request->process.workingDirectory, workingDirectory);
	EXPECT_EQ(request->exitBehavior, TerminalExitBehavior::CloseTab);
	EXPECT_THAT(request->process.commandLine, HasSubstr(L" /C "));
	EXPECT_THAT(request->process.commandLine,
		HasSubstr(LR"(call "C:\Working Directory\script.cmd" first second)"));
}

TEST(CommandPromptProfileTest, BatchExtensionIsCaseInsensitive)
{
	EXPECT_TRUE(CommandPromptProfile::CreateBatchFile(LR"(C:\Work\script.BAT)", L"", L""));
	EXPECT_TRUE(CommandPromptProfile::CreateBatchFile(LR"(C:\Work\script.CmD)", L"", L""));
}

TEST(CommandPromptProfileTest, RejectsNonBatchFile)
{
	EXPECT_FALSE(CommandPromptProfile::CreateBatchFile(LR"(C:\Work\script.exe)", L"", L""));
}

TEST(CommandPromptProfileTest, UncDirectoryUsesPushd)
{
	std::filesystem::path directory = LR"(\\server\share\folder)";
	auto request = CommandPromptProfile::CreateInteractive(directory);

	ASSERT_TRUE(request);
	EXPECT_EQ(request->initialDirectory, directory);
	EXPECT_NE(request->process.workingDirectory, directory);
	EXPECT_THAT(request->process.commandLine, HasSubstr(LR"(pushd "\\server\share\folder")"));
}
