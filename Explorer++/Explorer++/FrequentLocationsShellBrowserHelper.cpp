// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsShellBrowserHelper.h"
#include "FrequentLocationsModel.h"
#include "ShellBrowser/NavigationRequest.h"

FrequentLocationsShellBrowserHelper::FrequentLocationsShellBrowserHelper(ShellBrowser *shellBrowser,
	FrequentLocationsModel *model) :
	ShellBrowserHelper(shellBrowser),
	m_model(model)
{
	shellBrowser->AddNavigationCommittedObserver(
		std::bind_front(&FrequentLocationsShellBrowserHelper::OnNavigationCommitted, this));
}

void FrequentLocationsShellBrowserHelper::OnNavigationCommitted(const NavigationRequest *request)
{
	m_model->RegisterLocationVisit(request->GetNavigateParams().pidl);
}
