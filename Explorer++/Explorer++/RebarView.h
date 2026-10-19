// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SignalWrapper.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include <optional>
#include <vector>

struct RebarBandStorageData;
class RebarBandView;
class RebarViewDelegate;
class WindowSubclass;

class RebarView : private boost::noncopyable
{
public:
	enum class OverflowMenuPolicy
	{
		Disabled,
		Enabled
	};

	HWND GetHWND() const;
	void SetDelegate(RebarViewDelegate *delegate);
	UINT GetHeight() const;

	void AddBand(UINT id, RebarBandView *bandView, OverflowMenuPolicy overflowMenuPolicy);
	void ShowBand(UINT id, bool show);
	void SetBandOnNewLine(UINT id, bool newLine);
	void SetBandWidth(UINT id, UINT width);
	void LockBands(bool lock);

	std::vector<RebarBandStorageData> GetStorageData() const;

	// Signals
	SignalWrapper<RebarView, void()> heightChangedSignal;
	SignalWrapper<RebarView, void()> windowDestroyedSignal;

protected:
	RebarView(HWND parent, DWORD style);
	virtual ~RebarView();

private:
	friend class RebarViewTestApi;

	static HWND CreateRebar(HWND parent, DWORD style);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnShowContextMenu(HWND source, const POINT &ptScreen);

	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnChevronPushed(const NMREBARCHEVRON *chevronData);

	void OnBandSizeRequirementsChanged(UINT id, RebarBandView *bandView);

	std::optional<UINT> MaybeGetBandIdAtPoint(const POINT &ptScreen) const;
	std::optional<UINT> MaybeGetBandIdForWindow(HWND hwnd) const;
	int GetNumBands() const;
	UINT GetBandStyles(UINT id) const;
	void SetBandStyles(UINT id, UINT styles);
	REBARBANDINFO GetBandInfo(UINT id, UINT mask) const;
	void SetBandInfo(UINT id, const REBARBANDINFO &bandInfo);
	int GetBandIndex(UINT id) const;
	UINT GetBandId(int index) const;

	void OnNcDestroy();

	const HWND m_hwnd;
	RebarViewDelegate *m_delegate = nullptr;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	bool m_locked = false;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
