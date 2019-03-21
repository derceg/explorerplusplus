// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/iShellView.h"
#include "../Helper/Macros.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>

// Used when creating a tab.
struct TabSettings
{
	boost::optional<std::wstring> name;
	boost::optional<int> index;
	boost::optional<bool> locked;
	boost::optional<bool> addressLocked;
};

class Tab
{
public:

	enum class PropertyType
	{
		LOCKED,
		ADDRESS_LOCKED,
		NAME
	};

	typedef boost::signals2::signal<void(const Tab &tab, PropertyType propertyType)> TabUpdatedSignal;

	Tab(int id);

	int GetId() const;

	CShellBrowser *GetShellBrowser() const;
	void SetShellBrowser(CShellBrowser *shellBrowser);

	std::wstring GetName() const;
	bool GetUseCustomName() const;
	void SetCustomName(const std::wstring &name);
	void ClearCustomName();

	bool GetLocked() const;
	void SetLocked(bool locked);
	bool GetAddressLocked() const;
	void SetAddressLocked(bool addressLocked);

	boost::signals2::connection AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer);

	HWND	listView;

	/* Although each tab manages its
	own columns, it does not know
	about any column defaults.
	Therefore, it makes more sense
	for this setting to remain here. */
	//BOOL	bUsingDefaultColumns;

private:

	DISALLOW_COPY_AND_ASSIGN(Tab);

	const int m_id;
	CShellBrowser *m_shellBrowser;
	bool m_useCustomName;
	std::wstring m_customName;
	bool m_locked;
	bool m_addressLocked;

	TabUpdatedSignal m_tabUpdatedSignal;
};