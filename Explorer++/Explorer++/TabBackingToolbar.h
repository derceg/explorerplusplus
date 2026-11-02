// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarBase.h"
#include <wil/resource.h>

class AcceleratorManager;
class BrowserWindow;
class ResourceLoader;
class TabBackingToolbarView;
class TabContainer;
class TabEvents;

class TabBackingToolbar : public ToolbarBase
{
public:
	TabBackingToolbar(TabBackingToolbarView *view, BrowserWindow *browser,
		TabContainer *tabContainer, TabEvents *tabEvents,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader);

private:
	wil::unique_himagelist m_imageList;
};
