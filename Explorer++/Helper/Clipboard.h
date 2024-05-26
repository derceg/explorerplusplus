// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <gdiplus.h>
#include <optional>
#include <string>

class Clipboard
{
public:
	Clipboard();
	~Clipboard();

	std::optional<std::wstring> ReadText();
	std::optional<std::vector<std::wstring>> ReadHDropData();
	std::unique_ptr<Gdiplus::Bitmap> ReadPng();
	std::unique_ptr<Gdiplus::Bitmap> ReadDIB();
	std::optional<std::string> ReadCustomData(UINT format);

	bool WriteText(const std::wstring &str);
	bool WriteHDropData(const std::vector<std::wstring> &paths);
	bool WritePng(Gdiplus::Bitmap *bitmap);
	bool WriteDIB(Gdiplus::Bitmap *bitmap);
	bool WriteCustomData(UINT format, const std::string &data);

	bool Clear();

private:
	bool WriteDataToClipboard(UINT format, wil::unique_hglobal global);

	bool m_clipboardOpened;
};
