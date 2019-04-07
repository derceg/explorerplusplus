// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include "MainResource.h"
#include "../Helper/ImageHelper.h"

HImageListPtr GetShellImageList()
{
	HImageListPtr imageList = HImageListPtr(ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 48));

	if (!imageList)
	{
		return HImageListPtr();
	}

	HBitmapPtr bitmap = HBitmapPtr(static_cast<HBITMAP>(LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_SHELLIMAGES), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION)));

	if (!bitmap)
	{
		return HImageListPtr();
	}

	const int res = ImageList_Add(imageList.get(), bitmap.get(), nullptr);

	if (res == -1)
	{
		return HImageListPtr();
	}

	return imageList;
}

void SetMenuItemImageFromImageList(HMENU menu, UINT menuItemId, HIMAGELIST imageList, int bitmapIndex, std::vector<HBitmapPtr> &menuImages)
{
	HIconPtr icon = HIconPtr(ImageList_GetIcon(imageList, bitmapIndex, ILD_NORMAL));

	if (!icon)
	{
		return;
	}

	HBitmapPtr bitmapPARGB32 = HBitmapPtr(ImageHelper::IconToBitmapPARGB32(icon.get(), 16, 16));

	if (!bitmapPARGB32)
	{
		return;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = bitmapPARGB32.get();
	const BOOL res = SetMenuItemInfo(menu, menuItemId, FALSE, &mii);

	if (res)
	{
		/* The bitmap needs to live
		for as long as the menu
		does. It's up to the caller
		to ensure that the bitmap
		is destroyed at the appropriate
		time. */
		menuImages.push_back(std::move(bitmapPARGB32));
	}
}