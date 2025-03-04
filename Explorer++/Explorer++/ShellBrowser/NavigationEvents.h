// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <optional>
#include <utility>

class BrowserWindow;
class NavigationRequest;
class ShellBrowser;

class NavigationEventScope
{
public:
	enum class Scope
	{
		Global,
		Browser,
		ShellBrowser
	};

	static NavigationEventScope Global();
	static NavigationEventScope ForBrowser(const BrowserWindow &browser);
	static NavigationEventScope ForShellBrowser(const ShellBrowser &shellBrowser);

	Scope GetScope() const;
	std::optional<int> GetBrowserId() const;
	std::optional<int> GetShellBrowserId() const;

private:
	NavigationEventScope();
	NavigationEventScope(const BrowserWindow &browser);
	NavigationEventScope(const ShellBrowser &shellBrowser);

	const Scope m_scope;
	const std::optional<int> m_browserId;
	const std::optional<int> m_shellBrowserId;
};

class NavigationEvents : private boost::noncopyable
{
public:
	using FailedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser,
		const NavigationRequest *request)>;

	using StoppedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser)>;

	// Triggered when the enumeration for a navigation fails. If the initial navigation fails, this
	// won't be triggered, as the navigation will be committed instead.
	boost::signals2::connection AddFailedObserver(const FailedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	// Triggered when the pending navigations are requested to stop. At the point at which this
	// observer is invoked, the navigations will still be pending and may still be active.
	boost::signals2::connection AddStoppedObserver(const StoppedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back);

	void NotifyFailed(const ShellBrowser *shellBrowser, const NavigationRequest *request);

	void NotifyStopped(const ShellBrowser *shellBrowser);

private:
	template <typename Observer>
	static auto MakeFilteredObserver(Observer &&observer, const NavigationEventScope &scope)
	{
		return [observer = std::forward<Observer>(observer),
				   scope]<typename... Args>(const ShellBrowser *shellBrowser, Args &&...args)
		{
			if (!DoesEventMatchScope(scope, shellBrowser))
			{
				return;
			}

			observer(shellBrowser, std::forward<Args>(args)...);
		};
	}

	static bool DoesEventMatchScope(const NavigationEventScope &scope,
		const ShellBrowser *shellBrowser);

	FailedSignal m_failedSignal;

	StoppedSignal m_stoppedSignal;
};
