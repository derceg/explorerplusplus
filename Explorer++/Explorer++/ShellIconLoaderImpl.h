// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellIconLoader.h"
#include <wil/com.h>

class IconFetcher;

class ShellIconLoaderImpl : public ShellIconLoader
{
public:
	ShellIconLoaderImpl(IconFetcher *iconFetcher);

	wil::unique_hbitmap LoadShellIcon(PCIDLIST_ABSOLUTE pidl, ShellIconSize size,
		ShellIconUpdateCallback updateCallback) override;

private:
	void QueueIconUpdateTask(PCIDLIST_ABSOLUTE pidl, ShellIconUpdateCallback updateCallback);
	wil::unique_hbitmap GetDefaultIcon(PCIDLIST_ABSOLUTE pidl);

	IconFetcher *const m_iconFetcher;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
};
