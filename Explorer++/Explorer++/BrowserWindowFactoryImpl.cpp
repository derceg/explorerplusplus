// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserWindowFactoryImpl.h"
#include "Explorer++.h"

BrowserWindowFactoryImpl::BrowserWindowFactoryImpl(AppServices *appServices,
	HINSTANCE resourceInstance) :
	m_appServices(appServices),
	m_resourceInstance(resourceInstance)
{
}

BrowserWindow *BrowserWindowFactoryImpl::CreateBrowserWindow(const WindowStorageData &storageData)
{
	return Explorerplusplus::Create(storageData, m_appServices, m_resourceInstance);
}
