// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "IconFetcherFake.h"

void IconFetcherFake::QueueIconTask(std::wstring_view path, Callback callback)
{
	UNREFERENCED_PARAMETER(path);
	UNREFERENCED_PARAMETER(callback);
}

void IconFetcherFake::QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(callback);
}

void IconFetcherFake::ClearQueue()
{
}

int IconFetcherFake::GetCachedIconIndexOrDefault(const std::wstring &itemPath,
	DefaultIconType defaultIconType) const
{
	UNREFERENCED_PARAMETER(itemPath);
	UNREFERENCED_PARAMETER(defaultIconType);

	return 0;
}

std::optional<int> IconFetcherFake::GetCachedIconIndex(const std::wstring &itemPath) const
{
	UNREFERENCED_PARAMETER(itemPath);

	return std::nullopt;
}
