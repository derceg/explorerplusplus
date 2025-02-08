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

	enum class ExecutionMode
	{
		Sync,
		Async
	};

	enum class SlotGroup
	{
		HighPriority = 0,
		Default = 1
	};

	NavigationManager(ExecutionMode executionMode,
		std::shared_ptr<const ShellEnumerator> shellEnumerator,
		std::shared_ptr<concurrencpp::executor> comStaExecutor,
		std::shared_ptr<concurrencpp::executor> originalExecutor);
	~NavigationManager();

	void StartNavigation(const NavigateParams &navigateParams);
	int GetNumPendingNavigations() const;
	bool HasAnyPendingNavigations() const;

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
	static concurrencpp::null_result StartNavigationInternal(WeakPtr<NavigationManager> weakSelf,
		NavigateParams navigateParams);

	void OnNavigationStarted(const NavigateParams &navigateParams);
	void OnEnumerationCompleted(const NavigateParams &navigateParams,
		const std::vector<PidlChild> &items);
	void OnEnumerationFailed(const NavigateParams &navigateParams);
	void OnNavigationCancelled(const NavigateParams &navigateParams);

	const ExecutionMode m_executionMode;
	const std::shared_ptr<const ShellEnumerator> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::executor> m_comStaExecutor;
	const std::shared_ptr<concurrencpp::executor> m_originalExecutor;

	bool m_anyNavigationsCommitted = false;
	int m_numPendingNavigations = 0;

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
