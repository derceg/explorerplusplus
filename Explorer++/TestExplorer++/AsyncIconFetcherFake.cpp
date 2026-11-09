// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AsyncIconFetcherFake.h"
#include <wil/result.h>

AsyncIconFetcherFake::AsyncIconFetcherFake()
{
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_defaultFileIconIndex));
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> AsyncIconFetcherFake::GetIconIndexAsync(
	PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken)
{
	UNREFERENCED_PARAMETER(pidl);

	PendingRequest request(stopToken);
	auto result = request.promise.get_result();
	m_pendingRequests.push_back(std::move(request));
	co_return co_await result;
}

int AsyncIconFetcherFake::GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const
{
	UNREFERENCED_PARAMETER(pidl);

	return m_defaultFileIconIndex;
}

std::optional<int> AsyncIconFetcherFake::MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	UNREFERENCED_PARAMETER(pidl);

	return std::nullopt;
}

int AsyncIconFetcherFake::GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	UNREFERENCED_PARAMETER(pidl);

	return m_defaultFileIconIndex;
}

int AsyncIconFetcherFake::GetDefaultIconIndex(DefaultIconType defaultIconType) const
{
	UNREFERENCED_PARAMETER(defaultIconType);

	return m_defaultFileIconIndex;
}

bool AsyncIconFetcherFake::HasPendingRequests() const
{
	return !m_pendingRequests.empty();
}

void AsyncIconFetcherFake::CompletePendingRequests()
{
	while (!m_pendingRequests.empty())
	{
		PendingRequest request = std::move(m_pendingRequests.front());
		m_pendingRequests.pop_front();

		std::optional<ShellIconInfo> result;

		if (!request.stopToken.stop_requested())
		{
			result = { m_defaultFileIconIndex, 0 };
		}

		request.promise.set_result(result);
	}
}
