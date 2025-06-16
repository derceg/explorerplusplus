// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NoOpMenuHelpTextHost.h"

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
