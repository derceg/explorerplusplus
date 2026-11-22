// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Clipboard.h"
#include "ClipboardStore.h"
#include "DataExchangeHelper.h"
#include "GraphicsFactories.h"
#include "ImageInterop.h"

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
	auto clipboardData = m_store->GetGlobalData(CF_UNICODETEXT);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadStringFromGlobal(clipboardData.get());
}

std::optional<std::vector<std::wstring>> Clipboard::ReadHDropData()
{
	auto clipboardData = m_store->GetGlobalData(CF_HDROP);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadHDropDataFromGlobal(clipboardData.get());
}

wil::com_ptr_nothrow<IWICBitmapSource> Clipboard::ReadPng()
{
	auto clipboardData = m_store->GetGlobalData(GetPngClipboardFormat());

	if (!clipboardData)
	{
		return nullptr;
	}

	return ReadPngDataFromGlobal(clipboardData.get());
}

wil::com_ptr_nothrow<IWICBitmapSource> Clipboard::ReadGdiBitmap()
{
	auto gdiBitmap = m_store->GetGdiBitmap();

	if (!gdiBitmap)
	{
		return nullptr;
	}

	auto *imagingFactory = GraphicsFactories::GetWICFactory();

	wil::com_ptr_nothrow<IWICBitmap> bitmap;
	HRESULT hr = imagingFactory->CreateBitmapFromHBITMAP(gdiBitmap.get(), nullptr,
		WICBitmapIgnoreAlpha, &bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}

std::optional<std::string> Clipboard::ReadCustomData(UINT format)
{
	auto clipboardData = m_store->GetGlobalData(format);

	if (!clipboardData)
	{
		return std::nullopt;
	}

	return ReadBinaryDataFromGlobal(clipboardData.get());
}

bool Clipboard::WriteText(const std::wstring &text)
{
	auto global = WriteStringToGlobal(text);

	if (!global)
	{
		return false;
	}

	return m_store->SetGlobalData(CF_UNICODETEXT, std::move(global));
}

bool Clipboard::WriteHDropData(const std::vector<std::wstring> &paths)
{
	auto global = WriteHDropDataToGlobal(paths);

	if (!global)
	{
		return false;
	}

	return m_store->SetGlobalData(CF_HDROP, std::move(global));
}

bool Clipboard::WritePng(IWICBitmapSource *bitmap)
{
	auto global = WritePngDataToGlobal(bitmap);

	if (!global)
	{
		return false;
	}

	return m_store->SetGlobalData(GetPngClipboardFormat(), std::move(global));
}

bool Clipboard::WriteGdiBitmap(IWICBitmapSource *bitmap)
{
	wil::unique_hbitmap gdiBitmap;
	HRESULT hr = ImageInterop::CreateHBITMAPFromWICBitmap(bitmap, ImageInterop::AlphaMode::Straight,
		gdiBitmap);

	if (FAILED(hr))
	{
		return false;
	}

	return m_store->SetGdiBitmap(std::move(gdiBitmap));
}

bool Clipboard::WriteCustomData(UINT format, const std::string &data)
{
	auto global = WriteBinaryDataToGlobal(data);

	if (!global)
	{
		return false;
	}

	return m_store->SetGlobalData(format, std::move(global));
}

bool Clipboard::Clear()
{
	return m_store->Clear();
}
