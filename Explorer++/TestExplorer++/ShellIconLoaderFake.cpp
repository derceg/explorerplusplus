// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellIconLoaderFake.h"
#include "ImageTestHelper.h"

wil::unique_hbitmap ShellIconLoaderFake::LoadShellIcon(PCIDLIST_ABSOLUTE pidl, ShellIconSize size,
	ShellIconUpdateCallback updateCallback)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(size);

	if (m_storeUpdateCallbacks)
	{
		m_updateCallbacks.push_back(updateCallback);
	}

	return GenerateImage();
}

void ShellIconLoaderFake::SetBitmapGeneratedCallback(BitmapGeneratedCallback callback)
{
	m_bitmapGeneratedCallback = callback;
}

void ShellIconLoaderFake::ClearBitmapGeneratedCallback()
{
	m_bitmapGeneratedCallback = {};
}

void ShellIconLoaderFake::TriggerPendingUpdateCallbacks()
{
	for (const auto &callback : m_updateCallbacks)
	{
		callback(GenerateImage());
	}

	m_updateCallbacks.clear();
}

void ShellIconLoaderFake::SetStoreUpdateCallbacks(bool storeUpdateCallbacks)
{
	m_storeUpdateCallbacks = storeUpdateCallbacks;
}

wil::unique_hbitmap ShellIconLoaderFake::GenerateImage() const
{
	wil::unique_hbitmap bitmap;
	BuildTestBitmap(10, 10, bitmap);

	if (m_bitmapGeneratedCallback)
	{
		m_bitmapGeneratedCallback(bitmap.get());
	}

	return bitmap;
}
