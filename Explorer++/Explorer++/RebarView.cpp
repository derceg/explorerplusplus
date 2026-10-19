// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RebarView.h"
#include "RebarBandStorageData.h"
#include "RebarBandView.h"
#include "RebarViewDelegate.h"
#include "../Helper/WindowSubclass.h"
#include <wil/common.h>

RebarView::RebarView(HWND parent, DWORD style) : m_hwnd(CreateRebar(parent, style))
{
	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&RebarView::WndProc, this)));
	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(parent, std::bind_front(&RebarView::ParentWndProc, this)));
}

RebarView::~RebarView() = default;

HWND RebarView::CreateRebar(HWND parent, DWORD style)
{
	HWND hwnd = CreateWindowEx(WS_EX_CONTROLPARENT, REBARCLASSNAME, L"", style, 0, 0, 0, 0, parent,
		nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);
	return hwnd;
}

HWND RebarView::GetHWND() const
{
	return m_hwnd;
}

void RebarView::SetDelegate(RebarViewDelegate *delegate)
{
	m_delegate = delegate;
}

LRESULT RebarView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CONTEXTMENU:
		OnShowContextMenu(reinterpret_cast<HWND>(wParam),
			{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		return 0;

	case WM_NCDESTROY:
		OnNcDestroy();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void RebarView::OnShowContextMenu(HWND source, const POINT &ptScreen)
{
	if (!m_delegate)
	{
		return;
	}

	POINT finalPoint = ptScreen;

	if (ptScreen.x == -1 && ptScreen.y == -1)
	{
		finalPoint = { 0, 0 };
		auto res = ClientToScreen(source, &finalPoint);
		CHECK(res);
	}

	auto id =
		(source == m_hwnd) ? MaybeGetBandIdAtPoint(finalPoint) : MaybeGetBandIdForWindow(source);

	if (!id)
	{
		return;
	}

	m_delegate->OnShowContextMenu(*id, finalPoint);
}

LRESULT RebarView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case RBN_CHEVRONPUSHED:
				OnChevronPushed(reinterpret_cast<NMREBARCHEVRON *>(lParam));
				return 0;

			case RBN_HEIGHTCHANGE:
				heightChangedSignal.m_signal();
				return 0;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void RebarView::OnChevronPushed(const NMREBARCHEVRON *chevronData)
{
	if (!m_delegate)
	{
		return;
	}

	POINT ptScreen;
	ptScreen.x = chevronData->rc.left;
	ptScreen.y = chevronData->rc.bottom;
	auto res = ClientToScreen(m_hwnd, &ptScreen);
	CHECK(res);

	m_delegate->OnShowOverflowMenu(chevronData->wID, ptScreen);
}

UINT RebarView::GetHeight() const
{
	return static_cast<UINT>(SendMessage(m_hwnd, RB_GETBARHEIGHT, 0, 0));
}

void RebarView::AddBand(UINT id, RebarBandView *bandView, OverflowMenuPolicy overflowMenuPolicy)
{
	CHECK(SendMessage(GetHWND(), RB_IDTOINDEX, id, 0) == -1);

	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE;
	bandInfo.fStyle = RBBS_BREAK;
	bandInfo.wID = id;
	bandInfo.hwndChild = bandView->GetHWND();
	bandInfo.cxMinChild = 0;
	bandInfo.cyMinChild = bandView->GetHeight();

	if (m_locked)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
	}

	if (overflowMenuPolicy == OverflowMenuPolicy::Enabled)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_USECHEVRON);
	}

	if (auto idealWidth = bandView->MaybeGetIdealWidth())
	{
		WI_SetFlag(bandInfo.fMask, RBBIM_IDEALSIZE);
		bandInfo.cxIdeal = *idealWidth;
	}

	auto res = SendMessage(m_hwnd, RB_INSERTBAND, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(&bandInfo));
	CHECK(res);

	m_connections.push_back(bandView->AddSizeRequirementsChangedObserver(
		std::bind_front(&RebarView::OnBandSizeRequirementsChanged, this, id, bandView)));
}

// Updating the band size when the required size of the underlying view changes is important for two
// reasons:
//
// 1. If the minimum height has changed, the band height needs to be updated to ensure the view
//    continues to display correctly.
//
// 2. If the ideal width has changed, updating the band with that information ensures that the
//    overflow button will be shown at the correct point (i.e. when at least one button is hidden).
//    It also ensures that when the band is unlocked and the gripper is clicked, the band will
//    update its width to match the content of the view, rather than the final size being too small
//    or too large.
void RebarView::OnBandSizeRequirementsChanged(UINT id, RebarBandView *bandView)
{
	auto bandInfo = GetBandInfo(id, RBBIM_CHILDSIZE);

	if (auto idealWidth = bandView->MaybeGetIdealWidth())
	{
		WI_SetFlag(bandInfo.fMask, RBBIM_IDEALSIZE);
		bandInfo.cxIdeal = *idealWidth;
	}

	bandInfo.cyMinChild = bandView->GetHeight();
	SetBandInfo(id, bandInfo);
}

void RebarView::ShowBand(UINT id, bool show)
{
	auto res = SendMessage(m_hwnd, RB_SHOWBAND, GetBandIndex(id), show);
	CHECK(res);
}

void RebarView::SetBandOnNewLine(UINT id, bool newLine)
{
	UINT styles = GetBandStyles(id);

	if (newLine)
	{
		WI_SetFlag(styles, RBBS_BREAK);
	}
	else
	{
		WI_ClearFlag(styles, RBBS_BREAK);
	}

	SetBandStyles(id, styles);
}

void RebarView::SetBandWidth(UINT id, UINT width)
{
	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_SIZE;
	bandInfo.cx = width;
	SetBandInfo(id, bandInfo);
}

void RebarView::LockBands(bool lock)
{
	if (lock == m_locked)
	{
		return;
	}

	int numBands = GetNumBands();

	for (int i = 0; i < numBands; i++)
	{
		UINT id = GetBandId(i);
		auto bandInfo = GetBandInfo(id, RBBIM_STYLE);

		// Without this, the control won't correctly update once the gripper has been added or
		// removed. That is, the control won't add or remove the space for the gripper. By toggling
		// the RBBS_GRIPPERALWAYS style, adding or removing the gripper will work as expected. WTL
		// also does this when toggling the gripper - see
		// https://sourceforge.net/p/wtl/git/ci/faa1f28fb7fe9277532ed563101b489655b40131/tree/Include/atlctrls.h#l8604.
		WI_SetFlag(bandInfo.fStyle, RBBS_GRIPPERALWAYS);
		SetBandInfo(id, bandInfo);
		WI_ClearFlag(bandInfo.fStyle, RBBS_GRIPPERALWAYS);

		if (lock)
		{
			WI_SetFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
		}
		else
		{
			WI_ClearFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
		}

		SetBandInfo(id, bandInfo);
	}

	m_locked = lock;
}

std::optional<UINT> RebarView::MaybeGetBandIdAtPoint(const POINT &ptScreen) const
{
	RBHITTESTINFO hitTest = {};
	hitTest.pt = ptScreen;
	auto res = ScreenToClient(m_hwnd, &hitTest.pt);
	CHECK(res);

	int index =
		static_cast<int>(SendMessage(m_hwnd, RB_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTest)));

	if (index == -1)
	{
		return std::nullopt;
	}

	return GetBandId(index);
}

std::optional<UINT> RebarView::MaybeGetBandIdForWindow(HWND hwnd) const
{
	int numBands = GetNumBands();

	for (int i = 0; i < numBands; i++)
	{
		UINT id = GetBandId(i);
		auto bandInfo = GetBandInfo(id, RBBIM_CHILD);

		if (bandInfo.hwndChild == hwnd || IsChild(bandInfo.hwndChild, hwnd))
		{
			return id;
		}
	}

	return std::nullopt;
}

std::vector<RebarBandStorageData> RebarView::GetStorageData() const
{
	std::vector<RebarBandStorageData> rebarStorageInfo;
	int numBands = GetNumBands();

	for (int i = 0; i < numBands; i++)
	{
		UINT id = GetBandId(i);
		auto bandInfo = GetBandInfo(id, RBBIM_SIZE | RBBIM_STYLE);

		RebarBandStorageData bandStorageInfo;
		bandStorageInfo.id = id;
		bandStorageInfo.style = bandInfo.fStyle & RBBS_BREAK;
		bandStorageInfo.length = bandInfo.cx;
		rebarStorageInfo.push_back(bandStorageInfo);
	}

	return rebarStorageInfo;
}

int RebarView::GetNumBands() const
{
	return static_cast<int>(SendMessage(m_hwnd, RB_GETBANDCOUNT, 0, 0));
}

UINT RebarView::GetBandStyles(UINT id) const
{
	auto bandInfo = GetBandInfo(id, RBBIM_STYLE);
	return bandInfo.fStyle;
}

void RebarView::SetBandStyles(UINT id, UINT styles)
{
	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_STYLE;
	bandInfo.fStyle = styles;
	SetBandInfo(id, bandInfo);
}

REBARBANDINFO RebarView::GetBandInfo(UINT id, UINT mask) const
{
	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = mask;
	auto res =
		SendMessage(m_hwnd, RB_GETBANDINFO, GetBandIndex(id), reinterpret_cast<LPARAM>(&bandInfo));
	CHECK(res);

	return bandInfo;
}

void RebarView::SetBandInfo(UINT id, const REBARBANDINFO &bandInfo)
{
	auto res =
		SendMessage(m_hwnd, RB_SETBANDINFO, GetBandIndex(id), reinterpret_cast<LPARAM>(&bandInfo));
	CHECK(res);
}

int RebarView::GetBandIndex(UINT id) const
{
	auto index = static_cast<int>(SendMessage(m_hwnd, RB_IDTOINDEX, id, 0));
	CHECK(index != -1);
	return index;
}

UINT RebarView::GetBandId(int index) const
{
	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_ID;
	auto res = SendMessage(m_hwnd, RB_GETBANDINFO, index, reinterpret_cast<LPARAM>(&bandInfo));
	CHECK(res);

	return bandInfo.wID;
}

void RebarView::OnNcDestroy()
{
	windowDestroyedSignal.m_signal();

	delete this;
}
