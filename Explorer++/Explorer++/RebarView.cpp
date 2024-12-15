// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RebarView.h"
#include "MainRebarStorage.h"
#include <wil/common.h>

RebarView::RebarView(HWND parent, DWORD style) :
	m_hwnd(CreateRebar(parent, style)),
	m_subclass(m_hwnd, std::bind_front(&RebarView::WndProc, this))
{
}

HWND RebarView::CreateRebar(HWND parent, DWORD style)
{
	return CreateWindowEx(WS_EX_CONTROLPARENT, REBARCLASSNAME, L"", style, 0, 0, 0, 0, parent,
		nullptr, GetModuleHandle(nullptr), nullptr);
}

LRESULT RebarView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCDESTROY:
		delete this;
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HWND RebarView::GetHWND() const
{
	return m_hwnd;
}

UINT RebarView::GetHeight() const
{
	return static_cast<UINT>(SendMessage(m_hwnd, RB_GETBARHEIGHT, 0, 0));
}

void RebarView::AddBands(const std::vector<Band> &bands)
{
	for (const auto &band : bands)
	{
		AddBand(band);
	}
}

void RebarView::AddBand(const Band &band)
{
	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
	bandInfo.fStyle = 0;
	bandInfo.wID = band.id;
	bandInfo.hwndChild = band.child;
	bandInfo.cx = band.length;
	bandInfo.cxMinChild = 0;
	bandInfo.cyMinChild = band.height;
	bandInfo.cyChild = band.height;

	if (m_locked)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
	}

	if (band.newLine)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_BREAK);
	}

	if (band.useChevron)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_USECHEVRON);
	}

	if (!band.show)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_HIDDEN);
	}

	if (band.idealLength)
	{
		WI_SetFlag(bandInfo.fMask, RBBIM_IDEALSIZE);
		bandInfo.cxIdeal = *band.idealLength;
	}

	auto res = SendMessage(m_hwnd, RB_INSERTBAND, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(&bandInfo));
	DCHECK(res);
}

void RebarView::ShowBand(HWND child, bool show)
{
	auto numBands = static_cast<UINT>(SendMessage(m_hwnd, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(bandInfo);
		bandInfo.fMask = RBBIM_CHILD;
		auto res = SendMessage(m_hwnd, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		if (child == bandInfo.hwndChild)
		{
			res = SendMessage(m_hwnd, RB_SHOWBAND, i, show);
			DCHECK(res);
			break;
		}
	}
}

void RebarView::LockBands(bool lock)
{
	auto numBands = static_cast<UINT>(SendMessage(m_hwnd, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(REBARBANDINFO);
		bandInfo.fMask = RBBIM_STYLE;
		auto res = SendMessage(m_hwnd, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		// Without this, the control won't correctly update once the gripper has been added or
		// removed. That is, the control won't add or remove the space for the gripper. By
		// toggling the RBBS_GRIPPERALWAYS style, adding or removing the gripper will work as
		// expected. WTL also does this when toggling the gripper - see
		// https://sourceforge.net/p/wtl/git/ci/faa1f28fb7fe9277532ed563101b489655b40131/tree/Include/atlctrls.h#l8604.
		WI_SetFlag(bandInfo.fStyle, RBBS_GRIPPERALWAYS);
		res = SendMessage(m_hwnd, RB_SETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));
		DCHECK(res);
		WI_ClearFlag(bandInfo.fStyle, RBBS_GRIPPERALWAYS);

		if (lock)
		{
			WI_SetFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
		}
		else
		{
			WI_ClearFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
		}

		res = SendMessage(m_hwnd, RB_SETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));
		DCHECK(res);
	}

	m_locked = lock;
}

// This method should be called when the size of a control contained within a rebar changes. For
// example, adding or removing buttons from a toolbar will change the toolbar's ideal width (i.e.
// the width needed to show every button). Changing a control's font can change both its ideal width
// and its height.
// Setting the ideal width for a band is important, since when a band is unlocked, clicking the
// gripper will resize the band to its ideal size, which should match the size of the content in the
// control.
// Setting the height is also important, since otherwise, the band may end up being too small or too
// large.
void RebarView::UpdateBandSize(HWND child, int idealWidth, int height)
{
	auto numBands = static_cast<UINT>(SendMessage(m_hwnd, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(bandInfo);
		bandInfo.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE;
		auto res = SendMessage(m_hwnd, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (res == 0)
		{
			DCHECK(false);
			continue;
		}

		if (bandInfo.hwndChild == child)
		{
			bandInfo.fMask = RBBIM_IDEALSIZE | RBBIM_CHILDSIZE;
			bandInfo.cxIdeal = idealWidth;
			bandInfo.cyMinChild = height;
			res = SendMessage(m_hwnd, RB_SETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));
			DCHECK(res);
			break;
		}
	}
}

std::vector<RebarBandStorageInfo> RebarView::GetStorageData() const
{
	std::vector<RebarBandStorageInfo> rebarStorageInfo;
	auto numBands = static_cast<UINT>(SendMessage(m_hwnd, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(bandInfo);
		bandInfo.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
		auto res = SendMessage(m_hwnd, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		RebarBandStorageInfo bandStorageInfo;
		bandStorageInfo.id = bandInfo.wID;
		bandStorageInfo.style = bandInfo.fStyle;
		bandStorageInfo.length = bandInfo.cx;
		rebarStorageInfo.push_back(bandStorageInfo);
	}

	return rebarStorageInfo;
}
