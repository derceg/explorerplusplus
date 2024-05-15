// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CommandLineSplitter.h"
#include <gtest/gtest.h>

using namespace testing;

class CommandLineSplitterTest : public Test
{
protected:
	void PerformSuccessTest(const std::string &commandLine,
		const std::vector<std::string> expectedArguments)
	{
		auto splitResult = CommandLineSplitter::Split(commandLine);
		EXPECT_TRUE(splitResult.succeeded);
		EXPECT_TRUE(splitResult.errorMessage.empty());
		EXPECT_EQ(splitResult.arguments, expectedArguments);
	}

	void PerformFailureTest(const std::string &commandLine)
	{
		auto splitResult = CommandLineSplitter::Split(commandLine);
		EXPECT_FALSE(splitResult.succeeded);
		EXPECT_FALSE(splitResult.errorMessage.empty());
		EXPECT_TRUE(splitResult.arguments.empty());
	}
};

TEST_F(CommandLineSplitterTest, PlainAndQuotedArguments)
{
	PerformSuccessTest(R"(C:\ E:\)", { "C:\\", "E:\\" });
	PerformSuccessTest(R"("C:\" "E:\")", { "C:\\", "E:\\" });
	PerformSuccessTest(R"(C:\path1 C:\path2)", { "C:\\path1", "C:\\path2" });
	PerformSuccessTest(R"("C:\Program Files" C:\path2)", { "C:\\Program Files", "C:\\path2" });
	PerformSuccessTest(R"(--command1 "C:\path\containing spaces" --command2 value)",
		{ "--command1", "C:\\path\\containing spaces", "--command2", "value" });
}

TEST_F(CommandLineSplitterTest, AdditionalWhitespace)
{
	// Additional whitespace between and after arguments should be ignored.
	PerformSuccessTest(R"(C:\    E:\    )", { "C:\\", "E:\\" });
	PerformSuccessTest(R"(        "C:\"    "E:\"             )", { "C:\\", "E:\\" });
}

TEST_F(CommandLineSplitterTest, ClosingQuoteMissing)
{
	PerformFailureTest(R"("C:\" "E:\)");
}

TEST_F(CommandLineSplitterTest, ExtraDataOutsideQuotes)
{
	PerformFailureTest(R"(C:\"E:\")");
	PerformFailureTest(R"("C:\"E:\)");
}

TEST_F(CommandLineSplitterTest, Empty)
{
	// An empty command line isn't valid (the executable name should always be present, so there
	// should always be at least one argument). But if the caller does pass an empty command line,
	// splitting it should fail.
	PerformFailureTest("");
	PerformFailureTest("    ");
}
