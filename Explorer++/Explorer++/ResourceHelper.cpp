// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include "IconResourceLoader.h"
#include "../Helper/ImageHelper.h"

const int MENU_IMAGE_SIZE_96DPI = 16;

void SetMenuItemImage(HMENU menu, UINT menuItemId, Icon icon, int dpi, std::vector<wil::unique_hbitmap> &menuImages)
{
	wil::unique_hbitmap bitmap = IconResourceLoader::LoadBitmapFromPNGForDpi(icon, MENU_IMAGE_SIZE_96DPI, dpi);

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

std::tuple<wil::unique_himagelist, IconImageListMapping> CreateIconImageList(int iconSize, int dpi, const std::initializer_list<Icon> &icons)
{
	int scaledIconSize = MulDiv(iconSize, dpi, USER_DEFAULT_SCREEN_DPI);
	wil::unique_himagelist imageList(ImageList_Create(scaledIconSize, scaledIconSize, ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(icons.size())));
	IconImageListMapping imageListMappings;

	for (auto icon : icons)
	{
		AddIconToImageList(imageList.get(), icon, iconSize, dpi, imageListMappings);
	}

	return { std::move(imageList), imageListMappings };
}

void AddIconToImageList(HIMAGELIST imageList, Icon icon, int iconSize, int dpi, IconImageListMapping &imageListMappings)
{
	wil::unique_hbitmap bitmap = IconResourceLoader::LoadBitmapFromPNGForDpi(icon, iconSize, dpi);
	int imagePosition = ImageList_Add(imageList, bitmap.get(), nullptr);

	if (imagePosition == -1)
	{
		return;
	}

	imageListMappings.insert({ icon, imagePosition });
}