// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconFetcher.h"

class IconFetcherFake : public IconFetcher
{
public:
	void QueueIconTask(std::wstring_view path, Callback callback) override;
	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) override;
	void ClearQueue() override;
	int GetCachedIconIndexOrDefault(const std::wstring &itemPath,
		DefaultIconType defaultIconType) const override;
	std::optional<int> GetCachedIconIndex(const std::wstring &itemPath) const override;
};
