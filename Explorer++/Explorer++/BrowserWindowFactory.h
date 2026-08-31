// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BrowserWindow;
struct WindowStorageData;

class BrowserWindowFactory
{
public:
	virtual ~BrowserWindowFactory() = default;

	virtual BrowserWindow *CreateBrowserWindow(const WindowStorageData &storageData) = 0;
};
