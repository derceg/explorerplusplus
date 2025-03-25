// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>

class BrowserWindow;
class ShellBrowser;
class Tab;

// Indicates the minimum scope level that's desired - either `Browser` scope or `ShellBrowser`
// scope. Some events (e.g. tab events) are only valid at either the `Browser` level or globally.
// The `ShellBrowser` scope is valid for events that can occur within a `ShellBrowser` instance.
enum class EventScopeMinimumLevel
{
	Browser,
	ShellBrowser
};

template <EventScopeMinimumLevel Level>
concept BrowserLevel = (Level == EventScopeMinimumLevel::Browser);

template <EventScopeMinimumLevel Level>
concept ShellBrowserLevel = (Level == EventScopeMinimumLevel::ShellBrowser);

// Some types of events (e.g. navigation events) are broadcast globally. This class allows the
// caller to add an observer for an event, scoped to a particular source location. For example, a
// caller may only be interested in navigation events that occur within a particular browser window.
template <EventScopeMinimumLevel Level>
class EventScope
{
public:
	static EventScope Global();
	static EventScope ForBrowser(const BrowserWindow &browser);
	static EventScope ForShellBrowser(const ShellBrowser &shellBrowser)
		requires ShellBrowserLevel<Level>;

	// This will ensure that the caller is only notified of events that occur within the active
	// ShellBrowser of the specified window. Active here specifically refers to the ShellBrowser
	// that's active when an event is triggered.
	static EventScope ForActiveShellBrowser(const BrowserWindow &browser)
		requires ShellBrowserLevel<Level>;

	bool DoesEventSourceMatch(const Tab &tab) const
		requires BrowserLevel<Level>;
	bool DoesEventSourceMatch(const ShellBrowser &shellBrowser) const
		requires ShellBrowserLevel<Level>;

private:
	enum class Scope
	{
		Global,
		Browser,
		ShellBrowser,
		ActiveShellBrowser
	};

	struct BrowserScoped
	{
	};

	struct ActiveShellBrowserScoped
	{
	};

	EventScope();
	EventScope(const BrowserWindow &browser, BrowserScoped);
	EventScope(const BrowserWindow &browser, ActiveShellBrowserScoped)
		requires ShellBrowserLevel<Level>;
	EventScope(const ShellBrowser &shellBrowser)
		requires ShellBrowserLevel<Level>;

	const Scope m_scope;
	const std::optional<int> m_browserId;
	const std::optional<int> m_shellBrowserId;
};
