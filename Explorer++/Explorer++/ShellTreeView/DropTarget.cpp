// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

DWORD ShellTreeView::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	m_currentDropObject = dataObject;
	return OnDragInWindow(dataObject, keyState, pt, effect);
}

DWORD ShellTreeView::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	assert(m_currentDropObject);
	return OnDragInWindow(m_currentDropObject, keyState, pt, effect);
}

void ShellTreeView::DragLeave()
{
	ResetDropState();
}

DWORD ShellTreeView::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	HTREEITEM dropLocation = GetDropLocation(pt);
	DWORD targetEffect =
		PerformDrop(dropLocation, dataObject, m_previousKeyState, keyState, pt, effect);

	ResetDropState();

	return targetEffect;
}

DWORD ShellTreeView::OnDragInWindow(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	HTREEITEM dropLocation = GetDropLocation(pt);
	DWORD targetEffect = GetDropEffect(dropLocation, dataObject, keyState, pt, effect);

	UpdateUiForDrop(dropLocation, pt);

	m_previousKeyState = keyState;
	m_previousDropLocation = dropLocation;

	return targetEffect;
}

HTREEITEM ShellTreeView::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hTreeView, &ptClient);

	if (!res)
	{
		return nullptr;
	}

	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	return TreeView_HitTest(m_hTreeView, &hitTestInfo);
}

DWORD ShellTreeView::GetDropEffect(
	HTREEITEM dropLocation, IDataObject *dataObject, DWORD keyState, POINT pt, DWORD allowedEffects)
{
	if (m_previousDropLocation && m_previousDropLocation != dropLocation && m_previousDropTargetInfo
		&& m_previousDropTargetInfo->dropTarget)
	{
		m_previousDropTargetInfo->dropTarget->DragLeave();
		m_previousDropTargetInfo.reset();
	}

	auto dropTargetInfo = GetDropTargetInfoForItem(dropLocation);
	m_previousDropTargetInfo = dropTargetInfo;

	if (!dropTargetInfo.dropTarget)
	{
		return DROPEFFECT_NONE;
	}

	DWORD targetEffect = allowedEffects;
	HRESULT hr;

	if (dropTargetInfo.dropTargetInitialised)
	{
		hr = dropTargetInfo.dropTarget->DragOver(keyState, { pt.x, pt.y }, &targetEffect);
	}
	else
	{
		hr = dropTargetInfo.dropTarget->DragEnter(
			dataObject, keyState, { pt.x, pt.y }, &targetEffect);
	}

	if (FAILED(hr))
	{
		return DROPEFFECT_NONE;
	}

	m_previousDropTargetInfo->dropTargetInitialised = true;

	return targetEffect;
}

DWORD ShellTreeView::PerformDrop(HTREEITEM dropLocation, IDataObject *dataObject,
	DWORD previousKeyState, DWORD keyState, POINT pt, DWORD allowedEffects)
{
	auto dropTargetInfo = GetDropTargetInfoForItem(dropLocation);

	if (!dropTargetInfo.dropTarget)
	{
		return DROPEFFECT_NONE;
	}

	DWORD targetEffect;
	HRESULT hr;

	if (!dropTargetInfo.dropTargetInitialised)
	{
		// Note that the key state provided to this method is used to determine whether this is a
		// left-click or right-click drag. When the Drop() method is called, the mouse button that
		// originally started the drag wouldn't be down, so passing the final key state to this
		// method would mean it wouldn't be able to properly detect a left-click/right-click drag.
		// Therefore, the key state here is the state that was in effect right before the drop. At
		// that point, the mouse button that started the drag will have still been down.
		targetEffect = allowedEffects;
		hr = dropTargetInfo.dropTarget->DragEnter(
			dataObject, previousKeyState, { pt.x, pt.y }, &targetEffect);

		if (FAILED(hr) || targetEffect == DROPEFFECT_NONE)
		{
			return DROPEFFECT_NONE;
		}
	}

	targetEffect = allowedEffects;
	hr = dropTargetInfo.dropTarget->Drop(dataObject, keyState, { pt.x, pt.y }, &targetEffect);

	if (FAILED(hr))
	{
		return DROPEFFECT_NONE;
	}

	return targetEffect;
}

ShellTreeView::DropTargetInfo ShellTreeView::GetDropTargetInfoForItem(HTREEITEM treeItem)
{
	if (!treeItem)
	{
		return { nullptr, false };
	}

	if (treeItem == m_previousDropLocation)
	{
		return *m_previousDropTargetInfo;
	}

	auto dropTarget = GetDropTargetForItem(treeItem);
	return { dropTarget, false };
}

wil::com_ptr_nothrow<IDropTarget> ShellTreeView::GetDropTargetForItem(HTREEITEM treeItem)
{
	auto &item = GetItemByHandle(treeItem);

	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, item.pidl.get(), nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IDropTarget> dropTarget;
	hr = shellFolder->CreateViewObject(m_hTreeView, IID_PPV_ARGS(&dropTarget));

	if (FAILED(hr))
	{
		return nullptr;
	}

	return dropTarget;
}

void ShellTreeView::UpdateUiForDrop(HTREEITEM dropLocation, const POINT &pt)
{
	UpdateUiForDropLocation(dropLocation);

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

void ShellTreeView::UpdateUiForDropLocation(HTREEITEM dropLocation)
{
	if (dropLocation)
	{
		TreeView_Select(m_hTreeView, dropLocation, TVGN_DROPHILITE);

		if (m_dropExpandItem != dropLocation)
		{
			m_dropExpandItem = dropLocation;
			SetTimer(m_hTreeView, DROP_EXPAND_TIMER_ID, DROP_EXPAND_TIMER_ELAPSE, nullptr);
		}
	}
	else
	{
		TreeView_Select(m_hTreeView, nullptr, TVGN_DROPHILITE);

		KillTimer(m_hTreeView, DROP_EXPAND_TIMER_ID);
	}
}

void ShellTreeView::OnDropExpandTimer()
{
	TreeView_Expand(m_hTreeView, m_dropExpandItem, TVE_EXPAND);

	KillTimer(m_hTreeView, DROP_EXPAND_TIMER_ID);
}

void ShellTreeView::ResetDropState()
{
	ResetDropUiState();

	m_currentDropObject = nullptr;
	m_previousDropLocation = nullptr;
	m_previousDropTargetInfo.reset();
	m_previousKeyState = 0;
	m_dropExpandItem = nullptr;
}

void ShellTreeView::ResetDropUiState()
{
	TreeView_Select(m_hTreeView, nullptr, TVGN_DROPHILITE);

	KillTimer(m_hTreeView, DROP_EXPAND_TIMER_ID);
}