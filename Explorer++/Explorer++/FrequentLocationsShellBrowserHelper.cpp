// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsShellBrowserHelper.h"
#include "FrequentLocationsModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/NavigationRequest.h"

FrequentLocationsShellBrowserHelper::FrequentLocationsShellBrowserHelper(ShellBrowser *shellBrowser,
	FrequentLocationsModel *model, NavigationEvents *navigationEvents) :
	ShellBrowserHelper(shellBrowser),
	m_model(model)
{
	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&FrequentLocationsShellBrowserHelper::OnNavigationCommitted, this),
		NavigationEventScope::ForShellBrowser(*shellBrowser)));
}

void FrequentLocationsShellBrowserHelper::OnNavigationCommitted(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(shellBrowser);

	m_model->RegisterLocationVisit(request->GetNavigateParams().pidl);
}
