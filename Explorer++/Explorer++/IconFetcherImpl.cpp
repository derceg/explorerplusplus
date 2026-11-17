// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconFetcherImpl.h"
#include "RuntimeHelper.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Pidl.h"
#include "../Helper/ShellHelper.h"

IconFetcherImpl::IconFetcherImpl(const Runtime *runtime, CachedIcons *cachedIcons) :
	m_runtime(runtime),
	m_cachedIcons(cachedIcons)
{
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_defaultFileIconIndex));
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconIndex));
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> IconFetcherImpl::GetIconIndexAsync(
	PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken)
{
	return GetIconIndexAsyncImpl(m_weakPtrFactory.GetWeakPtr(), pidl, stopToken);
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> IconFetcherImpl::GetIconIndexAsync(
	const std::wstring &path, std::stop_token stopToken)
{
	return GetIconIndexAsyncImpl(m_weakPtrFactory.GetWeakPtr(), path, stopToken);
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> IconFetcherImpl::GetIconIndexAsyncImpl(
	WeakPtr<IconFetcherImpl> weakSelf, IconTarget target, std::stop_token stopToken)
{
	auto *runtime = weakSelf->m_runtime;

	co_await ResumeOnComStaThread(runtime);

	if (stopToken.stop_requested())
	{
		co_return std::nullopt;
	}

	auto pidl = MaybeResolveTargetToPidl(target);

	if (!pidl.HasValue())
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

PidlAbsolute IconFetcherImpl::MaybeResolveTargetToPidl(const IconTarget &target)
{
	if (const auto *pidl = std::get_if<PidlAbsolute>(&target))
	{
		return *pidl;
	}

	// SHGetFileInfo will fail for non-filesystem paths that are passed in as strings. For example,
	// attempting to retrieve the icon for the recycle bin will fail if you pass the parsing path
	// (i.e. ::{645FF040-5081-101B-9F08-00AA002F954E}). If, however, you pass the pidl, the function
	// will succeed. Therefore, the path will always be converted to a pidl first here.
	PidlAbsolute pidl;
	HRESULT hr = SHParseDisplayName(std::get<std::wstring>(target).c_str(), nullptr,
		PidlOutParam(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		return {};
	}

	return pidl;
}

int IconFetcherImpl::GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const
{
	auto cachedIconIndex = MaybeGetCachedIconIndex(pidl);

	if (cachedIconIndex)
	{
		return *cachedIconIndex;
	}

	return GetDefaultIconIndex(pidl);
}

std::optional<int> IconFetcherImpl::MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	std::wstring path;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, path);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return MaybeGetCachedIconIndex(path);
}

std::optional<int> IconFetcherImpl::MaybeGetCachedIconIndex(const std::wstring &path) const
{
	return m_cachedIcons->MaybeGetIconIndex(path);
}

int IconFetcherImpl::GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	SFGAOF attributes = SFGAO_FOLDER;
	HRESULT hr = GetItemAttributes(pidl, &attributes);

	if (SUCCEEDED(hr) && WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		return m_defaultFolderIconIndex;
	}

	return m_defaultFileIconIndex;
}

int IconFetcherImpl::GetDefaultIconIndex(DefaultIconType defaultIconType) const
{
	return defaultIconType == DefaultIconType::File ? m_defaultFileIconIndex
													: m_defaultFolderIconIndex;
}
