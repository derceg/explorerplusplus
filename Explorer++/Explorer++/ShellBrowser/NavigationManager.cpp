// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationManager.h"
#include "ShellEnumerator.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/ShellHelper.h"

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
	weakSelf->m_numPendingNavigations++;

	weakSelf->OnNavigationStarted(navigateParams);

	// It's not safe to access this object once the coroutine here has switched to a different
	// thread, making it necessary to retrieve these values up front.
	auto shellEnumerator = weakSelf->m_shellEnumerator;
	auto enumerationExecutor = weakSelf->m_enumerationExecutor;
	auto originalExecutor = weakSelf->m_originalExecutor;
	auto stopToken = weakSelf->m_scopedStopSource->GetToken();

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

	weakSelf->m_numPendingNavigations--;

	if (stopToken.stop_requested())
	{
		weakSelf->OnNavigationCancelled(navigateParams);
		co_return;
	}

	if (FAILED(hr))
	{
		weakSelf->OnEnumerationFailed(navigateParams);
		co_return;
	}

	weakSelf->OnEnumerationCompleted(navigateParams, items);
}

void NavigationManager::OnNavigationStarted(const NavigateParams &navigateParams)
{
	m_numActiveNavigations++;

	m_navigationStartedSignal(navigateParams);
}

void NavigationManager::OnEnumerationCompleted(const NavigateParams &navigateParams,
	const std::vector<PidlChild> &items)
{
	// This navigation will be committed, so all other navigations should be cancelled.
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	// This navigation is about to be committed and all other in progress navigations have been
	// cancelled. So, at this point, there no other navigations in progress that can commit.
	m_numActiveNavigations = 0;

	m_anyNavigationsCommitted = true;

	m_navigationWillCommitSignal(navigateParams);
	m_navigationCommittedSignal(navigateParams);
	m_navigationItemsAvailableSignal(navigateParams, items);
	m_navigationCompletedSignal(navigateParams);
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

	m_numActiveNavigations--;

	m_navigationFailedSignal(navigateParams);
}

void NavigationManager::OnNavigationCancelled(const NavigateParams &navigateParams)
{
	m_navigationCancelledSignal(navigateParams);
}

int NavigationManager::GetNumPendingNavigations() const
{
	return m_numPendingNavigations;
}

bool NavigationManager::HasAnyPendingNavigations() const
{
	return m_numPendingNavigations > 0;
}

int NavigationManager::GetNumActiveNavigations() const
{
	return m_numActiveNavigations;
}

bool NavigationManager::HasAnyActiveNavigations() const
{
	return m_numActiveNavigations > 0;
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

boost::signals2::connection NavigationManager::AddNavigationItemsAvailableObserver(
	const NavigationItemsAvailableSignal::slot_type &observer,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_navigationItemsAvailableSignal.connect(static_cast<int>(slotGroup), observer,
		position);
}

boost::signals2::connection NavigationManager::AddNavigationCompletedObserver(
	const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_navigationCompletedSignal.connect(static_cast<int>(slotGroup), observer, position);
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
