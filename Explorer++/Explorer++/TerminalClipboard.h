// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>
#include <string>
#include <string_view>

class ClipboardStore;

class TerminalClipboard
{
public:
	explicit TerminalClipboard(ClipboardStore *clipboardStore);

	bool WriteSelection(std::wstring_view selection) const;
	std::optional<std::wstring> ReadText() const;

private:
	ClipboardStore *const m_clipboardStore;
};
