// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewBase.h"
#include "../Helper/SignalWrapper.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <optional>
#include <vector>

struct RebarBandStorageData;
class RebarBandView;
class RebarViewDelegate;

class RebarView : public ViewBase, private boost::noncopyable
{
public:
	enum class OverflowMenuPolicy
	{
		Disabled,
		Enabled
	};

	RebarView(ViewBase *parent, DWORD style);
	~RebarView();

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

private:
	friend class RebarViewTestApi;

	static HWND CreateRebar(ViewBase *parent, DWORD style);

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

	RebarViewDelegate *m_delegate = nullptr;
	bool m_locked = false;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
