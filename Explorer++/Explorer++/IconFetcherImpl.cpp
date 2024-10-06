// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconFetcherImpl.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/WindowSubclassWrapper.h"

IconFetcherImpl::IconFetcherImpl(HWND hwnd, CachedIcons *cachedIcons) :
	m_hwnd(hwnd),
	m_cachedIcons(cachedIcons),
	m_iconThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_iconResultIDCounter(0)
{
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_defaultFileIconIndex));
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconIndex));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hwnd,
		std::bind_front(&IconFetcherImpl::WindowSubclass, this)));
}

IconFetcherImpl::~IconFetcherImpl()
{
	m_iconThreadPool.clear_queue();
}

LRESULT IconFetcherImpl::WindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_APP_ICON_RESULT_READY:
		ProcessIconResult(static_cast<int>(wParam));
		return 0;
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void IconFetcherImpl::QueueIconTask(std::wstring_view path, Callback callback)
{
	int iconResultID = m_iconResultIDCounter++;

	auto iconResult = m_iconThreadPool.push(
		[this, iconResultID, copiedPath = std::wstring(path)](int id) -> std::optional<IconResult>
		{
			UNREFERENCED_PARAMETER(id);

			// SHGetFileInfo will fail for non-filesystem paths that are passed in
			// as strings. For example, attempting to retrieve the icon for the
			// recycle bin will fail if you pass the parsing path (i.e.
			// ::{645FF040-5081-101B-9F08-00AA002F954E}). If, however, you pass the
			// pidl, the function will succeed. Therefore, paths will always be
			// converted to pidls first here.
			unique_pidl_absolute pidl;
			HRESULT hr =
				SHParseDisplayName(copiedPath.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

			if (FAILED(hr))
			{
				return std::nullopt;
			}

			auto iconIndex = FindIconAsync(pidl.get());

			if (!iconIndex)
			{
				return std::nullopt;
			}

			IconResult result;
			result.iconIndex = *iconIndex;
			result.path = copiedPath;

			PostMessage(m_hwnd, WM_APP_ICON_RESULT_READY, iconResultID, 0);

			return result;
		});

	FutureResult futureResult;
	futureResult.callback = callback;
	futureResult.iconResult = std::move(iconResult);
	m_iconResults.insert({ iconResultID, std::move(futureResult) });
}

void IconFetcherImpl::QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback)
{
	int iconResultID = m_iconResultIDCounter++;

	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl.reset(ILCloneFull(pidl));

	auto iconResult = m_iconThreadPool.push(
		[this, iconResultID, basicItemInfo](int id) -> std::optional<IconResult>
		{
			UNREFERENCED_PARAMETER(id);

			// It's important that pidl is updated. Otherwise, the icon that's retrieved may be the
			// original icon.
			PidlAbsolute updatedPidl;
			HRESULT hr = UpdatePidl(basicItemInfo.pidl.get(), updatedPidl);

			PCIDLIST_ABSOLUTE finalPidl;

			if (SUCCEEDED(hr))
			{
				finalPidl = updatedPidl.Raw();
			}
			else
			{
				finalPidl = basicItemInfo.pidl.get();
			}

			auto iconIndex = FindIconAsync(finalPidl);

			if (!iconIndex)
			{
				return std::nullopt;
			}

			IconResult result;
			result.iconIndex = *iconIndex;

			std::wstring filePath;
			hr = GetDisplayName(finalPidl, SHGDN_FORPARSING, filePath);

			if (SUCCEEDED(hr))
			{
				result.path = filePath;
			}

			PostMessage(m_hwnd, WM_APP_ICON_RESULT_READY, iconResultID, 0);

			return result;
		});

	FutureResult futureResult;
	futureResult.callback = callback;
	futureResult.iconResult = std::move(iconResult);
	m_iconResults.insert({ iconResultID, std::move(futureResult) });
}

std::optional<int> IconFetcherImpl::FindIconAsync(PCIDLIST_ABSOLUTE pidl)
{
	// Must use SHGFI_ICON here, rather than SHGFO_SYSICONINDEX, or else
	// icon overlays won't be applied.
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidl), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		return std::nullopt;
	}

	DestroyIcon(shfi.hIcon);

	return shfi.iIcon;
}

void IconFetcherImpl::ProcessIconResult(int iconResultId)
{
	auto itr = m_iconResults.find(iconResultId);

	if (itr == m_iconResults.end())
	{
		return;
	}

	auto &futureResult = itr->second;
	auto result = futureResult.iconResult.get();

	if (!result)
	{
		// Icon lookup failed.
		return;
	}

	if (!result->path.empty())
	{
		m_cachedIcons->AddOrUpdateIcon(result->path, result->iconIndex);
	}

	futureResult.callback(result->iconIndex);
}

void IconFetcherImpl::ClearQueue()
{
	m_iconThreadPool.clear_queue();
	m_iconResults.clear();
}

int IconFetcherImpl::GetCachedIconIndexOrDefault(const std::wstring &itemPath,
	DefaultIconType defaultIconType) const
{
	auto cachedIconIndex = GetCachedIconIndex(itemPath);

	if (cachedIconIndex)
	{
		return *cachedIconIndex;
	}

	if (defaultIconType == DefaultIconType::File)
	{
		return m_defaultFileIconIndex;
	}
	else
	{
		return m_defaultFolderIconIndex;
	}
}

std::optional<int> IconFetcherImpl::GetCachedIconIndex(const std::wstring &itemPath) const
{
	return m_cachedIcons->MaybeGetIconIndex(itemPath);
}
