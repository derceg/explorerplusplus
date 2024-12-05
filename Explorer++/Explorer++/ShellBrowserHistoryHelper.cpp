// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserHistoryHelper.h"
#include "HistoryModel.h"

ShellBrowserHistoryHelper::ShellBrowserHistoryHelper(ShellBrowser *shellBrowser,
	HistoryModel *historyModel) :
	ShellBrowserHelper(shellBrowser),
	m_historyModel(historyModel)
{
	// There's no need to explicitly remove this observer, since this object is tied to the
	// lifetime of the shellBrowser instance.
	shellBrowser->AddNavigationCommittedObserver(
		std::bind_front(&ShellBrowserHistoryHelper::OnNavigationCommitted, this));
}

void ShellBrowserHistoryHelper::OnNavigationCommitted(const NavigateParams &navigateParams)
{
	m_historyModel->AddHistoryItem(navigateParams.pidl);
}
