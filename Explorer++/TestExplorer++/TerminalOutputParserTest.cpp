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
	m_parser.Process(L"before\x1b]9;9;C:\\Work\aafter");

	EXPECT_EQ(m_directories, std::vector<std::wstring>{ L"C:\\Work" });
}

TEST_F(TerminalOutputParserTest, ParsesStringTerminatedDirectory)
{
	m_parser.Process(L"\x1b]9;9;D:\\Source\x1b\\");

	EXPECT_EQ(m_directories, std::vector<std::wstring>{ L"D:\\Source" });
}

TEST_F(TerminalOutputParserTest, ParsesFragmentedSequence)
{
	m_parser.Process(L"output\x1b]9");
	m_parser.Process(L";9;C:\\Fragmented");
	m_parser.Process(L"\a");

	EXPECT_EQ(m_directories, std::vector<std::wstring>{ L"C:\\Fragmented" });
}

TEST_F(TerminalOutputParserTest, ParsesMultipleSequences)
{
	m_parser.Process(L"\x1b]9;9;C:\\One\aoutput\x1b]9;9;D:\\Two\a");

	EXPECT_EQ(m_directories, (std::vector<std::wstring>{ L"C:\\One", L"D:\\Two" }));
}

TEST_F(TerminalOutputParserTest, IgnoresUnrelatedOutput)
{
	m_parser.Process(L"ordinary command output");

	EXPECT_TRUE(m_directories.empty());
}
