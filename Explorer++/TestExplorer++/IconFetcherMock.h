// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconFetcher.h"
#include <gmock/gmock.h>

class IconFetcherMock : public IconFetcher
{
public:
	MOCK_METHOD(void, QueueIconTask, (std::wstring_view path, Callback callback), (override));
	MOCK_METHOD(void, QueueIconTask, (PCIDLIST_ABSOLUTE pidl, Callback callback), (override));
	MOCK_METHOD(void, ClearQueue, (), (override));
	MOCK_METHOD(int, GetCachedIconIndexOrDefault,
		(const std::wstring &itemPath, DefaultIconType defaultIconType), (const, override));
	MOCK_METHOD(std::optional<int>, GetCachedIconIndex, (const std::wstring &itemPath),
		(const, override));
};
