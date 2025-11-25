// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NavigationRequestDelegate.h"
#include "../Helper/Pidl.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>

struct NavigateParams;
class NavigationEvents;
class NavigationRequest;
class ScopedStopSource;
class ShellBrowser;
class ShellEnumerator;

// This class is responsible for managing ongoing navigations and allowing new navigations to be
// started.
class NavigationManager : private NavigationRequestDelegate
{
public:
	NavigationManager(const ShellBrowser *shellBrowser, NavigationEvents *navigationEvents,
		std::shared_ptr<const ShellEnumerator> shellEnumerator,
		std::shared_ptr<concurrencpp::executor> enumerationExecutor,
		std::shared_ptr<concurrencpp::executor> originalExecutor);
	~NavigationManager();

	void StartNavigation(const NavigateParams &navigateParams);

	// Stops all in-progress navigations.
	void StopLoading();

	// Returns the list of pending navigations, with more recent navigations appearing last.
	//
	// A pending navigation is one that is still in progress. This includes both navigations that
	// will be cancelled once they return to the main thread (e.g. because a navigation was
	// committed in the meantime), as well as active navigations that can still be committed once
	// they return.
	concurrencpp::generator<const NavigationRequest *> GetPendingNavigations() const;

	const NavigationRequest *MaybeGetLatestPendingNavigation() const;
	size_t GetNumPendingNavigations() const;
	bool HasAnyPendingNavigations() const;

	// Returns the list of active navigations, with more recent navigations appearing last.
	//
	// An active navigation is one that may commit once it returns to the main thread. This
	// explicitly excludes navigations that will be cancelled but are technically still in progress.
	concurrencpp::generator<const NavigationRequest *> GetActiveNavigations() const;

	const NavigationRequest *MaybeGetLatestActiveNavigation() const;
	size_t GetNumActiveNavigations() const;
	bool HasAnyActiveNavigations() const;

private:
	// NavigationRequestListener
	void OnEnumerationCompleted(NavigationRequest *request) override;
	void OnEnumerationFailed(NavigationRequest *request) override;
	void OnEnumerationStopped(NavigationRequest *request) override;
	void OnFinished(NavigationRequest *request) override;

	void CommitNavigation(NavigationRequest *request);

	void RemoveNavigationRequest(NavigationRequest *request);

	bool ActiveNavigationFilter(const std::unique_ptr<NavigationRequest> &pendingNavigation) const;

	const ShellBrowser *const m_shellBrowser;
	NavigationEvents *const m_navigationEvents;
	const std::shared_ptr<const ShellEnumerator> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::executor> m_enumerationExecutor;
	const std::shared_ptr<concurrencpp::executor> m_originalExecutor;

	bool m_anyNavigationsCommitted = false;
	std::vector<std::unique_ptr<NavigationRequest>> m_pendingNavigations;

	std::unique_ptr<ScopedStopSource> m_scopedStopSource;
};
