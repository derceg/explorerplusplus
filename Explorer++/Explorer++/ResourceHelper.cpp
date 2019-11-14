// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include "IconResourceLoader.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ImageHelper.h"

void SetMenuItemImage(HMENU menu, UINT menuItemId, Icon icon, int dpi, std::vector<wil::unique_hbitmap> &menuImages)
{
	DpiCompatibility dpiCompat;
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	wil::unique_hbitmap bitmap = IconResourceLoader::LoadBitmapFromPNGAndScale(icon, iconWidth, iconHeight);

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

std::tuple<wil::unique_himagelist, IconImageListMapping> CreateIconImageList(int iconWidth,
	int iconHeight, const std::initializer_list<Icon> &icons)
{
	wil::unique_himagelist imageList(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(icons.size())));
	IconImageListMapping imageListMappings;

	for (auto icon : icons)
	{
		AddIconToImageList(imageList.get(), icon, iconWidth, iconHeight, imageListMappings);
	}

	return { std::move(imageList), imageListMappings };
}

void AddIconToImageList(HIMAGELIST imageList, Icon icon, int iconWidth, int iconHeight,
	IconImageListMapping &imageListMappings)
{
	wil::unique_hbitmap bitmap = IconResourceLoader::LoadBitmapFromPNGAndScale(icon, iconWidth, iconHeight);
	int imagePosition = ImageList_Add(imageList, bitmap.get(), nullptr);

	if (imagePosition == -1)
	{
		return;
	}

	imageListMappings.insert({ icon, imagePosition });
}