// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationEvents.h"
#include "BrowserWindow.h"
#include "ShellBrowser.h"
#include "Tab.h"

NavigationEventScope NavigationEventScope::Global()
{
	return NavigationEventScope();
}

NavigationEventScope NavigationEventScope::ForBrowser(const BrowserWindow &browser)
{
	return NavigationEventScope(browser);
}

NavigationEventScope NavigationEventScope::ForShellBrowser(const ShellBrowser &shellBrowser)
{
	return NavigationEventScope(shellBrowser);
}

NavigationEventScope::NavigationEventScope() : m_scope(Scope::Global)
{
}

NavigationEventScope::NavigationEventScope(const BrowserWindow &browser) :
	m_scope(Scope::Browser),
	m_browserId(browser.GetId())
{
}

NavigationEventScope::NavigationEventScope(const ShellBrowser &shellBrowser) :
	m_scope(Scope::ShellBrowser),
	m_shellBrowserId(shellBrowser.GetId())
{
}

NavigationEventScope::Scope NavigationEventScope::GetScope() const
{
	return m_scope;
}

std::optional<int> NavigationEventScope::GetBrowserId() const
{
	return m_browserId;
}

std::optional<int> NavigationEventScope::GetShellBrowserId() const
{
	return m_shellBrowserId;
}

boost::signals2::connection NavigationEvents::AddStartedObserver(
	const StartedSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_startedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddFailedObserver(
	const FailedSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_failedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddStoppedObserver(
	const StoppedSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_stoppedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

bool NavigationEvents::DoesEventMatchScope(const NavigationEventScope &scope,
	const ShellBrowser *shellBrowser)
{
	switch (scope.GetScope())
	{
	case NavigationEventScope::Scope::ShellBrowser:
		return shellBrowser->GetId() == scope.GetShellBrowserId();

	case NavigationEventScope::Scope::Browser:
		return shellBrowser->GetTab()->GetBrowser()->GetId() == scope.GetBrowserId();

	case NavigationEventScope::Scope::Global:
	default:
		return true;
	}
}

void NavigationEvents::NotifyStarted(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_startedSignal(shellBrowser, request);
}

void NavigationEvents::NotifyFailed(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_failedSignal(shellBrowser, request);
}

void NavigationEvents::NotifyStopped(const ShellBrowser *shellBrowser)
{
	m_stoppedSignal(shellBrowser);
}
