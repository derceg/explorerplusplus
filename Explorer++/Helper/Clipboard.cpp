// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Clipboard.h"
#include <type_traits>

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

	wil::unique_hglobal_locked mem(clipboardData);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(mem.get());

	if (size == 0)
	{
		return std::nullopt;
	}

	using charType = T::traits_type::char_type;

	return T(static_cast<const charType *>(mem.get()), size / sizeof(charType));
}

bool Clipboard::WriteText(const std::wstring &str)
{
	auto global = CreateGlobalFromString(str);

	if (!global)
	{
		return false;
	}

	return WriteData(CF_UNICODETEXT, std::move(global));
}

bool Clipboard::WriteCustomData(UINT format, const std::string &data)
{
	auto global = CreateGlobalFromString(data);

	if (!global)
	{
		return false;
	}

	return WriteData(format, std::move(global));
}

template <class T>
wil::unique_hglobal Clipboard::CreateGlobalFromString(const T &str)
{
	static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>);

	size_t stringSize = (str.size() + 1) * sizeof(T::traits_type::char_type);
	wil::unique_hglobal global(GlobalAlloc(GMEM_MOVEABLE, stringSize));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	memcpy(mem.get(), str.c_str(), stringSize);

	return global;
}

bool Clipboard::WriteData(UINT format, wil::unique_hglobal global)
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