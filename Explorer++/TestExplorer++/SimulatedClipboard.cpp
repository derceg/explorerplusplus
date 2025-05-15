// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboard.h"

std::optional<std::wstring> SimulatedClipboard::ReadText()
{
	return ReadData<wchar_t>(CF_UNICODETEXT);
}

std::optional<std::string> SimulatedClipboard::ReadCustomData(UINT format)
{
	return ReadData<char>(format);
}

bool SimulatedClipboard::WriteText(const std::wstring &text)
{
	WriteData(CF_UNICODETEXT, text);
	return true;
}

bool SimulatedClipboard::WriteCustomData(UINT format, const std::string &data)
{
	WriteData(format, data);
	return true;
}

bool SimulatedClipboard::Clear()
{
	m_data.clear();
	return true;
}
