// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AsyncIconFetcher.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/core/noncopyable.hpp>
#include <variant>

class CachedIcons;
class Runtime;

class AsyncIconFetcherImpl : public AsyncIconFetcher, private boost::noncopyable
{
public:
	AsyncIconFetcherImpl(const Runtime *runtime, CachedIcons *cachedIcons);

	// AsyncIconFetcher
	concurrencpp::lazy_result<std::optional<ShellIconInfo>> GetIconIndexAsync(
		PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken) override;
	concurrencpp::lazy_result<std::optional<ShellIconInfo>> GetIconIndexAsync(
		const std::wstring &path, std::stop_token stopToken) override;
	int GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const override;
	std::optional<int> MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const override;
	std::optional<int> MaybeGetCachedIconIndex(const std::wstring &path) const override;
	int GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const override;
	int GetDefaultIconIndex(DefaultIconType defaultIconType) const override;

private:
	using IconTarget = std::variant<PidlAbsolute, std::wstring>;

	static concurrencpp::lazy_result<std::optional<ShellIconInfo>> GetIconIndexAsyncImpl(
		WeakPtr<AsyncIconFetcherImpl> weakSelf, IconTarget target, std::stop_token stopToken);
	static PidlAbsolute MaybeResolveTargetToPidl(const IconTarget &target);

	const Runtime *const m_runtime;
	CachedIcons *const m_cachedIcons;
	int m_defaultFileIconIndex;
	int m_defaultFolderIconIndex;

	WeakPtrFactory<AsyncIconFetcherImpl> m_weakPtrFactory{ this };
};
