// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <functional>
#include <string>
#include <string_view>

class TerminalOutputParser
{
public:
	explicit TerminalOutputParser(
		std::function<void(const std::wstring &)> directoryChangedCallback);

	std::wstring Process(std::wstring_view output);

private:
	std::wstring m_pendingOutput;
	std::function<void(const std::wstring &)> m_directoryChangedCallback;
};
