// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "IconFetcherFake.h"

IconFetcherFake::IconFetcherFake()
{
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_defaultFileIconIndex));
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> IconFetcherFake::GetIconIndexAsync(
	PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken)
{
	UNREFERENCED_PARAMETER(pidl);

	return GetIconIndexAsyncImpl(stopToken);
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> IconFetcherFake::GetIconIndexAsync(
	const std::wstring &path, std::stop_token stopToken)
{
	UNREFERENCED_PARAMETER(path);

	return GetIconIndexAsyncImpl(stopToken);
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> IconFetcherFake::GetIconIndexAsyncImpl(
	std::stop_token stopToken)
{
	PendingRequest request(stopToken);
	auto result = request.promise.get_result();
	m_pendingRequests.push_back(std::move(request));
	co_return co_await result;
}

int IconFetcherFake::GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const
{
	UNREFERENCED_PARAMETER(pidl);

	return m_defaultFileIconIndex;
}

std::optional<int> IconFetcherFake::MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	UNREFERENCED_PARAMETER(pidl);

	return std::nullopt;
}

std::optional<int> IconFetcherFake::MaybeGetCachedIconIndex(const std::wstring &path) const
{
	UNREFERENCED_PARAMETER(path);

	return std::nullopt;
}

int IconFetcherFake::GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	UNREFERENCED_PARAMETER(pidl);

	return m_defaultFileIconIndex;
}

int IconFetcherFake::GetDefaultIconIndex(DefaultIconType defaultIconType) const
{
	UNREFERENCED_PARAMETER(defaultIconType);

	return m_defaultFileIconIndex;
}

bool IconFetcherFake::HasPendingRequests() const
{
	return !m_pendingRequests.empty();
}

void IconFetcherFake::CompletePendingRequests()
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
