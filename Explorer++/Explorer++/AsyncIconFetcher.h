// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <boost/core/noncopyable.hpp>
#include <concurrencpp/concurrencpp.h>
#include <shtypes.h>
#include <memory>
#include <stop_token>

class CachedIcons;
class Runtime;

class AsyncIconFetcher : private boost::noncopyable
{
public:
	AsyncIconFetcher(const Runtime *runtime, std::shared_ptr<CachedIcons> cachedIcons);

	[[nodiscard]] concurrencpp::lazy_result<std::optional<ShellIconInfo>> GetIconIndexAsync(
		PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken);
	int GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const;
	std::optional<int> MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const;
	int GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const;

private:
	const Runtime *const m_runtime;
	const std::shared_ptr<CachedIcons> m_cachedIcons;
	int m_defaultFileIconIndex;
	int m_defaultFolderIconIndex;
};
