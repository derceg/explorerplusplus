// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AsyncIconFetcher.h"
#include "RuntimeHelper.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/ShellHelper.h"

AsyncIconFetcher::AsyncIconFetcher(const Runtime *runtime,
	std::shared_ptr<CachedIcons> cachedIcons) :
	m_runtime(runtime),
	m_cachedIcons(cachedIcons)
{
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_defaultFileIconIndex));
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconIndex));
}

concurrencpp::lazy_result<std::optional<ShellIconInfo>> AsyncIconFetcher::GetIconIndexAsync(
	PCIDLIST_ABSOLUTE pidl, std::stop_token stopToken)
{
	co_await ResumeOnComStaThread(m_runtime);

	if (stopToken.stop_requested())
	{
		co_return std::nullopt;
	}

	PidlAbsolute updatedPidl;
	HRESULT hr = UpdatePidl(pidl, updatedPidl);

	PCIDLIST_ABSOLUTE finalPidl;

	if (SUCCEEDED(hr))
	{
		finalPidl = updatedPidl.Raw();
	}
	else
	{
		finalPidl = pidl;
	}

	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(finalPidl), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		co_return std::nullopt;
	}

	DestroyIcon(shfi.hIcon);

	co_await ResumeOnUiThread(m_runtime);

	std::wstring itemPath;
	hr = GetDisplayName(finalPidl, SHGDN_FORPARSING, itemPath);

	auto iconInfo = ExtractShellIconParts(shfi.iIcon);

	if (SUCCEEDED(hr))
	{
		m_cachedIcons->AddOrUpdateIcon(itemPath, iconInfo.iconIndex);
	}

	co_return iconInfo;
}

int AsyncIconFetcher::GetCachedIconIndexOrDefault(PCIDLIST_ABSOLUTE pidl) const
{
	auto cachedIconIndex = MaybeGetCachedIconIndex(pidl);

	if (cachedIconIndex)
	{
		return *cachedIconIndex;
	}

	return GetDefaultIconIndex(pidl);
}

std::optional<int> AsyncIconFetcher::MaybeGetCachedIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	std::wstring itemPath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, itemPath);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return m_cachedIcons->MaybeGetIconIndex(itemPath);
}

int AsyncIconFetcher::GetDefaultIconIndex(PCIDLIST_ABSOLUTE pidl) const
{
	SFGAOF attributes = SFGAO_FOLDER;
	HRESULT hr = GetItemAttributes(pidl, &attributes);

	if (SUCCEEDED(hr) && WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		return m_defaultFolderIconIndex;
	}

	return m_defaultFileIconIndex;
}
