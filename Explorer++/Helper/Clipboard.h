// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>
#include <string>

class Clipboard
{
public:
	virtual ~Clipboard() = default;

	virtual std::optional<std::wstring> ReadText() = 0;
	virtual std::optional<std::string> ReadCustomData(UINT format) = 0;

	virtual bool WriteText(const std::wstring &text) = 0;
	virtual bool WriteCustomData(UINT format, const std::string &data) = 0;

	virtual bool Clear() = 0;
};
