// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainTabViewImageListManager.h"
#include "Icon.h"
#include "ResourceLoader.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"

MainTabViewImageListManager::MainTabViewImageListManager(const ResourceLoader *resourceLoader,
	UINT dpi)
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));
	FAIL_FAST_IF_FAILED(::GetDefaultFolderIconIndex(m_defaultFolderIconSystemImageListIndex));

	int dpiScaledSize = MulDiv(ICON_SIZE_96DPI, dpi, USER_DEFAULT_SCREEN_DPI);
	m_imageList.reset(
		ImageList_Create(dpiScaledSize, dpiScaledSize, ILC_COLOR32 | ILC_MASK, 0, 10));
	CHECK(m_imageList);

	wil::unique_hbitmap bitmap =
		resourceLoader->LoadBitmapFromPNGForDpi(Icon::Lock, ICON_SIZE_96DPI, ICON_SIZE_96DPI, dpi);
	m_lockIconIndex = ImageList_Add(m_imageList.get(), bitmap.get(), nullptr);
	CHECK_NE(m_lockIconIndex, -1);

	m_defaultFolderIconIndex = ImageHelper::CopyImageListIcon(m_imageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()),
		m_defaultFolderIconSystemImageListIndex);
}

HIMAGELIST MainTabViewImageListManager::GetImageList() const
{
	return m_imageList.get();
}

int MainTabViewImageListManager::AddIconFromSystemImageList(int systemIconIndex)
{
	if (systemIconIndex == m_defaultFolderIconSystemImageListIndex)
	{
		return m_defaultFolderIconIndex;
	}

	return ImageHelper::CopyImageListIcon(m_imageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()), systemIconIndex);
}

bool MainTabViewImageListManager::IsDefaultIcon(int iconIndex) const
{
	return iconIndex == m_lockIconIndex || iconIndex == m_defaultFolderIconIndex;
}

int MainTabViewImageListManager::GetLockIconIndex() const
{
	return m_lockIconIndex;
}

int MainTabViewImageListManager::GetDefaultFolderIconIndex() const
{
	return m_defaultFolderIconIndex;
}
