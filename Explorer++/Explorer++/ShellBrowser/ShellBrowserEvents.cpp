// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserEvents.h"

boost::signals2::connection ShellBrowserEvents::AddDirectoryContentsChangedObserver(
	const Signal::slot_type &observer, const ShellBrowserEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_directoryContentsChangedSignal.connect(MakeFilteredObserver(observer, scope),
		position);
}

void ShellBrowserEvents::NotifyDirectoryContentsChanged(const ShellBrowser *shellBrowser)
{
	m_directoryContentsChangedSignal(shellBrowser);
}
