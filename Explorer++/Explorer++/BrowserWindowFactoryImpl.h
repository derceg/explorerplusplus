// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindowFactory.h"

class AppServices;

class BrowserWindowFactoryImpl : public BrowserWindowFactory
{
public:
	BrowserWindowFactoryImpl(AppServices *appServices, HINSTANCE resourceInstance);

	BrowserWindow *CreateBrowserWindow(const WindowStorageData &storageData) override;

private:
	AppServices *const m_appServices;
	const HINSTANCE m_resourceInstance;
};
