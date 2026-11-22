// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wincodec.h>
#include <optional>
#include <string>
#include <vector>

class ClipboardStore;

class Clipboard
{
public:
	Clipboard(ClipboardStore *store);
	~Clipboard();

	std::optional<std::wstring> ReadText();
	std::optional<std::vector<std::wstring>> ReadHDropData();
	wil::com_ptr_nothrow<IWICBitmapSource> ReadPng();
	wil::com_ptr_nothrow<IWICBitmapSource> ReadGdiBitmap();
	std::optional<std::string> ReadCustomData(UINT format);

	bool WriteText(const std::wstring &text);
	bool WriteHDropData(const std::vector<std::wstring> &paths);
	bool WritePng(IWICBitmapSource *bitmap);
	bool WriteGdiBitmap(IWICBitmapSource *bitmap);
	bool WriteCustomData(UINT format, const std::string &data);

	bool Clear();

private:
	ClipboardStore *const m_store;
	bool m_clipboardOpened = false;
};
