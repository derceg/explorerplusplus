// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBarViewImageListManager.h"
#include "TabView.h"

class TabBarView : public TabView
{
public:
	TabBarView(HWND parent, const DarkModeManager *darkModeManager,
		const DarkModeColorProvider *darkModeColorProvider,
		const LightModeColorProvider *lightModeColorProvider, const Config *config,
		const ResourceLoader *resourceLoader);

	TabBarViewImageListManager *GetImageListManager();

private:
	TabBarViewImageListManager m_imageListManager;
};
