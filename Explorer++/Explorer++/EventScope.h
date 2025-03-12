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

	bool DoesEventSourceMatch(const Tab &tab) const;
	bool DoesEventSourceMatch(const ShellBrowser &shellBrowser) const
		requires ShellBrowserLevel<Level>;

private:
	enum class Scope
	{
		Global,
		Browser,
		ShellBrowser
	};

	EventScope();
	EventScope(const BrowserWindow &browser);
	EventScope(const ShellBrowser &shellBrowser);

	const Scope m_scope;
	const std::optional<int> m_browserId;
	const std::optional<int> m_shellBrowserId;
};
