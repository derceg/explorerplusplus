// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationManager.h"
#include "NavigationEvents.h"
#include "NavigationRequest.h"
#include "ShellEnumerator.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/ShellHelper.h"
#include <algorithm>

NavigationManager::NavigationManager(const ShellBrowser *shellBrowser,
	NavigationEvents *navigationEvents, std::shared_ptr<const ShellEnumerator> shellEnumerator,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor) :
	m_shellBrowser(shellBrowser),
	m_navigationEvents(navigationEvents),
	m_shellEnumerator(shellEnumerator),
	m_enumerationExecutor(enumerationExecutor),
	m_originalExecutor(originalExecutor),
	m_scopedStopSource(std::make_unique<ScopedStopSource>())
{
}

NavigationManager::~NavigationManager() = default;

void NavigationManager::StartNavigation(const NavigateParams &navigateParams)
{
	auto navigationRequest = std::make_unique<NavigationRequest>(m_shellBrowser, m_navigationEvents,
		static_cast<NavigationRequestDelegate *>(this), m_shellEnumerator, m_enumerationExecutor,
		m_originalExecutor, navigateParams, m_scopedStopSource->GetToken());
	auto *rawNavigationRequest = navigationRequest.get();
	m_pendingNavigations.push_back(std::move(navigationRequest));

	rawNavigationRequest->Start();
}

void NavigationManager::OnEnumerationCompleted(NavigationRequest *request)
{
	CommitNavigation(request);
}

void NavigationManager::OnEnumerationFailed(NavigationRequest *request)
{
	if (!m_anyNavigationsCommitted)
	{
		// Typically, when a navigation fails, nothing will happen. The original folder will
		// continue to be shown. However, when the initial navigation fails, there is no original
		// folder, so the only reasonable choice is to commit the failed navigation, regardless.
		CommitNavigation(request);
		return;
	}

	request->Fail();
}

void NavigationManager::OnEnumerationStopped(NavigationRequest *request)
{
	if (!m_anyNavigationsCommitted)
	{
		CommitNavigation(request);
		return;
	}

	request->Cancel();
}

void NavigationManager::OnFinished(NavigationRequest *request)
{
	RemoveNavigationRequest(request);
}

void NavigationManager::CommitNavigation(NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	// The specified navigation will be committed, so all other navigations should be cancelled.
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	m_anyNavigationsCommitted = true;

	request->Commit();
}

void NavigationManager::RemoveNavigationRequest(NavigationRequest *request)
{
	auto itr = std::ranges::find_if(m_pendingNavigations,
		[request](const auto &navigationRequest) { return navigationRequest.get() == request; });
	CHECK(itr != m_pendingNavigations.end());
	m_pendingNavigations.erase(itr);
}

void NavigationManager::StopLoading()
{
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	m_navigationEvents->NotifyStopped(m_shellBrowser);
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<const NavigationRequest *> NavigationManager::GetPendingNavigations() const
{
	for (const auto &pendingNavigation : m_pendingNavigations)
	{
		co_yield pendingNavigation.get();
	}
}

const NavigationRequest *NavigationManager::MaybeGetLatestPendingNavigation() const
{
	if (m_pendingNavigations.empty())
	{
		return nullptr;
	}

	const auto &lastItem = *m_pendingNavigations.rbegin();
	return lastItem.get();
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
concurrencpp::generator<const NavigationRequest *> NavigationManager::GetActiveNavigations() const
{
	for (const auto &pendingNavigation : m_pendingNavigations
			| std::views::filter(std::bind_front(&NavigationManager::ActiveNavigationFilter, this)))
	{
		co_yield pendingNavigation.get();
	}
}

const NavigationRequest *NavigationManager::MaybeGetLatestActiveNavigation() const
{
	auto itr = std::ranges::find_if(m_pendingNavigations.rbegin(), m_pendingNavigations.rend(),
		std::bind_front(&NavigationManager::ActiveNavigationFilter, this));

	if (itr == m_pendingNavigations.rend())
	{
		return nullptr;
	}

	return itr->get();
}

size_t NavigationManager::GetNumActiveNavigations() const
{
	return std::ranges::count_if(m_pendingNavigations,
		std::bind_front(&NavigationManager::ActiveNavigationFilter, this));
}

bool NavigationManager::ActiveNavigationFilter(
	const std::unique_ptr<NavigationRequest> &pendingNavigation) const
{
	if (!m_anyNavigationsCommitted)
	{
		// In this case, any pending navigation has the ability to commit, so every pending
		// navigation is considered active.
		return true;
	}

	// Typically, only navigations that haven't been stopped are considered active.
	return !pendingNavigation->Stopped();
}

bool NavigationManager::HasAnyActiveNavigations() const
{
	return GetNumActiveNavigations() > 0;
}
