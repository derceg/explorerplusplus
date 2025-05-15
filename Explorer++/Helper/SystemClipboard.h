// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Clipboard.h"
#include <wil/resource.h>
#include <gdiplus.h>

class SystemClipboard : public Clipboard
{
public:
	SystemClipboard();
	~SystemClipboard();

	std::optional<std::wstring> ReadText() override;
	std::optional<std::vector<std::wstring>> ReadHDropData();
	std::unique_ptr<Gdiplus::Bitmap> ReadPng();
	std::unique_ptr<Gdiplus::Bitmap> ReadDIB();
	std::optional<std::string> ReadCustomData(UINT format) override;

	bool WriteText(const std::wstring &text) override;
	bool WriteHDropData(const std::vector<std::wstring> &paths);
	bool WritePng(Gdiplus::Bitmap *bitmap);
	bool WriteDIB(Gdiplus::Bitmap *bitmap);
	bool WriteCustomData(UINT format, const std::string &data) override;

	bool Clear() override;

private:
	bool WriteDataToClipboard(UINT format, wil::unique_hglobal global);

	bool m_clipboardOpened;
};
