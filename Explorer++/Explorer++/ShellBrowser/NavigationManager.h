// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellNavigator.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>

class ScopedStopSource;
class ShellEnumerator;

// This class is responsible for managing ongoing navigations and allowing new navigations to be
// started.
class NavigationManager
{
public:
	using NavigationStartedSignal =
		boost::signals2::signal<void(const NavigateParams &navigateParams)>;

	using NavigationWillCommitSignal =
		boost::signals2::signal<void(const NavigateParams &navigateParams)>;
	using NavigationCommittedSignal =
		boost::signals2::signal<void(const NavigateParams &navigateParams)>;
	using NavigationItemsAvailableSignal = boost::signals2::signal<void(
		const NavigateParams &navigateParams, const std::vector<PidlChild> &items)>;
	using NavigationCompletedSignal =
		boost::signals2::signal<void(const NavigateParams &navigateParams)>;

	using NavigationFailedSignal =
		boost::signals2::signal<void(const NavigateParams &navigateParams)>;

	using NavigationCancelledSignal =
		boost::signals2::signal<void(const NavigateParams &navigateParams)>;

	enum class SlotGroup
	{
		HighestPriority = 0,
		HighPriority = 1,
		Default = 2
	};

	NavigationManager(std::shared_ptr<const ShellEnumerator> shellEnumerator,
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
	concurrencpp::generator<const NavigateParams &> GetPendingNavigations() const;

	const NavigateParams *MaybeGetLatestPendingNavigation() const;
	size_t GetNumPendingNavigations() const;
	bool HasAnyPendingNavigations() const;

	// Returns the list of active navigations, with more recent navigations appearing last.
	//
	// An active navigation is one that may commit once it returns to the main thread. This
	// explicitly excludes navigations that will be cancelled but are technically still in progress.
	concurrencpp::generator<const NavigateParams &> GetActiveNavigations() const;

	const NavigateParams *MaybeGetLatestActiveNavigation() const;
	size_t GetNumActiveNavigations() const;
	bool HasAnyActiveNavigations() const;

	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	boost::signals2::connection AddNavigationWillCommitObserver(
		const NavigationWillCommitSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);
	boost::signals2::connection AddNavigationItemsAvailableObserver(
		const NavigationItemsAvailableSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

	boost::signals2::connection AddNavigationCancelledObserver(
		const NavigationCancelledSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back,
		SlotGroup slotGroup = SlotGroup::Default);

private:
	struct PendingNavigation
	{
		NavigateParams navigateParams;
		std::stop_token stopToken;
	};

	static concurrencpp::null_result StartNavigationInternal(WeakPtr<NavigationManager> weakSelf,
		NavigateParams navigateParams);

	PendingNavigation *AddPendingNavigation(std::unique_ptr<PendingNavigation> pendingNavigation);
	void RemovePendingNavigation(PendingNavigation *pendingNavigation);

	void OnNavigationStarted(const NavigateParams &navigateParams);
	void OnEnumerationCompleted(const NavigateParams &navigateParams,
		const std::vector<PidlChild> &items);
	void OnEnumerationFailed(const NavigateParams &navigateParams);
	void OnEnumerationStopped(const NavigateParams &navigateParams);

	bool ActiveNavigationFilter(const std::unique_ptr<PendingNavigation> &pendingNavigation) const;

	const std::shared_ptr<const ShellEnumerator> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::executor> m_enumerationExecutor;
	const std::shared_ptr<concurrencpp::executor> m_originalExecutor;

	bool m_anyNavigationsCommitted = false;
	std::vector<std::unique_ptr<PendingNavigation>> m_pendingNavigations;

	NavigationStartedSignal m_navigationStartedSignal;

	NavigationWillCommitSignal m_navigationWillCommitSignal;
	NavigationCommittedSignal m_navigationCommittedSignal;
	NavigationItemsAvailableSignal m_navigationItemsAvailableSignal;
	NavigationCompletedSignal m_navigationCompletedSignal;

	NavigationFailedSignal m_navigationFailedSignal;

	NavigationCancelledSignal m_navigationCancelledSignal;

	std::unique_ptr<ScopedStopSource> m_scopedStopSource;

	WeakPtrFactory<NavigationManager> m_weakPtrFactory{ this };
};
