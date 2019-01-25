// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PIDLWrapper.h"
#include "../Helper/ShellHelper.h"

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
		pidlComplete.reset(ILClone(other.pidlComplete.get()));
		pridl.reset(ILClone(other.pridl.get()));
		wfd = other.wfd;
		StringCchCopy(szDisplayName, SIZEOF_ARRAY(szDisplayName), other.szDisplayName);
	}

	PIDLPointer		pidlComplete;
	PIDLPointer		pridl;
	WIN32_FIND_DATA	wfd;
	TCHAR			szDisplayName[MAX_PATH];

	std::wstring getFullPath() const
	{
		TCHAR fullPath[MAX_PATH];
		GetDisplayName(pidlComplete.get(), fullPath, SIZEOF_ARRAY(fullPath), SHGDN_FORPARSING);
		return fullPath;
	}
};