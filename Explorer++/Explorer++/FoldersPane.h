// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FoldersPaneToolbar.h"

class AcceleratorManager;
struct Config;
class FoldersPaneView;
class MenuHelpTextHost;
class ResourceLoader;

class FoldersPane
{
public:
	FoldersPane(FoldersPaneView *view, Config *config, MenuHelpTextHost *menuHelpTextHost,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader);

private:
	FoldersPaneToolbar m_toolbar;
};
