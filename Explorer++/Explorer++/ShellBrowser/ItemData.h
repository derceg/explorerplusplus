// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <wil/resource.h>

struct BasicItemInfo_t
{
	BasicItemInfo_t() = default;
	BasicItemInfo_t(BasicItemInfo_t &&) = default;

	// Being able to explicitly copy this structure is important. The
	// structure can be passed to a background thread and that thread
	// needs to be able to hold its own copy of the data. A shallow copy
	// wouldn't work, as the copies would share the same underlying
	// PIDLs.
	BasicItemInfo_t(const BasicItemInfo_t &other)
	{
		pidlComplete.reset(ILCloneFull(other.pidlComplete.get()));
		pridl.reset(ILCloneChild(other.pridl.get()));
		wfd = other.wfd;
		StringCchCopy(szDisplayName, SIZEOF_ARRAY(szDisplayName), other.szDisplayName);
		isRoot = other.isRoot;
	}

	unique_pidl_absolute pidlComplete;
	unique_pidl_child pridl;
	WIN32_FIND_DATA wfd;
	TCHAR szDisplayName[MAX_PATH];
	bool isRoot;

	std::wstring getFullPath() const
	{
		TCHAR fullPath[MAX_PATH];
		GetDisplayName(pidlComplete.get(), fullPath, SIZEOF_ARRAY(fullPath), SHGDN_FORPARSING);
		return fullPath;
	}
};