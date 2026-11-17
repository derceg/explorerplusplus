// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ImageHelper.h"
#include <glog/logging.h>

std::wstring ResourceHelper::LoadString(HINSTANCE resourceInstance, UINT stringId)
{
	WCHAR *string;
	int numCharacters =
		LoadString(resourceInstance, stringId, reinterpret_cast<LPWSTR>(&string), 0);
	CHECK_NE(numCharacters, 0);

	return std::wstring(string, numCharacters);
}

std::tuple<wil::unique_himagelist, IconImageListMapping> ResourceHelper::CreateIconImageList(
	const ResourceLoader *resourceLoader, int iconWidth, int iconHeight,
	const std::initializer_list<Icon> &icons)
{
	wil::unique_himagelist imageList(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK,
		0, static_cast<int>(icons.size())));
	IconImageListMapping imageListMappings;

	for (auto icon : icons)
	{
		AddIconToImageList(imageList.get(), resourceLoader, icon, iconWidth, iconHeight,
			imageListMappings);
	}

	return { std::move(imageList), imageListMappings };
}

void ResourceHelper::AddIconToImageList(HIMAGELIST imageList, const ResourceLoader *resourceLoader,
	Icon icon, int iconWidth, int iconHeight, IconImageListMapping &imageListMappings)
{
	auto bitmap = resourceLoader->LoadBitmap(icon, iconWidth, iconHeight);
	int imagePosition = ImageHelper::AddBitmapToImageList(imageList, bitmap.get());

	imageListMappings.insert({ icon, imagePosition });
}
