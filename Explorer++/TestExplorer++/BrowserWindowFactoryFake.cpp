// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserWindowFactoryFake.h"

BrowserWindowFactoryFake::BrowserWindowFactoryFake(CreationCallback creationCallback) :
	m_creationCallback(creationCallback)
{
}

BrowserWindow *BrowserWindowFactoryFake::CreateBrowserWindow(const WindowStorageData &storageData)
{
	return m_creationCallback(storageData);
}
