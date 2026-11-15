// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellIconLoaderFake.h"
#include "ImageTestHelper.h"

wil::com_ptr_nothrow<IWICBitmapSource> ShellIconLoaderFake::LoadShellIcon(PCIDLIST_ABSOLUTE pidl,
	IconSize size, IconUpdateCallback updateCallback, std::stop_token stopToken)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(size);

	if (m_storeUpdateCallbacks)
	{
		m_pendingUpdates.push_back({ std::move(updateCallback), stopToken });
	}

	m_numIconsRequested++;

	return GenerateImage();
}

int ShellIconLoaderFake::GetNumIconsRequested() const
{
	return m_numIconsRequested;
}

void ShellIconLoaderFake::TriggerPendingUpdateCallbacks()
{
	for (const auto &pendingUpdate : m_pendingUpdates)
	{
		if (pendingUpdate.stopToken.stop_requested())
		{
			continue;
		}

		auto bitmap = GenerateImage();
		pendingUpdate.callback(bitmap.get());
	}

	m_pendingUpdates.clear();
}

void ShellIconLoaderFake::SetStoreUpdateCallbacks(bool storeUpdateCallbacks)
{
	m_storeUpdateCallbacks = storeUpdateCallbacks;
}

wil::com_ptr_nothrow<IWICBitmapSource> ShellIconLoaderFake::GenerateImage() const
{
	return BuildTestWICBitmap(10, 10);
}
