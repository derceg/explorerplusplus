// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalClipboard.h"
#include "../Helper/Clipboard.h"

namespace
{

constexpr std::wstring_view TERMINAL_EDGE_WHITESPACE = L" \t\r\n\f\v";

std::wstring TrimText(std::wstring_view text)
{
	auto first = text.find_first_not_of(TERMINAL_EDGE_WHITESPACE);

	if (first == std::wstring_view::npos)
	{
		return {};
	}

	auto last = text.find_last_not_of(TERMINAL_EDGE_WHITESPACE);
	return std::wstring(text.substr(first, last - first + 1));
}

}

TerminalClipboard::TerminalClipboard(ClipboardStore *clipboardStore) :
	m_clipboardStore(clipboardStore)
{
}

bool TerminalClipboard::WriteSelection(std::wstring_view selection) const
{
	Clipboard clipboard(m_clipboardStore);
	return clipboard.Clear() && clipboard.WriteText(TrimText(selection));
}

std::optional<std::wstring> TerminalClipboard::ReadText() const
{
	Clipboard clipboard(m_clipboardStore);
	auto text = clipboard.ReadText();

	if (!text)
	{
		return std::nullopt;
	}

	return TrimText(*text);
}
