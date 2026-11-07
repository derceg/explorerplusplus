// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarBase.h"
#include <wil/resource.h>

struct Config;
class FoldersPaneToolbarView;
class ResourceLoader;

class FoldersPaneToolbar : public ToolbarBase
{
public:
	FoldersPaneToolbar(FoldersPaneToolbarView *view, Config *config,
		MenuHelpTextHost *menuHelpTextHost, const AcceleratorManager *acceleratorManager,
		const ResourceLoader *resourceLoader);

private:
	wil::unique_himagelist m_imageList;
};
