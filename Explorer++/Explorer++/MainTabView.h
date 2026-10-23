// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainTabViewImageListManager.h"
#include "TabView.h"

class ResourceLoader;

class MainTabView : public TabView
{
public:
	MainTabView(HWND parent, const Config *config, const ResourceLoader *resourceLoader);

	MainTabViewImageListManager *GetImageListManager();

private:
	MainTabViewImageListManager m_imageListManager;
};
