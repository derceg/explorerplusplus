// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "EventScope.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"

template <EventScopeMinimumLevel Level>
EventScope<Level> EventScope<Level>::Global()
{
	return EventScope();
}

template <EventScopeMinimumLevel Level>
EventScope<Level> EventScope<Level>::ForBrowser(const BrowserWindow &browser)
{
	return EventScope(browser, BrowserScoped{});
}

template <EventScopeMinimumLevel Level>
EventScope<Level> EventScope<Level>::ForShellBrowser(const ShellBrowser &shellBrowser)
	requires ShellBrowserLevel<Level>
{
	return EventScope(shellBrowser);
}

template <EventScopeMinimumLevel Level>
EventScope<Level> EventScope<Level>::ForActiveShellBrowser(const BrowserWindow &browser)
	requires ShellBrowserLevel<Level>
{
	return EventScope(browser, ActiveShellBrowserScoped{});
}

template <EventScopeMinimumLevel Level>
EventScope<Level>::EventScope() : m_scope(Scope::Global)
{
}

template <EventScopeMinimumLevel Level>
EventScope<Level>::EventScope(const BrowserWindow &browser, BrowserScoped) :
	m_scope(Scope::Browser),
	m_browserId(browser.GetId())
{
}

template <EventScopeMinimumLevel Level>
EventScope<Level>::EventScope(const BrowserWindow &browser, ActiveShellBrowserScoped)
	requires ShellBrowserLevel<Level>
	: m_scope(Scope::ActiveShellBrowser), m_browserId(browser.GetId())
{
}

template <EventScopeMinimumLevel Level>
EventScope<Level>::EventScope(const ShellBrowser &shellBrowser)
	requires ShellBrowserLevel<Level>
	: m_scope(Scope::ShellBrowser), m_shellBrowserId(shellBrowser.GetId())
{
}

template <EventScopeMinimumLevel Level>
bool EventScope<Level>::DoesEventSourceMatch(const Tab &tab) const
	requires BrowserLevel<Level>
{
	switch (m_scope)
	{
	case Scope::Browser:
		return tab.GetBrowser()->GetId() == m_browserId;

	case Scope::Global:
	default:
		return true;
	}
}

template <EventScopeMinimumLevel Level>
bool EventScope<Level>::DoesEventSourceMatch(const ShellBrowser &shellBrowser) const
	requires ShellBrowserLevel<Level>
{
	switch (m_scope)
	{
	case Scope::ActiveShellBrowser:
		return shellBrowser.GetTab()->GetBrowser()->GetId() == m_browserId
			&& shellBrowser.GetTab()->GetBrowser()->IsShellBrowserActive(&shellBrowser);

	case Scope::ShellBrowser:
		return shellBrowser.GetId() == m_shellBrowserId;

	case Scope::Browser:
		return shellBrowser.GetTab()->GetBrowser()->GetId() == m_browserId;

	case Scope::Global:
	default:
		return true;
	}
}

template class EventScope<EventScopeMinimumLevel::Browser>;
template class EventScope<EventScopeMinimumLevel::ShellBrowser>;
