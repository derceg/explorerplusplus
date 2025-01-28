// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFake.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowserHelper.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "../Helper/WeakPtr.h"
#include <gtest/gtest.h>

NavigationRequest::NavigationRequest(const NavigateParams &navigateParams, Type type) :
	m_navigateParams(navigateParams),
	m_type(type)
{
}

void NavigationRequest::Start()
{
	ASSERT_EQ(m_stage, Stage::NotStarted);

	m_stage = Stage::Started;
	navigationStartedSignal.m_signal(m_navigateParams);

	if (m_type == Type::Sync)
	{
		m_stage = Stage::Completed;
		enumerationCompletedSignal.m_signal(m_navigateParams);
	}
}

void NavigationRequest::Complete()
{
	ASSERT_EQ(m_stage, Stage::Started);

	m_stage = Stage::Completed;
	enumerationCompletedSignal.m_signal(m_navigateParams);
}

void NavigationRequest::Fail()
{
	ASSERT_EQ(m_stage, Stage::Started);

	m_stage = Stage::Completed;
	enumerationFailedSignal.m_signal(m_navigateParams);
}

NavigationRequestSync::NavigationRequestSync(const NavigateParams &navigateParams) :
	NavigationRequest(navigateParams, Type::Sync)
{
}

NavigationRequestAsync::NavigationRequestAsync(const NavigateParams &navigateParams) :
	NavigationRequest(navigateParams, Type::Async)
{
}

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, tabNavigation,
		preservedEntries, currentEntry);
}

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, tabNavigation);
}

ShellBrowserFake::~ShellBrowserFake() = default;

// Although the ShellNavigationController can navigate to a path (by transforming it into a pidl),
// it requires that the path exist. This function will transform the path into a simple pidl, which
// doesn't require the path to exist.
HRESULT ShellBrowserFake::NavigateToPath(const std::wstring &path, HistoryEntryType addHistoryType,
	PidlAbsolute *outputPidl)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw(), addHistoryType);
	HRESULT hr = m_navigationController->Navigate(navigateParams);

	if (outputPidl)
	{
		*outputPidl = pidl;
	}

	return hr;
}

void ShellBrowserFake::SetNavigationMode(NavigationMode navigationMode)
{
	if (navigationMode == m_navigationMode)
	{
		return;
	}

	m_navigationMode = navigationMode;
	m_lastAsyncNavigationRequest.reset();
}

NavigationRequestAsync *ShellBrowserFake::GetLastAsyncNavigationRequest() const
{
	return m_lastAsyncNavigationRequest.get();
}

FolderSettings ShellBrowserFake::GetFolderSettings() const
{
	return {};
}

ShellNavigationController *ShellBrowserFake::GetNavigationController() const
{
	return m_navigationController.get();
}

void ShellBrowserFake::AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper)
{
	m_helpers.push_back(std::move(helper));
}

HRESULT ShellBrowserFake::Navigate(NavigateParams &navigateParams)
{
	std::unique_ptr<NavigationRequest> ownedRequest;
	NavigationRequest *request = nullptr;

	if (m_navigationMode == NavigationMode::Sync)
	{
		ownedRequest = std::make_unique<NavigationRequestSync>(navigateParams);
		request = ownedRequest.get();
	}
	else
	{
		m_lastAsyncNavigationRequest = std::make_unique<NavigationRequestAsync>(navigateParams);
		request = m_lastAsyncNavigationRequest.get();
	}

	SetUpNavigationRequestListeners(request);
	request->Start();

	return S_OK;
}

void ShellBrowserFake::SetUpNavigationRequestListeners(NavigationRequest *request)
{
	auto weakSelf = m_weakPtrFactory.GetWeakPtr();

	request->navigationStartedSignal.AddObserver(
		[weakSelf](const NavigateParams &navigateParams)
		{
			if (weakSelf)
			{
				weakSelf->m_navigationStartedSignal(navigateParams);
			}
		});

	request->enumerationCompletedSignal.AddObserver(
		[weakSelf](const NavigateParams &navigateParams)
		{
			if (weakSelf)
			{
				weakSelf->m_navigationCommittedSignal(navigateParams);
				weakSelf->m_navigationCompletedSignal(navigateParams);

				// This will cause any other simulated async navigation requests to be ignored.
				weakSelf->m_weakPtrFactory.InvalidateWeakPtrs();
			}
		});

	request->enumerationFailedSignal.AddObserver(
		[weakSelf](const NavigateParams &navigateParams)
		{
			if (weakSelf)
			{
				weakSelf->m_navigationFailedSignal(navigateParams);
			}
		});
}

boost::signals2::connection ShellBrowserFake::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return m_navigationStartedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowserFake::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCommittedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowserFake::AddNavigationCompletedObserver(
	const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCompletedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowserFake::AddNavigationFailedObserver(
	const NavigationFailedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return m_navigationFailedSignal.connect(observer, position);
}
