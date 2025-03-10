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
	using StartedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser,
		const NavigationRequest *request)>;
	using WillCommitSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser,
		const NavigationRequest *request)>;
	using CommittedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser,
		const NavigationRequest *request)>;
	using FailedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser,
		const NavigationRequest *request)>;

	using StoppedSignal = boost::signals2::signal<void(const ShellBrowser *shellBrowser)>;

	enum class SlotGroup
	{
		HighestPriority = 0,
		HighPriority = 1,
		Default = 2
	};

	// Triggered when a navigation is initiated.
	boost::signals2::connection AddStartedObserver(const StartedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when it's known that a navigation will commit, but the commit hasn't yet occurred.
	boost::signals2::connection AddWillCommitObserver(const WillCommitSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when the enumeration for a directory successfully finishes. At this point, the
	// requested folder has become the current folder and the enumerated items have been displayed.
	//
	// This can also be triggered if the initial navigation fails. Typically, if a navigation fails,
	// no folder change will occur. Instead, the original folder will continue to be shown. However,
	// when the first navigation occurs, there is no original folder. Because a folder always needs
	// to be displayed, the only effective option is to consider the failed navigation committed. In
	// that case, this event will be triggered, while the failed event won't be triggered.
	//
	// For the same reasons, this can also be triggered if the initial navigation is stopped early.
	boost::signals2::connection AddCommittedObserver(const CommittedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when the enumeration for a navigation fails. If the initial navigation fails, this
	// won't be triggered, as the navigation will be committed instead.
	boost::signals2::connection AddFailedObserver(const FailedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	// Triggered when the pending navigations are requested to stop. At the point at which this
	// observer is invoked, the navigations will still be pending and may still be active.
	boost::signals2::connection AddStoppedObserver(const StoppedSignal::slot_type &observer,
		const NavigationEventScope &scope,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	void NotifyStarted(const ShellBrowser *shellBrowser, const NavigationRequest *request);
	void NotifyWillCommit(const ShellBrowser *shellBrowser, const NavigationRequest *request);
	void NotifyCommitted(const ShellBrowser *shellBrowser, const NavigationRequest *request);
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

	StartedSignal m_startedSignal;
	WillCommitSignal m_willCommitSignal;
	CommittedSignal m_committedSignal;
	FailedSignal m_failedSignal;

	StoppedSignal m_stoppedSignal;
};
