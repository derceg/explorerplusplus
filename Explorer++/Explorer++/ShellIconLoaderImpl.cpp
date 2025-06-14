// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellIconLoaderImpl.h"
#include "IconFetcher.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"

ShellIconLoaderImpl::ShellIconLoaderImpl(IconFetcher *iconFetcher) : m_iconFetcher(iconFetcher)
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));
}

wil::unique_hbitmap ShellIconLoaderImpl::LoadShellIcon(PCIDLIST_ABSOLUTE pidl, ShellIconSize size,
	IconUpdateCallback updateCallback)
{
	if (size != ShellIconSize::Small)
	{
		return nullptr;
	}

	QueueIconUpdateTask(pidl, updateCallback);
	return GetDefaultIcon(pidl);
}

void ShellIconLoaderImpl::QueueIconUpdateTask(PCIDLIST_ABSOLUTE pidl,
	IconUpdateCallback updateCallback)
{
	// The item may not have a cached icon, or the cached icon may be out of date. Therefore, this
	// call will retrieve the updated icon. Even if the caller doesn't use the updated icon, doing
	// this is still useful, since the updated icon will be cached.
	m_iconFetcher->QueueIconTask(pidl,
		[systemImageList = m_systemImageList, updateCallback](int iconIndex, int overlayIndex)
		{
			UNREFERENCED_PARAMETER(overlayIndex);

			wil::unique_hbitmap bitmap;
			HRESULT hr =
				ImageHelper::ImageListIconToPBGRABitmap(systemImageList.get(), iconIndex, bitmap);

			if (FAILED(hr))
			{
				return;
			}

			updateCallback(std::move(bitmap));
		});
}

wil::unique_hbitmap ShellIconLoaderImpl::GetDefaultIcon(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring itemPath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, itemPath);

	if (FAILED(hr))
	{
		return nullptr;
	}

	SFGAOF attributes = SFGAO_FOLDER;
	hr = GetItemAttributes(pidl, &attributes);

	if (FAILED(hr))
	{
		return nullptr;
	}

	DefaultIconType defaultIconType;

	if (WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		defaultIconType = DefaultIconType::Folder;
	}
	else
	{
		defaultIconType = DefaultIconType::File;
	}

	int iconIndex = m_iconFetcher->GetCachedIconIndexOrDefault(itemPath, defaultIconType);

	wil::unique_hbitmap bitmap;
	hr = ImageHelper::ImageListIconToPBGRABitmap(m_systemImageList.get(), iconIndex, bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}
