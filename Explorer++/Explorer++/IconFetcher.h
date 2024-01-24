// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <ShlObj.h>
#include <functional>
#include <optional>

enum class DefaultIconType
{
	File,
	Folder
};

class IconFetcher
{
public:
	using Callback = std::function<void(int iconIndex)>;

	virtual ~IconFetcher() = default;

	virtual void QueueIconTask(std::wstring_view path, Callback callback) = 0;
	virtual void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) = 0;
	virtual void ClearQueue() = 0;
	virtual int GetCachedIconIndexOrDefault(const std::wstring &itemPath,
		DefaultIconType defaultIconType) const = 0;
	virtual std::optional<int> GetCachedIconIndex(const std::wstring &itemPath) const = 0;
};
