// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NavigationController.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/Macros.h"
#include <boost/optional.hpp>
#include <boost/parameter.hpp>
#include <boost/signals2.hpp>

BOOST_PARAMETER_NAME(name)
BOOST_PARAMETER_NAME(index)
BOOST_PARAMETER_NAME(selected)
BOOST_PARAMETER_NAME(locked)
BOOST_PARAMETER_NAME(addressLocked)

// The use of Boost Parameter here allows values to be set by name
// during construction. It would be better (and simpler) for this to be
// done using designated initializers, but that feature's not due to be
// introduced until C++20.
struct TabSettingsImpl
{
	template <class ArgumentPack>
	TabSettingsImpl(const ArgumentPack &args)
	{
		name = args[_name | boost::none];
		index = args[_index | boost::none];
		selected = args[_selected | boost::none];
		locked = args[_locked | boost::none];
		addressLocked = args[_addressLocked | boost::none];
	}

	boost::optional<std::wstring> name;
	boost::optional<int> index;
	boost::optional<bool> selected;
	boost::optional<bool> locked;
	boost::optional<bool> addressLocked;
};

// Used when creating a tab.
struct TabSettings : TabSettingsImpl
{
	BOOST_PARAMETER_CONSTRUCTOR(
		TabSettings,
		(TabSettingsImpl),
		tag,
		(optional
			(name, (std::wstring))
			(index, (int))
			(selected, (bool))
			(locked, (bool))
			(addressLocked, (bool))
		)
	)
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

	explicit Tab(int id);

	int GetId() const;

	NavigationController *GetNavigationController() const;

	CShellBrowser *GetShellBrowser() const;
	void SetShellBrowser(CShellBrowser *shellBrowser);

	HWND GetListView() const;
	void SetListView(HWND listView);

	std::wstring GetName() const;
	bool GetUseCustomName() const;
	void SetCustomName(const std::wstring &name);
	void ClearCustomName();

	bool GetLocked() const;
	void SetLocked(bool locked);
	bool GetAddressLocked() const;
	void SetAddressLocked(bool addressLocked);

	boost::signals2::connection AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer);

	/* Although each tab manages its
	own columns, it does not know
	about any column defaults.
	Therefore, it makes more sense
	for this setting to remain here. */
	//BOOL	bUsingDefaultColumns;

private:

	DISALLOW_COPY_AND_ASSIGN(Tab);

	const int m_id;

	std::unique_ptr<NavigationController> m_navigationController;
	CShellBrowser *m_shellBrowser;

	HWND m_listView;

	bool m_useCustomName;
	std::wstring m_customName;
	bool m_locked;
	bool m_addressLocked;

	TabUpdatedSignal m_tabUpdatedSignal;
};