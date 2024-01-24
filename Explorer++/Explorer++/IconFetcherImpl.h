// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconFetcher.h"
#include "../Helper/ShellHelper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <future>
#include <unordered_map>

class CachedIcons;
class WindowSubclassWrapper;

class IconFetcherImpl : public IconFetcher
{
public:
	IconFetcherImpl(HWND hwnd, CachedIcons *cachedIcons);
	~IconFetcherImpl();

	void QueueIconTask(std::wstring_view path, Callback callback) override;
	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) override;
	void ClearQueue() override;
	int GetCachedIconIndexOrDefault(const std::wstring &itemPath,
		DefaultIconType defaultIconType) const;
	std::optional<int> GetCachedIconIndex(const std::wstring &itemPath) const;

private:
	// This is the end of the range that starts at WM_APP. This class subclasses the window that's
	// passed to the constructor, so it's not possible to tell what other WM_APP messages are in
	// use. To try to avoid clashes with other messages sent throughout the application, the last
	// value in the range will be used.
	static const UINT WM_APP_ICON_RESULT_READY = 0xBFFF;

	struct BasicItemInfo
	{
		BasicItemInfo() = default;

		BasicItemInfo(const BasicItemInfo &other)
		{
			pidl.reset(ILCloneFull(other.pidl.get()));
		}

		unique_pidl_absolute pidl;
	};

	struct IconResult
	{
		int iconIndex;
		std::wstring path;
	};

	struct FutureResult
	{
		Callback callback;
		std::future<std::optional<IconResult>> iconResult;
	};

	LRESULT WindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static std::optional<int> FindIconAsync(PCIDLIST_ABSOLUTE pidl);
	void ProcessIconResult(int iconResultId);

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	int m_defaultFileIconIndex;
	int m_defaultFolderIconIndex;

	ctpl::thread_pool m_iconThreadPool;
	std::unordered_map<int, FutureResult> m_iconResults;
	int m_iconResultIDCounter;
	CachedIcons *m_cachedIcons;
	std::function<void(int data)> m_callback;
};
