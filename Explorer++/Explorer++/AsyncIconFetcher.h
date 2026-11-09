// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DefaultIconType.h"
#include "../Helper/ShellHelper.h"
#include <concurrencpp/concurrencpp.h>
#include <shtypes.h>
#include <optional>
#include <stop_token>

class AsyncIconFetcher
{
public:
	virtual ~AsyncIconFetcher() = default;

	[[nodiscard]] virtual concurrencpp::lazy_result<std::optional<ShellIconInfo>> GetIconIndexAsync(
		PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken) = 0;
	virtual int GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const = 0;
	virtual std::optional<int> MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const = 0;
	virtual int GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const = 0;
	virtual int GetDefaultIconIndex(DefaultIconType defaultIconType) const = 0;
};
