// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NoOpMenuHelpTextHost.h"

void NoOpMenuHelpTextHost::MenuItemSelected(HMENU menu, UINT itemId, UINT flags)
{
	UNREFERENCED_PARAMETER(menu);
	UNREFERENCED_PARAMETER(itemId);
	UNREFERENCED_PARAMETER(flags);
}

boost::signals2::connection NoOpMenuHelpTextHost::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	UNREFERENCED_PARAMETER(observer);

	return {};
}

NoOpMenuHelpTextHost *NoOpMenuHelpTextHost::GetInstance()
{
	static NoOpMenuHelpTextHost helpTextHost;
	return &helpTextHost;
}
