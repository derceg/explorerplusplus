// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindowFactory.h"
#include <functional>

class BrowserWindowFactoryFake : public BrowserWindowFactory
{
public:
	using CreationCallback = std::function<BrowserWindow *(const WindowStorageData &storageData)>;

	BrowserWindowFactoryFake(CreationCallback creationCallback);

	// BrowserWindowFactory
	BrowserWindow *CreateBrowserWindow(const WindowStorageData &storageData) override;

private:
	CreationCallback m_creationCallback;
};
