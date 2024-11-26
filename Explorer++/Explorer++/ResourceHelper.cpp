// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include <glog/logging.h>

std::wstring ResourceHelper::LoadString(HINSTANCE resourceInstance, UINT stringId)
{
	auto string = MaybeLoadString(resourceInstance, stringId);
	CHECK(string) << "String resource not found";
	return *string;
}

std::optional<std::wstring> ResourceHelper::MaybeLoadString(HINSTANCE resourceInstance,
	UINT stringId)
{
	WCHAR *string;
	int numCharacters =
		LoadString(resourceInstance, stringId, reinterpret_cast<LPWSTR>(&string), 0);

	if (numCharacters == 0)
	{
		return std::nullopt;
	}

	return std::wstring(string, numCharacters);
}

void ResourceHelper::SetMenuItemImage(HMENU menu, UINT menuItemId,
	const IconResourceLoader *iconResourceLoader, Icon icon, int dpi,
	std::vector<wil::unique_hbitmap> &menuImages)
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	wil::unique_hbitmap bitmap =
		iconResourceLoader->LoadBitmapFromPNGAndScale(icon, iconWidth, iconHeight);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = bitmap.get();
	const BOOL res = SetMenuItemInfo(menu, menuItemId, false, &mii);

	if (res)
	{
		/* The bitmap needs to live for as long as the menu does. */
		menuImages.push_back(std::move(bitmap));
	}
}

std::tuple<wil::unique_himagelist, IconImageListMapping> ResourceHelper::CreateIconImageList(
	const IconResourceLoader *iconResourceLoader, int iconWidth, int iconHeight,
	const std::initializer_list<Icon> &icons)
{
	wil::unique_himagelist imageList(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK,
		0, static_cast<int>(icons.size())));
	IconImageListMapping imageListMappings;

	for (auto icon : icons)
	{
		AddIconToImageList(imageList.get(), iconResourceLoader, icon, iconWidth, iconHeight,
			imageListMappings);
	}

	return { std::move(imageList), imageListMappings };
}

void ResourceHelper::AddIconToImageList(HIMAGELIST imageList,
	const IconResourceLoader *iconResourceLoader, Icon icon, int iconWidth, int iconHeight,
	IconImageListMapping &imageListMappings)
{
	wil::unique_hbitmap bitmap =
		iconResourceLoader->LoadBitmapFromPNGAndScale(icon, iconWidth, iconHeight);
	int imagePosition = ImageList_Add(imageList, bitmap.get(), nullptr);

	if (imagePosition == -1)
	{
		return;
	}

	imageListMappings.insert({ icon, imagePosition });
}
