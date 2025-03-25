// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserEvents.h"

boost::signals2::connection ShellBrowserEvents::AddItemsChangedObserver(
	const Signal::slot_type &observer, const ShellBrowserEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_itemsChangedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection ShellBrowserEvents::AddDirectoryPropertiesChangedObserver(
	const Signal::slot_type &observer, const ShellBrowserEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_directoryPropertiesChangedSignal.connect(MakeFilteredObserver(observer, scope),
		position);
}

boost::signals2::connection ShellBrowserEvents::AddSelectionChangedObserver(
	const Signal::slot_type &observer, const ShellBrowserEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_selectionChangedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

void ShellBrowserEvents::NotifyItemsChanged(const ShellBrowser *shellBrowser)
{
	m_itemsChangedSignal(shellBrowser);
}

void ShellBrowserEvents::NotifyDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	m_directoryPropertiesChangedSignal(shellBrowser);
}

void ShellBrowserEvents::NotifySelectionChanged(const ShellBrowser *shellBrowser)
{
	m_selectionChangedSignal(shellBrowser);
}
