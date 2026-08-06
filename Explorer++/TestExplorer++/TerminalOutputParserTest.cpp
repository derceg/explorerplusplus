// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TerminalOutputParser.h"
#include <gtest/gtest.h>

using namespace testing;

class TerminalOutputParserTest : public Test
{
protected:
	TerminalOutputParserTest() :
		m_parser([this](const std::wstring &directory) { m_directories.push_back(directory); })
	{
	}

	std::vector<std::wstring> m_directories;
	TerminalOutputParser m_parser;
};

TEST_F(TerminalOutputParserTest, ParsesBellTerminatedDirectory)
{
	auto output = m_parser.Process(L"before\x1b]9;9;C:\\Work\aafter");

	EXPECT_EQ(m_directories, std::vector<std::wstring>{ L"C:\\Work" });
	EXPECT_EQ(output, L"beforeafter");
}

TEST_F(TerminalOutputParserTest, ParsesStringTerminatedDirectory)
{
	auto output = m_parser.Process(L"\x1b]9;9;D:\\Source\x1b\\");

	EXPECT_EQ(m_directories, std::vector<std::wstring>{ L"D:\\Source" });
	EXPECT_TRUE(output.empty());
}

TEST_F(TerminalOutputParserTest, ParsesFragmentedSequence)
{
	auto firstOutput = m_parser.Process(L"output\x1b]9");
	auto secondOutput = m_parser.Process(L";9;C:\\Fragmented");
	auto thirdOutput = m_parser.Process(L"\a");

	EXPECT_EQ(m_directories, std::vector<std::wstring>{ L"C:\\Fragmented" });
	EXPECT_EQ(firstOutput, L"output");
	EXPECT_TRUE(secondOutput.empty());
	EXPECT_TRUE(thirdOutput.empty());
}

TEST_F(TerminalOutputParserTest, ParsesMultipleSequences)
{
	auto output = m_parser.Process(L"\x1b]9;9;C:\\One\aoutput\x1b]9;9;D:\\Two\a");

	EXPECT_EQ(m_directories, (std::vector<std::wstring>{ L"C:\\One", L"D:\\Two" }));
	EXPECT_EQ(output, L"output");
}

TEST_F(TerminalOutputParserTest, IgnoresUnrelatedOutput)
{
	auto output = m_parser.Process(L"ordinary command output");

	EXPECT_TRUE(m_directories.empty());
	EXPECT_EQ(output, L"ordinary command output");
}

TEST_F(TerminalOutputParserTest, PreservesFragmentThatIsNotDirectorySequence)
{
	auto firstOutput = m_parser.Process(L"ordinary output\x1b");
	auto secondOutput = m_parser.Process(L"[31mcolored output");

	EXPECT_EQ(firstOutput, L"ordinary output");
	EXPECT_EQ(secondOutput, L"\x1b[31mcolored output");
	EXPECT_TRUE(m_directories.empty());
}
