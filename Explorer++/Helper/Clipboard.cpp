// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Clipboard.h"
#include "ClipboardStore.h"
#include "DataExchangeHelper.h"

Clipboard::Clipboard(ClipboardStore *store) : m_store(store)
{
	if (m_store->Open())
	{
		m_clipboardOpened = true;
	}
}

Clipboard::~Clipboard()
{
	if (m_clipboardOpened)
	{
		m_store->Close();
	}
}

std::optional<std::wstring> Clipboard::ReadText()
{
	HANDLE clipboardData = m_store->GetData(CF_UNICODETEXT);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadStringFromGlobal(clipboardData);
}

std::optional<std::vector<std::wstring>> Clipboard::ReadHDropData()
{
	HANDLE clipboardData = m_store->GetData(CF_HDROP);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadHDropDataFromGlobal(clipboardData);
}

std::unique_ptr<Gdiplus::Bitmap> Clipboard::ReadPng()
{
	HANDLE clipboardData = m_store->GetData(GetPngClipboardFormat());

	if (!clipboardData)
	{
		return nullptr;
	}

	return ReadPngDataFromGlobal(clipboardData);
}

std::unique_ptr<Gdiplus::Bitmap> Clipboard::ReadDIB()
{
	HANDLE clipboardData = m_store->GetData(CF_DIB);

	if (!clipboardData)
	{
		return nullptr;
	}

	return ReadDIBDataFromGlobal(clipboardData);
}

std::optional<std::string> Clipboard::ReadCustomData(UINT format)
{
	HANDLE clipboardData = m_store->GetData(format);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadBinaryDataFromGlobal(clipboardData);
}

bool Clipboard::WriteText(const std::wstring &text)
{
	auto global = WriteStringToGlobal(text);

	if (!global)
	{
		return false;
	}

	return m_store->SetData(CF_UNICODETEXT, std::move(global));
}

bool Clipboard::WriteHDropData(const std::vector<std::wstring> &paths)
{
	auto global = WriteHDropDataToGlobal(paths);

	if (!global)
	{
		return false;
	}

	return m_store->SetData(CF_HDROP, std::move(global));
}

bool Clipboard::WritePng(Gdiplus::Bitmap *bitmap)
{
	auto global = WritePngDataToGlobal(bitmap);

	if (!global)
	{
		return false;
	}

	return m_store->SetData(GetPngClipboardFormat(), std::move(global));
}

bool Clipboard::WriteDIB(Gdiplus::Bitmap *bitmap)
{
	auto global = WriteDIBDataToGlobal(bitmap);

	if (!global)
	{
		return false;
	}

	return m_store->SetData(CF_DIB, std::move(global));
}

bool Clipboard::WriteCustomData(UINT format, const std::string &data)
{
	auto global = WriteBinaryDataToGlobal(data);

	if (!global)
	{
		return false;
	}

	return m_store->SetData(format, std::move(global));
}

bool Clipboard::Clear()
{
	return m_store->Clear();
}
