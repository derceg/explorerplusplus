// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellNavigator.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/core/noncopyable.hpp>
#include <concurrencpp/concurrencpp.h>
#include <vector>

class NavigationEvents;
class NavigationRequestListener;
class ShellBrowser;
class ShellEnumerator;

// Manages a single navigation. An instance of this class may be destroyed at any point after
// `Start` is called.
class NavigationRequest : private boost::noncopyable
{
public:
	enum class State
	{
		NotStarted,
		Started,
		EnumerationFinished,
		WillCommit,
		Committed,
		Failed,
		Cancelled
	};

	NavigationRequest(const ShellBrowser *shellBrowser, NavigationEvents *navigationEvents,
		NavigationRequestListener *listener, std::shared_ptr<const ShellEnumerator> shellEnumerator,
		std::shared_ptr<concurrencpp::executor> enumerationExecutor,
		std::shared_ptr<concurrencpp::executor> originalExecutor,
		const NavigateParams &navigateParams, std::stop_token stopToken);

	void Start();
	void Commit();
	void Fail();
	void Cancel();

	State GetState() const;
	const NavigateParams &GetNavigateParams() const;

	// Indicates whether the enumeration process was stopped early. Note that this is independent of
	// whether the navigation is ultimately committed or cancelled. That is, it's up to the caller
	// to decide whether a stopped enumeration should result in a cancellation or not.
	bool Stopped() const;

private:
	static concurrencpp::null_result StartInternal(WeakPtr<NavigationRequest> weakSelf);

	void SetState(State state);

	const ShellBrowser *const m_shellBrowser;
	NavigationEvents *const m_navigationEvents;
	NavigationRequestListener *const m_listener;
	const std::shared_ptr<const ShellEnumerator> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::executor> m_enumerationExecutor;
	const std::shared_ptr<concurrencpp::executor> m_originalExecutor;

	// The target pidl can be updated during the navigation (e.g. if the target is a symlink), so
	// the values in this struct can change during the lifetime of the request.
	NavigateParams m_navigateParams;

	std::stop_token m_stopToken;

	State m_state = State::NotStarted;
	std::vector<PidlChild> m_items;

	WeakPtrFactory<NavigationRequest> m_weakPtrFactory{ this };
};
