// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FoldersPane.h"
#include "FoldersPaneView.h"

FoldersPane::FoldersPane(FoldersPaneView *view, Config *config, MenuHelpTextHost *menuHelpTextHost,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader) :
	m_toolbar(view->GetToolbarView(), config, menuHelpTextHost, acceleratorManager, resourceLoader)
{
}
