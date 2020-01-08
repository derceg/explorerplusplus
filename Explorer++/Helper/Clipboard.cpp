// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Clipboard.h"
#include "DataExchangeHelper.h"

Clipboard::Clipboard() :
	m_clipboardOpened(false)
{
	if (OpenClipboard(nullptr))
	{
		m_clipboardOpened = true;
	}
}

Clipboard::~Clipboard()
{
	if (m_clipboardOpened)
	{
		CloseClipboard();
	}
}

std::optional<std::wstring> Clipboard::ReadText()
{
	return ReadData<std::wstring>(CF_UNICODETEXT);
}

std::optional<std::string> Clipboard::ReadCustomData(UINT format)
{
	return ReadData<std::string>(format);
}

template <class T>
std::optional<T> Clipboard::ReadData(UINT format)
{
	static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>);

	HANDLE clipboardData = GetClipboardData(format);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadStringFromGlobal<T>(clipboardData);
}

bool Clipboard::WriteText(const std::wstring &str)
{
	return WriteData(CF_UNICODETEXT, str);
}

bool Clipboard::WriteCustomData(UINT format, const std::string &data)
{
	return WriteData(format, data);
}

template <class T>
bool Clipboard::WriteData(UINT format, const T &data)
{
	static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>);

	auto global = WriteStringToGlobal(data);

	if (!global)
	{
		return false;
	}

	return WriteDataToClipboard(format, std::move(global));
}

bool Clipboard::WriteDataToClipboard(UINT format, wil::unique_hglobal global)
{
	HANDLE clipboardData = SetClipboardData(format, global.get());

	if (!clipboardData)
	{
		return false;
	}

	// SetClipboardData() takes ownership of the data passed to it. Therefore,
	// it's important that the ownership of the data is relinquished here if the
	// call to SetClipboardData() succeeded.
	global.release();

	return true;
}

bool Clipboard::Clear()
{
	return EmptyClipboard();
}