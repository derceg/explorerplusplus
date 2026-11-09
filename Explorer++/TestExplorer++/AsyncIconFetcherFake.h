// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AsyncIconFetcher.h"
#include <deque>

class AsyncIconFetcherFake : public AsyncIconFetcher
{
public:
	AsyncIconFetcherFake();

	// AsyncIconFetcher
	concurrencpp::lazy_result<std::optional<ShellIconInfo>> GetIconIndexAsync(
		PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken) override;
	int GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const override;
	std::optional<int> MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const override;
	int GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const override;
	int GetDefaultIconIndex(DefaultIconType defaultIconType) const override;

	bool HasPendingRequests() const;
	void CompletePendingRequests();

private:
	struct PendingRequest
	{
		std::stop_token stopToken;
		concurrencpp::result_promise<std::optional<ShellIconInfo>> promise;
	};

	std::deque<PendingRequest> m_pendingRequests;
	int m_defaultFileIconIndex;
};
