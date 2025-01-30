// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowser.h"
#include "SignalWrapper.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/WeakPtrFactory.h"

class PreservedHistoryEntry;
class ShellNavigationController;
class TabNavigationInterface;

class NavigationRequest
{
public:
	enum class Type
	{
		Sync,
		Async
	};

	NavigationRequest(const NavigateParams &navigateParams, Type type);

	virtual ~NavigationRequest() = default;

	void Start();

	SignalWrapper<NavigationRequest, void(const NavigateParams &navigateParams)>
		navigationStartedSignal;
	SignalWrapper<NavigationRequest, void(const NavigateParams &navigateParams)>
		enumerationCompletedSignal;
	SignalWrapper<NavigationRequest, void(const NavigateParams &navigateParams)>
		enumerationFailedSignal;

protected:
	void Complete();
	void Fail();

private:
	enum class Stage
	{
		NotStarted,
		Started,
		Completed
	};

	const NavigateParams m_navigateParams;
	const Type m_type;
	Stage m_stage = Stage::NotStarted;
};

class NavigationRequestSync : public NavigationRequest
{
public:
	NavigationRequestSync(const NavigateParams &navigateParams);
};

class NavigationRequestAsync : public NavigationRequest
{
public:
	NavigationRequestAsync(const NavigateParams &navigateParams);

	using NavigationRequest::Complete;
	using NavigationRequest::Fail;
};

class ShellBrowserFake : public ShellBrowser
{
public:
	enum class NavigationMode
	{
		// In this mode, the simulated navigation will complete synchronously (within the Navigate
		// call).
		Sync,

		// In this mode, the navigation is started within the Navigate call and then completed by
		// the caller.
		Async
	};

	ShellBrowserFake(TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);
	ShellBrowserFake(TabNavigationInterface *tabNavigation);
	~ShellBrowserFake();

	void NavigateToPath(const std::wstring &path,
		HistoryEntryType addHistoryType = HistoryEntryType::AddEntry,
		PidlAbsolute *outputPidl = nullptr);
	void SetNavigationMode(NavigationMode navigationMode);
	NavigationRequestAsync *GetLastAsyncNavigationRequest() const;

	// ShellBrowser
	FolderSettings GetFolderSettings() const override;
	ShellNavigationController *GetNavigationController() const override;
	void AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper) override;

	// ShellNavigator
	void Navigate(NavigateParams &navigateParams) override;
	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

private:
	void SetUpNavigationRequestListeners(NavigationRequest *request);

	std::unique_ptr<ShellNavigationController> m_navigationController;
	std::vector<std::unique_ptr<ShellBrowserHelperBase>> m_helpers;

	NavigationMode m_navigationMode = NavigationMode::Sync;
	std::unique_ptr<NavigationRequestAsync> m_lastAsyncNavigationRequest;

	NavigationStartedSignal m_navigationStartedSignal;
	NavigationCommittedSignal m_navigationCommittedSignal;
	NavigationCompletedSignal m_navigationCompletedSignal;
	NavigationFailedSignal m_navigationFailedSignal;

	WeakPtrFactory<ShellBrowserFake> m_weakPtrFactory{ this };
};
