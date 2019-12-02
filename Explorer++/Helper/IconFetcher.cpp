// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconFetcher.h"
#include "CachedIcons.h"

IconFetcher::IconFetcher(HWND hwnd, CachedIcons *cachedIcons) :
	m_hwnd(hwnd),
	m_cachedIcons(cachedIcons),
	m_iconThreadPool(1),
	m_iconResultIDCounter(0)
{
	m_windowSubclasses.push_back(WindowSubclassWrapper(hwnd, WindowSubclassStub,
		SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	m_iconThreadPool.push([] (int id) {
		UNREFERENCED_PARAMETER(id);

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	});
}

IconFetcher::~IconFetcher()
{
	m_iconThreadPool.clear_queue();

	m_iconThreadPool.push([] (int id) {
		UNREFERENCED_PARAMETER(id);

		CoUninitialize();
	});
}

LRESULT CALLBACK IconFetcher::WindowSubclassStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	IconFetcher *iconFetcher = reinterpret_cast<IconFetcher *>(dwRefData);

	return iconFetcher->WindowSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK IconFetcher::WindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void IconFetcher::QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback)
{
	int iconResultID = m_iconResultIDCounter++;

	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl.reset(ILCloneFull(pidl));

	auto iconResult = m_iconThreadPool.push([this, iconResultID, basicItemInfo] (int id) {
		UNREFERENCED_PARAMETER(id);

		return FindIconAsync(m_hwnd, iconResultID, basicItemInfo.pidl.get());
	});

	FutureResult futureResult;
	futureResult.callback = callback;
	futureResult.pidl.reset(ILCloneFull(pidl));
	futureResult.iconResult = std::move(iconResult);
	m_iconResults.insert({ iconResultID, std::move(futureResult) });
}

std::optional<IconFetcher::IconResult> IconFetcher::FindIconAsync(HWND hwnd, int iconResultId,
	PCIDLIST_ABSOLUTE pidl)
{
	// Must use SHGFI_ICON here, rather than SHGFO_SYSICONINDEX, or else 
	// icon overlays won't be applied.
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidl), 0, &shfi,
		sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		return std::nullopt;
	}

	DestroyIcon(shfi.hIcon);

	IconResult result;
	result.iconIndex = shfi.iIcon;

	TCHAR filePath[MAX_PATH];
	HRESULT hr = GetDisplayName(pidl, filePath, static_cast<UINT>(std::size(filePath)),
		SHGDN_FORPARSING);

	if (SUCCEEDED(hr))
	{
		result.path = filePath;
	}

	PostMessage(hwnd, WM_APP_ICON_RESULT_READY, iconResultId, 0);

	return result;
}

void IconFetcher::ProcessIconResult(int iconResultId)
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
		m_cachedIcons->addOrUpdateFileIcon(result->path, result->iconIndex);
	}

	futureResult.callback(futureResult.pidl.get(), result->iconIndex);
}

void IconFetcher::ClearQueue()
{
	m_iconThreadPool.clear_queue();
	m_iconResults.clear();
}