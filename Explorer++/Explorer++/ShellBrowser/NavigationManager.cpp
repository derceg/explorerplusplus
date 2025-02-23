// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationManager.h"
#include "ShellEnumerator.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/ShellHelper.h"
#include <algorithm>

NavigationManager::NavigationManager(std::shared_ptr<const ShellEnumerator> shellEnumerator,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor) :
	m_shellEnumerator(shellEnumerator),
	m_enumerationExecutor(enumerationExecutor),
	m_originalExecutor(originalExecutor),
	m_scopedStopSource(std::make_unique<ScopedStopSource>())
{
}

NavigationManager::~NavigationManager() = default;

void NavigationManager::StartNavigation(const NavigateParams &navigateParams)
{
	StartNavigationInternal(m_weakPtrFactory.GetWeakPtr(), navigateParams);
}

concurrencpp::null_result NavigationManager::StartNavigationInternal(
	WeakPtr<NavigationManager> weakSelf, NavigateParams navigateParams)
{
	// It's not safe to access this object once the coroutine here has switched to a different
	// thread, making it necessary to retrieve these values up front.
	auto shellEnumerator = weakSelf->m_shellEnumerator;
	auto enumerationExecutor = weakSelf->m_enumerationExecutor;
	auto originalExecutor = weakSelf->m_originalExecutor;
	auto stopToken = weakSelf->m_scopedStopSource->GetToken();

	auto *pendingNavigation = weakSelf->AddPendingNavigation(
		std::make_unique<PendingNavigation>(navigateParams, stopToken));

	weakSelf->OnNavigationStarted(navigateParams);

	co_await concurrencpp::resume_on(enumerationExecutor);

	// Note that although standard shortcuts (.lnk files) are currently handled outside this class,
	// symlinks and virtual link objects aren't, so they will be handled here.
	//
	// Navigating to the target pidl is important for folders like the quick access folder. Although
	// navigating directly to a recent/pinned folder works as expected, directory monitoring doesn't
	// work. Presumably, that's because directory change notifications are only generated for the
	// original (target) directory. To have things work correctly, the navigation needs to proceed
	// to the original folder instead.
	//
	// Note that this call simply retrieves the target item, but doesn't attempt to resolve it. That
	// matches the behavior of Explorer. For example, if a symlink to a directory is created and the
	// target directory is then removed, Explorer will try to navigate to the target, without
	// attempting to resolve the link.
	unique_pidl_absolute targetPidl;
	HRESULT hr = MaybeGetLinkTarget(navigateParams.pidl.Raw(), targetPidl);

	if (SUCCEEDED(hr))
	{
		navigateParams.pidl = targetPidl.get();
	}

	std::vector<PidlChild> items;
	hr = shellEnumerator->EnumerateDirectory(navigateParams.pidl.Raw(), items, stopToken);

	co_await concurrencpp::resume_on(originalExecutor);

	if (!weakSelf)
	{
		co_return;
	}

	// This is set up only once this coroutine is back on the original thread, as that's the only
	// place where `weakSelf` can be properly accessed.
	auto removePendingNavigation = wil::scope_exit(
		[weakSelf, pendingNavigation]() { weakSelf->RemovePendingNavigation(pendingNavigation); });

	if (stopToken.stop_requested())
	{
		weakSelf->OnEnumerationStopped(navigateParams);
		co_return;
	}

	if (FAILED(hr))
	{
		weakSelf->OnEnumerationFailed(navigateParams);
		co_return;
	}

	weakSelf->OnEnumerationCompleted(navigateParams, items);
}

NavigationManager::PendingNavigation *NavigationManager::AddPendingNavigation(
	std::unique_ptr<PendingNavigation> pendingNavigation)
{
	auto *pendingNavigationRaw = pendingNavigation.get();
	m_pendingNavigations.push_back(std::move(pendingNavigation));
	return pendingNavigationRaw;
}

void NavigationManager::RemovePendingNavigation(PendingNavigation *pendingNavigation)
{
	auto itr = std::ranges::find_if(m_pendingNavigations,
		[pendingNavigation](const auto &current) { return current.get() == pendingNavigation; });
	CHECK(itr != m_pendingNavigations.end());
	m_pendingNavigations.erase(itr);
}

void NavigationManager::OnNavigationStarted(const NavigateParams &navigateParams)
{
	m_navigationStartedSignal(navigateParams);
}

void NavigationManager::OnEnumerationCompleted(const NavigateParams &navigateParams,
	const std::vector<PidlChild> &items)
{
	// This navigation will be committed, so all other navigations should be cancelled.
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	m_anyNavigationsCommitted = true;

	m_navigationWillCommitSignal(navigateParams);
	m_navigationCommittedSignal(navigateParams, items);
}

void NavigationManager::OnEnumerationFailed(const NavigateParams &navigateParams)
{
	if (!m_anyNavigationsCommitted)
	{
		// Typically, when a navigation fails, nothing will happen. The original folder will
		// continue to be shown. However, when the initial navigation fails, there is no original
		// folder, so the only reasonable choice is to commit the failed navigation, regardless.
		OnEnumerationCompleted(navigateParams, {});
		return;
	}

	m_navigationFailedSignal(navigateParams);
}

void NavigationManager::OnEnumerationStopped(const NavigateParams &navigateParams)
{
	if (!m_anyNavigationsCommitted)
	{
		OnEnumerationCompleted(navigateParams, {});
		return;
	}

	m_navigationCancelledSignal(navigateParams);
}

void NavigationManager::StopLoading()
{
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	m_navigationsStoppedSignal();
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<const NavigateParams &> NavigationManager::GetPendingNavigations() const
{
	for (const auto &pendingNavigation : m_pendingNavigations)
	{
		co_yield pendingNavigation->navigateParams;
	}
}

const NavigateParams *NavigationManager::MaybeGetLatestPendingNavigation() const
{
	if (m_pendingNavigations.empty())
	{
		return nullptr;
	}

	const auto &lastItem = *m_pendingNavigations.rbegin();
	return &lastItem->navigateParams;
}

size_t NavigationManager::GetNumPendingNavigations() const
{
	return m_pendingNavigations.size();
}

bool NavigationManager::HasAnyPendingNavigations() const
{
	return !m_pendingNavigations.empty();
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<const NavigateParams &> NavigationManager::GetActiveNavigations() const
{
	for (const auto &pendingNavigation : m_pendingNavigations
			| std::views::filter(std::bind_front(&NavigationManager::ActiveNavigationFilter, this)))
	{
		co_yield pendingNavigation->navigateParams;
	}
}

const NavigateParams *NavigationManager::MaybeGetLatestActiveNavigation() const
{
	auto itr = std::ranges::find_if(m_pendingNavigations.rbegin(), m_pendingNavigations.rend(),
		std::bind_front(&NavigationManager::ActiveNavigationFilter, this));

	if (itr == m_pendingNavigations.rend())
	{
		return nullptr;
	}

	return &(*itr)->navigateParams;
}

size_t NavigationManager::GetNumActiveNavigations() const
{
	return std::ranges::count_if(m_pendingNavigations,
		std::bind_front(&NavigationManager::ActiveNavigationFilter, this));
}

bool NavigationManager::ActiveNavigationFilter(
	const std::unique_ptr<PendingNavigation> &pendingNavigation) const
{
	if (!m_anyNavigationsCommitted)
	{
		// In this case, any pending navigation has the ability to commit, so every pending
		// navigation is considered active.
		return true;
	}

	// Typically, only navigations that haven't been stopped are considered active.
	return !pendingNavigation->stopToken.stop_requested();
}

bool NavigationManager::HasAnyActiveNavigations() const
{
	return GetNumActiveNavigations() > 0;
}

boost::signals2::connection NavigationManager::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, boost::signals2::connect_position position,
	SlotGroup slotGroup)
{
	return m_navigationStartedSignal.connect(static_cast<int>(slotGroup), observer, position);
}

boost::signals2::connection NavigationManager::AddNavigationWillCommitObserver(
	const NavigationWillCommitSignal::slot_type &observer,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_navigationWillCommitSignal.connect(static_cast<int>(slotGroup), observer, position);
}

boost::signals2::connection NavigationManager::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_navigationCommittedSignal.connect(static_cast<int>(slotGroup), observer, position);
}

boost::signals2::connection NavigationManager::AddNavigationFailedObserver(
	const NavigationFailedSignal::slot_type &observer, boost::signals2::connect_position position,
	SlotGroup slotGroup)
{
	return m_navigationFailedSignal.connect(static_cast<int>(slotGroup), observer, position);
}

boost::signals2::connection NavigationManager::AddNavigationCancelledObserver(
	const NavigationCancelledSignal::slot_type &observer,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_navigationCancelledSignal.connect(static_cast<int>(slotGroup), observer, position);
}

boost::signals2::connection NavigationManager::AddNavigationsStoppedObserver(
	const NavigationsStoppedSignal::slot_type &observer, boost::signals2::connect_position position,
	SlotGroup slotGroup)
{
	return m_navigationsStoppedSignal.connect(static_cast<int>(slotGroup), observer, position);
}
