// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "HolderWindow.h"
#include "../Helper/WindowHelper.h"

void Explorerplusplus::ToggleFolders()
{
	m_config->showFolders = !m_config->showFolders.get();
	lShowWindow(m_treeViewHolder->GetHWND(), m_config->showFolders.get());
	UpdateLayout();
}

void Explorerplusplus::ToggleDualPane()
{
	m_config->dualPane = !m_config->dualPane;
}
