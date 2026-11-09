// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AsyncIconFetcherImpl.h"
#include "RuntimeHelper.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Pidl.h"
#include "../Helper/ShellHelper.h"

AsyncIconFetcherImpl::AsyncIconFetcherImpl(const Runtime *runtime, CachedIcons *cachedIcons) :
	m_runtime(runtime),
	m_cachedIcons(cachedIcons)
{
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_defaultFileIconIndex));
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconIndex));
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> AsyncIconFetcherImpl::GetIconIndexAsync(
	PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken)
{
	return GetIconIndexAsyncImpl(m_weakPtrFactory.GetWeakPtr(), pidl, stopToken);
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> AsyncIconFetcherImpl::GetIconIndexAsyncImpl(
	WeakPtr<AsyncIconFetcherImpl> weakSelf, PidlAbsolute pidl, std::stop_token stopToken)
{
	auto *runtime = weakSelf->m_runtime;

	co_await ResumeOnComStaThread(runtime);

	if (stopToken.stop_requested())
	{
		co_return std::nullopt;
	}

	PidlAbsolute updatedPidl;
	HRESULT hr = UpdatePidl(pidl.Raw(), updatedPidl);

	PCIDLIST_ABSOLUTE finalPidl;

	if (SUCCEEDED(hr))
	{
		finalPidl = updatedPidl.Raw();
	}
	else
	{
		finalPidl = pidl.Raw();
	}

	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(finalPidl), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		co_return std::nullopt;
	}

	DestroyIcon(shfi.hIcon);

	co_await ResumeOnUiThread(runtime);

	if (stopToken.stop_requested() || !weakSelf)
	{
		co_return std::nullopt;
	}

	std::wstring itemPath;
	hr = GetDisplayName(finalPidl, SHGDN_FORPARSING, itemPath);

	auto iconInfo = ExtractShellIconParts(shfi.iIcon);

	if (SUCCEEDED(hr))
	{
		weakSelf->m_cachedIcons->AddOrUpdateIcon(itemPath, iconInfo.iconIndex);
	}

	co_return iconInfo;
}

int AsyncIconFetcherImpl::GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const
{
	auto cachedIconIndex = MaybeGetCachedIconIndex(pidl);

	if (cachedIconIndex)
	{
		return *cachedIconIndex;
	}

	return GetDefaultIconIndex(pidl);
}

std::optional<int> AsyncIconFetcherImpl::MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	std::wstring itemPath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, itemPath);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return m_cachedIcons->MaybeGetIconIndex(itemPath);
}

int AsyncIconFetcherImpl::GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	SFGAOF attributes = SFGAO_FOLDER;
	HRESULT hr = GetItemAttributes(pidl, &attributes);

	if (SUCCEEDED(hr) && WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		return m_defaultFolderIconIndex;
	}

	return m_defaultFileIconIndex;
}

int AsyncIconFetcherImpl::GetDefaultIconIndex(DefaultIconType defaultIconType) const
{
	return defaultIconType == DefaultIconType::File ? m_defaultFileIconIndex
													: m_defaultFolderIconIndex;
}
