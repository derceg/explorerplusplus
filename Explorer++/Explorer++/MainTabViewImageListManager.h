// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>
#include <CommCtrl.h>

class ResourceLoader;

// The main tab view displays two types of icons:
//
// - The lock icon (when a tab is locked)
// - The system icon for the associated folder (when a tab isn't locked)
//
// Since at least one custom icon is used, it's not feasible to assign the system image list itself
// to the view. Instead, a custom image list has to be used that contains the lock icon by default
// and allows system image list icons to be added on demand.
//
// This class manages that custom image list.
class MainTabViewImageListManager
{
public:
	MainTabViewImageListManager(const ResourceLoader *resourceLoader, UINT dpi);

	HIMAGELIST GetImageList() const;
	int AddIconFromSystemImageList(int systemIconIndex);
	bool IsDefaultIcon(int iconIndex) const;
	int GetLockIconIndex() const;
	int GetDefaultFolderIconIndex() const;

private:
	static constexpr int ICON_SIZE_96DPI = 16;

	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconSystemImageListIndex;

	wil::unique_himagelist m_imageList;
	int m_defaultFolderIconIndex;
	int m_lockIconIndex;
};
