// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TerminalClipboard.h"
#include "SimulatedClipboardStore.h"
#include "../Helper/Clipboard.h"
#include <gtest/gtest.h>

using namespace testing;

class TerminalClipboardTest : public Test
{
protected:
	SimulatedClipboardStore m_clipboardStore;
	TerminalClipboard m_terminalClipboard{ &m_clipboardStore };
};

TEST_F(TerminalClipboardTest, TrimsSelectionEdgesWhenCopying)
{
	ASSERT_TRUE(m_terminalClipboard.WriteSelection(L" \t\r\n selected \r\n text \t\r\n"));

	Clipboard clipboard(&m_clipboardStore);
	auto text = clipboard.ReadText();
	ASSERT_TRUE(text);
	EXPECT_EQ(*text, L"selected \r\n text");
}

TEST_F(TerminalClipboardTest, CopiesWhitespaceOnlySelectionAsEmptyText)
{
	ASSERT_TRUE(m_terminalClipboard.WriteSelection(L" \t\r\n"));

	Clipboard clipboard(&m_clipboardStore);
	auto text = clipboard.ReadText();
	ASSERT_TRUE(text);
	EXPECT_EQ(*text, L"");
}

TEST_F(TerminalClipboardTest, TrimsTextEdgesWhenPasting)
{
	{
		Clipboard clipboard(&m_clipboardStore);
		ASSERT_TRUE(clipboard.Clear());
		ASSERT_TRUE(clipboard.WriteText(L" \t\r\n paste \r\n text \t\r\n"));
	}

	auto text = m_terminalClipboard.ReadText();
	ASSERT_TRUE(text);
	EXPECT_EQ(*text, L"paste \r\n text");
}
