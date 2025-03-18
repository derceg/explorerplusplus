// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsTracker.h"
#include "FrequentLocationsModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/NavigationRequest.h"

FrequentLocationsTracker::FrequentLocationsTracker(FrequentLocationsModel *model,
	NavigationEvents *navigationEvents) :
	m_model(model)
{
	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&FrequentLocationsTracker::OnNavigationCommitted, this),
		NavigationEventScope::Global()));
}

void FrequentLocationsTracker::OnNavigationCommitted(const NavigationRequest *request)
{
	m_model->RegisterLocationVisit(request->GetNavigateParams().pidl);
}
