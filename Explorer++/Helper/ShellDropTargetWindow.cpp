// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellDropTargetWindow.h"
#include "DragDropHelper.h"

template <typename DropTargetItemIdentifierType>
ShellDropTargetWindow<DropTargetItemIdentifierType>::ShellDropTargetWindow(HWND hwnd) :
	m_hwnd(hwnd),
	m_currentDropObject(nullptr),
	m_previousKeyState(0)
{
	m_dropTargetWindow =
		winrt::make_self<DropTargetWindow>(hwnd, static_cast<DropTargetInternal *>(this));
}

template <typename DropTargetItemIdentifierType>
HWND ShellDropTargetWindow<DropTargetItemIdentifierType>::GetHWND() const
{
	return m_hwnd;
}

template <typename DropTargetItemIdentifierType>
DWORD ShellDropTargetWindow<DropTargetItemIdentifierType>::DragEnter(IDataObject *dataObject,
	DWORD keyState, POINT pt, DWORD effect)
{
	if (WI_IsFlagSet(keyState, MK_RBUTTON))
	{
		m_dropType = DropType::RightClick;
	}
	else
	{
		m_dropType = DropType::LeftClick;
	}

	m_currentDropObject = dataObject;
	return OnDragInWindow(dataObject, keyState, pt, effect);
}

template <typename DropTargetItemIdentifierType>
DWORD ShellDropTargetWindow<DropTargetItemIdentifierType>::DragOver(DWORD keyState, POINT pt,
	DWORD effect)
{
	CHECK(m_currentDropObject);
	return OnDragInWindow(m_currentDropObject, keyState, pt, effect);
}

template <typename DropTargetItemIdentifierType>
void ShellDropTargetWindow<DropTargetItemIdentifierType>::DragLeave()
{
	if (m_previousTargetInfo && m_previousTargetInfo->dropTarget
		&& m_previousTargetInfo->dropTargetInitialised)
	{
		m_previousTargetInfo->dropTarget->DragLeave();
		m_previousTargetInfo.reset();
	}

	ResetDropState();
}

template <typename DropTargetItemIdentifierType>
DWORD ShellDropTargetWindow<DropTargetItemIdentifierType>::Drop(IDataObject *dataObject,
	DWORD keyState, POINT pt, DWORD effect)
{
	auto targetItem = GetDropTargetItem(pt);
	DWORD targetEffect =
		PerformDrop(targetItem, dataObject, m_previousKeyState, keyState, pt, effect);

	ResetDropState();

	return targetEffect;
}

template <typename DropTargetItemIdentifierType>
DWORD ShellDropTargetWindow<DropTargetItemIdentifierType>::OnDragInWindow(IDataObject *dataObject,
	DWORD keyState, POINT pt, DWORD effect)
{
	auto targetItem = GetDropTargetItem(pt);
	DWORD targetEffect = GetDropEffect(targetItem, dataObject, keyState, pt, effect);

	UpdateUiForDrop(targetItem, pt);

	m_previousKeyState = keyState;

	return targetEffect;
}

template <typename DropTargetItemIdentifierType>
DWORD ShellDropTargetWindow<DropTargetItemIdentifierType>::GetDropEffect(
	DropTargetItemIdentifierType targetItem, IDataObject *dataObject, DWORD keyState, POINT pt,
	DWORD allowedEffects)
{
	if (m_previousTargetInfo && m_previousTargetInfo->item != targetItem
		&& m_previousTargetInfo->dropTarget && m_previousTargetInfo->dropTargetInitialised)
	{
		m_previousTargetInfo->dropTarget->DragLeave();
		m_previousTargetInfo.reset();
	}

	auto dropTargetInfo = GetDropTargetInfoForItem(targetItem);
	m_previousTargetInfo = dropTargetInfo;

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
		hr = dropTargetInfo.dropTarget->DragEnter(dataObject, keyState, { pt.x, pt.y },
			&targetEffect);
	}

	if (FAILED(hr))
	{
		return DROPEFFECT_NONE;
	}

	// Note that MK_XBUTTON1 corresponds to the ALT key.
	if (IsTargetSourceOfDrop(targetItem) && WI_IsFlagClear(keyState, MK_SHIFT)
		&& WI_IsFlagClear(keyState, MK_CONTROL) && WI_IsFlagClear(keyState, MK_XBUTTON1))
	{
		SetDropDescription(dataObject, DROPIMAGE_NOIMAGE, L"", L"");
	}

	m_previousTargetInfo->dropTargetInitialised = true;

	return targetEffect;
}

template <typename DropTargetItemIdentifierType>
typename ShellDropTargetWindow<DropTargetItemIdentifierType>::DropTargetInfo ShellDropTargetWindow<
	DropTargetItemIdentifierType>::GetDropTargetInfoForItem(DropTargetItemIdentifierType targetItem)
{
	if (m_previousTargetInfo && targetItem == m_previousTargetInfo->item)
	{
		return *m_previousTargetInfo;
	}

	auto pidl = GetPidlForTargetItem(targetItem);

	if (!pidl)
	{
		return { targetItem, nullptr, false };
	}

	auto dropTarget = GetDropTargetForPidl(pidl.get());
	return { targetItem, dropTarget, false };
}

template <typename DropTargetItemIdentifierType>
wil::com_ptr_nothrow<IDropTarget> ShellDropTargetWindow<
	DropTargetItemIdentifierType>::GetDropTargetForPidl(PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellFolder> parent;
	PCITEMID_CHILD child;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&parent), &child);

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IDropTarget> dropTarget;
	hr = GetUIObjectOf(parent.get(), m_hwnd, 1, &child, IID_PPV_ARGS(&dropTarget));

	if (FAILED(hr))
	{
		return nullptr;
	}

	auto site = GetSiteForTargetItem(pidl);

	if (site)
	{
		// This interface isn't available for at least some types of items (e.g. zip files). That
		// means dropping an item into a zip file won't result in the item being selected (Windows
		// Explorer doesn't select the item either).
		wil::com_ptr_nothrow<IObjectWithSite> objectWithSite;
		hr = dropTarget->QueryInterface(IID_PPV_ARGS(&objectWithSite));

		if (SUCCEEDED(hr))
		{
			objectWithSite->SetSite(site);
		}
	}

	return dropTarget;
}

template <typename DropTargetItemIdentifierType>
DWORD ShellDropTargetWindow<DropTargetItemIdentifierType>::PerformDrop(
	DropTargetItemIdentifierType targetItem, IDataObject *dataObject, DWORD previousKeyState,
	DWORD keyState, POINT pt, DWORD allowedEffects)
{
	auto dropTargetInfo = GetDropTargetInfoForItem(targetItem);

	if (!dropTargetInfo.dropTarget)
	{
		return DROPEFFECT_NONE;
	}

	if (m_dropType == DropType::LeftClick && IsTargetSourceOfDrop(targetItem)
		&& WI_IsFlagClear(keyState, MK_SHIFT) && WI_IsFlagClear(keyState, MK_CONTROL)
		&& WI_IsFlagClear(keyState, MK_XBUTTON1))
	{
		// The drag was started in the current window and the target is the folder represented by
		// that window.
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
		hr = dropTargetInfo.dropTarget->DragEnter(dataObject, previousKeyState, { pt.x, pt.y },
			&targetEffect);

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

template <typename DropTargetItemIdentifierType>
void ShellDropTargetWindow<DropTargetItemIdentifierType>::ResetDropState()
{
	ResetDropUiState();

	m_currentDropObject = nullptr;
	m_previousKeyState = 0;
	m_previousTargetInfo.reset();
}

template <typename DropTargetItemIdentifierType>
bool ShellDropTargetWindow<DropTargetItemIdentifierType>::IsWithinDrag() const
{
	return m_dropTargetWindow->IsWithinDrag();
}

template class ShellDropTargetWindow<int>;
template class ShellDropTargetWindow<HTREEITEM>;
