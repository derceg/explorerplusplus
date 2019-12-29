// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <optional>

class Clipboard
{
public:

	Clipboard();
	~Clipboard();

	std::optional<std::wstring> ReadText();
	std::optional<std::string> ReadCustomData(UINT format);

	bool WriteText(const std::wstring &str);
	bool WriteCustomData(UINT format, const std::string &data);

	bool Clear();

private:

	template <class T>
	wil::unique_hglobal CreateGlobalFromString(const T &str);

	template <class T>
	std::optional<T> ReadData(UINT format);
	bool WriteData(UINT format, wil::unique_hglobal global);

	bool m_clipboardOpened;
};