// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellHelper.h"
#include "WindowSubclassWrapper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <future>
#include <functional>
#include <optional>
#include <unordered_map>

class CachedIcons;

class IconFetcher
{
public:

	using Callback = std::function<void(PCIDLIST_ABSOLUTE pidl, int iconIndex)>;

	IconFetcher(HWND hwnd, CachedIcons *cachedIcons);
	~IconFetcher();

	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback);
	void ClearQueue();

private:

	static const UINT_PTR SUBCLASS_ID = 0;

	static const UINT WM_APP_ICON_RESULT_READY = WM_APP + 200;

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
		unique_pidl_absolute pidl;
		std::future<std::optional<IconResult>> iconResult;
	};

	static LRESULT CALLBACK WindowSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK WindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static std::optional<IconResult> FindIconAsync(HWND hwnd, int iconResultId, PCIDLIST_ABSOLUTE pidl);
	void ProcessIconResult(int iconResultId);

	const HWND m_hwnd;
	std::vector<WindowSubclassWrapper> m_windowSubclasses;

	ctpl::thread_pool m_iconThreadPool;
	std::unordered_map<int, FutureResult> m_iconResults;
	int m_iconResultIDCounter;
	CachedIcons *m_cachedIcons;
	std::function<void(int data)> m_callback;
};