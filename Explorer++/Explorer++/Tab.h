// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabNavigationInterface.h"
#include "../Helper/Macros.h"
#include <boost/optional.hpp>
#include <boost/parameter.hpp>
#include <boost/signals2.hpp>

struct PreservedTab;

class Tab
{
public:

	enum class PropertyType
	{
		Name,
		LockState
	};

	enum class LockState
	{
		NotLocked,
		Locked,
		AddressLocked
	};

	typedef boost::signals2::signal<void(const Tab &tab, PropertyType propertyType)> TabUpdatedSignal;

	Tab(IExplorerplusplus *expp, TabNavigationInterface *tabNavigation, const FolderSettings *folderSettings,
		boost::optional<FolderColumns> initialColumns);
	Tab(const PreservedTab &preservedTab, IExplorerplusplus *expp, TabNavigationInterface *tabNavigation);

	int GetId() const;

	ShellBrowser *GetShellBrowser() const;

	std::wstring GetName() const;
	bool GetUseCustomName() const;
	void SetCustomName(const std::wstring &name);
	void ClearCustomName();

	LockState GetLockState() const;
	void SetLockState(LockState lockState);

	boost::signals2::connection AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer);

	/* Although each tab manages its
	own columns, it does not know
	about any column defaults.
	Therefore, it makes more sense
	for this setting to remain here. */
	//BOOL	bUsingDefaultColumns;

private:

	DISALLOW_COPY_AND_ASSIGN(Tab);

	static int idCounter;
	const int m_id;

	ShellBrowser *m_shellBrowser;

	bool m_useCustomName;
	std::wstring m_customName;
	LockState m_lockState;

	TabUpdatedSignal m_tabUpdatedSignal;
};