// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "App.h"
#include "ShellTreeNode.h"
#include "../Helper/DpiCompatibility.h"

HTREEITEM ShellTreeView::GetDropTargetItem(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hTreeView, &ptClient);

	if (!res)
	{
		return nullptr;
	}

	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	HTREEITEM item = TreeView_HitTest(m_hTreeView, &hitTestInfo);

	if (!item)
	{
		return nullptr;
	}

	if (m_performingDrag)
	{
		ShellTreeNode *node = GetNodeFromTreeViewItem(item);

		// An item can't be dropped on itself.
		if (ArePidlsEquivalent(node->GetFullPidl().get(), m_draggedItemPidl))
		{
			return nullptr;
		}
	}

	return item;
}

unique_pidl_absolute ShellTreeView::GetPidlForTargetItem(HTREEITEM targetItem)
{
	if (!targetItem)
	{
		return nullptr;
	}

	auto *node = GetNodeFromTreeViewItem(targetItem);
	return node->GetFullPidl();
}

IUnknown *ShellTreeView::GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl)
{
	UNREFERENCED_PARAMETER(targetItemPidl);

	return nullptr;
}

bool ShellTreeView::IsTargetSourceOfDrop(HTREEITEM targetItem, IDataObject *dataObject)
{
	UNREFERENCED_PARAMETER(targetItem);
	UNREFERENCED_PARAMETER(dataObject);

	return false;
}

void ShellTreeView::UpdateUiForDrop(HTREEITEM targetItem, const POINT &pt)
{
	UpdateUiForTargetItem(targetItem);
	ScrollTreeViewForDrop(pt);
}

void ShellTreeView::UpdateUiForTargetItem(HTREEITEM targetItem)
{
	using namespace std::chrono_literals;

	if (targetItem)
	{
		TreeView_Select(m_hTreeView, targetItem, TVGN_DROPHILITE);

		if (m_dropExpandItem != targetItem)
		{
			// Note that although the provided executor outlives this class, the timer will be
			// cancelled when it's destroyed (i.e. when this class is destroyed). So, it's not
			// possible for the timer to fire after this class has been destroyed and the callback
			// below is safe.
#pragma warning(push)
#pragma warning(                                                                                   \
	disable : 4244) // 'argument': conversion from '_Rep' to 'size_t', possible loss of data
			m_dropExpandTimer = m_app->GetRuntime()->GetTimerQueue()->make_one_shot_timer(800ms,
				m_app->GetRuntime()->GetUiThreadExecutor(),
				std::bind_front(&ShellTreeView::OnDropExpandTimer, this));
#pragma warning(pop)
		}
	}
	else
	{
		TreeView_Select(m_hTreeView, nullptr, TVGN_DROPHILITE);

		m_dropExpandTimer.cancel();
	}

	m_dropExpandItem = targetItem;
}

void ShellTreeView::ScrollTreeViewForDrop(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hTreeView, &ptClient);

	if (!res)
	{
		return;
	}

	RECT rc;
	res = GetClientRect(m_hTreeView, &rc);

	if (!res)
	{
		return;
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hTreeView);

	if (ptClient.x < MulDiv(DROP_SCROLL_MARGIN_X_96DPI, dpi, USER_DEFAULT_SCREEN_DPI))
	{
		SendMessage(m_hTreeView, WM_HSCROLL, SB_LINELEFT, NULL);
	}
	else if (ptClient.x
		> (rc.right - MulDiv(DROP_SCROLL_MARGIN_X_96DPI, dpi, USER_DEFAULT_SCREEN_DPI)))
	{
		SendMessage(m_hTreeView, WM_HSCROLL, SB_LINERIGHT, NULL);
	}

	if (ptClient.y < MulDiv(DROP_SCROLL_MARGIN_Y_96DPI, dpi, USER_DEFAULT_SCREEN_DPI))
	{
		SendMessage(m_hTreeView, WM_VSCROLL, SB_LINEUP, NULL);
	}
	else if (ptClient.y
		> (rc.bottom - MulDiv(DROP_SCROLL_MARGIN_Y_96DPI, dpi, USER_DEFAULT_SCREEN_DPI)))
	{
		SendMessage(m_hTreeView, WM_VSCROLL, SB_LINEDOWN, NULL);
	}
}

void ShellTreeView::OnDropExpandTimer()
{
	CHECK(m_dropExpandItem);
	TreeView_Expand(m_hTreeView, m_dropExpandItem, TVE_EXPAND);
}

void ShellTreeView::ResetDropUiState()
{
	TreeView_Select(m_hTreeView, nullptr, TVGN_DROPHILITE);

	m_dropExpandItem = nullptr;
	m_dropExpandTimer.cancel();
}
