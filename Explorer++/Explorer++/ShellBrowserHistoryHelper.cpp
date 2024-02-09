// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserHistoryHelper.h"
#include "HistoryService.h"
#include "HistoryServiceFactory.h"

ShellBrowserHistoryHelper::ShellBrowserHistoryHelper(ShellBrowserInterface *shellBrowser) :
	ShellBrowserHelper(shellBrowser)
{
	// There's no need to explicitly remove this observer, since this object is tied to the
	// lifetime of the shellBrowser instance.
	shellBrowser->AddNavigationCommittedObserver(
		std::bind_front(&ShellBrowserHistoryHelper::OnNavigationCommitted, this));
}

void ShellBrowserHistoryHelper::OnNavigationCommitted(const NavigateParams &navigateParams)
{
	auto *historyService = HistoryServiceFactory::GetInstance()->GetHistoryService();
	historyService->AddHistoryItem(navigateParams.pidl);
}
