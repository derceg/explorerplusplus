// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/PreservedShellBrowser.h"
#include "Tab.h"
#include <boost/core/noncopyable.hpp>

struct PreservedTab : private boost::noncopyable
{
	PreservedTab(const Tab &tab, int index);

	const int id;
	const int browserId;
	const int index;

	const bool useCustomName;
	const std::wstring customName;
	const Tab::LockState lockState;

	const PreservedShellBrowser preservedShellBrowser;
};
