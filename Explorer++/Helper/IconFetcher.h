// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellHelper.h"
#include "WindowSubclassWrapper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <ShlObj.h>
#include <functional>
#include <future>
#include <optional>
#include <unordered_map>

class CachedIcons;

class IconFetcherInterface
{
public:
	using Callback = std::function<void(int iconIndex)>;

	virtual void QueueIconTask(std::wstring_view path, Callback callback) = 0;
	virtual void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) = 0;
	virtual void ClearQueue() = 0;
};

class IconFetcher : public IconFetcherInterface
{
public:
	IconFetcher(HWND hwnd, CachedIcons *cachedIcons);
	virtual ~IconFetcher();

	void QueueIconTask(std::wstring_view path, Callback callback) override;
	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) override;
	void ClearQueue() override;

private:
	static const UINT_PTR SUBCLASS_ID = 0;

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

	static LRESULT CALLBACK WindowSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK WindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static std::optional<int> FindIconAsync(PCIDLIST_ABSOLUTE pidl);
	void ProcessIconResult(int iconResultId);

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;

	ctpl::thread_pool m_iconThreadPool;
	std::unordered_map<int, FutureResult> m_iconResults;
	int m_iconResultIDCounter;
	CachedIcons *m_cachedIcons;
	std::function<void(int data)> m_callback;
};