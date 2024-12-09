// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsShellBrowserHelper.h"
#include "FrequentLocationsModel.h"

FrequentLocationsShellBrowserHelper::FrequentLocationsShellBrowserHelper(ShellBrowser *shellBrowser,
	FrequentLocationsModel *model) :
	ShellBrowserHelper(shellBrowser),
	m_model(model)
{
	shellBrowser->AddNavigationCommittedObserver(
		std::bind_front(&FrequentLocationsShellBrowserHelper::OnNavigationCommitted, this));
}

void FrequentLocationsShellBrowserHelper::OnNavigationCommitted(
	const NavigateParams &navigateParams)
{
	m_model->RegisterLocationVisit(navigateParams.pidl);
}
