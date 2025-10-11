// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListView.h"
#include "ListViewColumnModel.h"
#include "ListViewItem.h"
#include "ListViewModel.h"
#include "ResourceLoader.h"
#include "TestHelper.h"
#include "../Helper/KeyboardState.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/WindowSubclass.h"
#include <wil/common.h>

ListView::ListView(HWND hwnd, const KeyboardState *keyboardState,
	const ResourceLoader *resourceLoader) :
	m_hwnd(hwnd),
	m_keyboardState(keyboardState),
	m_resourceLoader(resourceLoader)
{
	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&ListView::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_hwnd),
		std::bind_front(&ListView::ParentWndProc, this)));
}

ListView::~ListView() = default;

HWND ListView::GetHWND() const
{
	return m_hwnd;
}

void ListView::SetModel(ListViewModel *model)
{
	// The model should only be set once; replacing the model isn't supported.
	CHECK(!m_model);

	m_model = model;

	m_connections.push_back(
		m_model->itemAddedSignal.AddObserver(std::bind_front(&ListView::AddItem, this)));
	m_connections.push_back(
		m_model->itemUpdatedSignal.AddObserver(std::bind_front(&ListView::OnItemUpdated, this)));
	m_connections.push_back(
		m_model->itemMovedSignal.AddObserver(std::bind_front(&ListView::OnItemMoved, this)));
	m_connections.push_back(
		m_model->itemRemovedSignal.AddObserver(std::bind_front(&ListView::RemoveItem, this)));
	m_connections.push_back(m_model->allItemsRemovedSignal.AddObserver(
		std::bind_front(&ListView::RemoveAllItems, this)));
	m_connections.push_back(m_model->sortOrderChangedSignal.AddObserver(
		std::bind_front(&ListView::OnSortOrderChanged, this)));

	m_connections.push_back(m_model->GetColumnModel()->columnVisibilityChangedSignal.AddObserver(
		std::bind_front(&ListView::OnColumnVisibilityChanged, this)));
	m_connections.push_back(m_model->GetColumnModel()->columnMovedSignal.AddObserver(
		std::bind_front(&ListView::OnColumnMoved, this)));

	AddColumns();
	AddItems();
}

void ListView::SetDelegate(ListViewDelegate *delegate)
{
	m_delegate = delegate ? delegate : &m_noOpDelegate;
}

void ListView::AddExtendedStyles(DWORD styles)
{
	ListView_SetExtendedListViewStyleEx(m_hwnd, styles, styles);
}

void ListView::SetImageList(HIMAGELIST imageList, ImageListType imageListType)
{
	int type = (imageListType == ImageListType::NormalIcons) ? LVSIL_NORMAL : LVSIL_SMALL;
	ListView_SetImageList(m_hwnd, imageList, type);
}

void ListView::AddColumns()
{
	for (auto columnId : m_model->GetColumnModel()->GetVisibleColumnIds())
	{
		AddColumn(columnId);
	}
}

void ListView::AddColumn(ListViewColumnId columnId)
{
	auto *columnModel = m_model->GetColumnModel();
	const auto &column = columnModel->GetColumnById(columnId);
	auto text = m_resourceLoader->LoadString(column.nameStringId);

	HWND header = GetHeader();
	int numColumns = Header_GetItemCount(header);
	CHECK_NE(numColumns, -1);

	// Note that the index here is ultimately irrelevant. A column ordering will be applied to
	// change the visual order of the columns.
	//
	// The only index that is important is the index of the primary column, which needs to be added
	// at index 0. That's necessary to ensure that item editing works as expected. That is, the
	// column at index 0 will be used for editing, regardless of its display index.
	int index = (column.id == columnModel->GetPrimaryColumnId()) ? 0 : numColumns;

	LVCOLUMN lvColumn = {};
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = text.data();
	lvColumn.cx = column.width;
	int insertedIndex = ListView_InsertColumn(m_hwnd, index, &lvColumn);
	CHECK_EQ(insertedIndex, index);

	HDITEM hdItem = {};
	hdItem.mask = HDI_LPARAM;
	hdItem.lParam = column.id.value;
	auto res = Header_SetItem(header, insertedIndex, &hdItem);
	CHECK(res);

	UpdateColumnOrdering();

	auto sortColumnId = m_model->GetSortColumnId();

	if (sortColumnId == column.id)
	{
		SetHeaderSortArrow();
	}
}

void ListView::UpdateColumnOrdering()
{
	HWND header = GetHeader();
	int numColumns = Header_GetItemCount(header);
	CHECK_NE(numColumns, -1);

	std::vector<int> ordering(numColumns);
	std::iota(ordering.begin(), ordering.end(), 0);

	auto projection = [this](int index)
	{
		auto visibleIndex =
			m_model->GetColumnModel()->MaybeGetColumnVisibleIndex(GetColumnIdAtIndex(index));
		CHECK(visibleIndex);
		return *visibleIndex;
	};

	std::ranges::sort(ordering, {}, projection);

	auto res = Header_SetOrderArray(header, numColumns, ordering.data());
	CHECK(res);

	res = RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
	CHECK(res);
}

void ListView::AddItems()
{
	int index = 0;

	for (auto *item : m_model->GetItems())
	{
		AddItem(item, index);
		index++;
	}
}

void ListView::AddItem(ListViewItem *item, int index)
{
	LVITEM lvItem = {};
	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.iImage = I_IMAGECALLBACK;
	lvItem.lParam = reinterpret_cast<LPARAM>(item);
	int insertedIndex = ListView_InsertItem(m_hwnd, &lvItem);
	CHECK_EQ(insertedIndex, index);
}

void ListView::OnItemUpdated(const ListViewItem *item)
{
	ResetItemImage(item);
	ResetItemColumns(item);
}

void ListView::OnItemMoved(ListViewItem *item, int newIndex)
{
	bool selected = IsItemSelected(item);

	RemoveItem(item);
	AddItem(item, newIndex);

	if (selected)
	{
		SelectItem(item);
	}
}

void ListView::ResetItemImage(const ListViewItem *item)
{
	LVITEM lvItem = {};
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = GetItemIndex(item);
	lvItem.iSubItem = 0;
	lvItem.iImage = I_IMAGECALLBACK;
	auto res = ListView_SetItem(m_hwnd, &lvItem);
	CHECK(res);
}

void ListView::ResetItemColumns(const ListViewItem *item)
{
	auto index = GetItemIndex(item);
	int numColumns = m_model->GetColumnModel()->GetNumVisibleColumns();

	for (int i = 0; i < numColumns; i++)
	{
		LVITEM lvItem = {};
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = index;
		lvItem.iSubItem = i;
		lvItem.pszText = LPSTR_TEXTCALLBACK;
		auto res = ListView_SetItem(m_hwnd, &lvItem);
		CHECK(res);
	}
}

void ListView::RemoveItem(const ListViewItem *item)
{
	auto res = ListView_DeleteItem(m_hwnd, GetItemIndex(item));
	CHECK(res);
}

void ListView::RemoveAllItems()
{
	auto res = ListView_DeleteAllItems(m_hwnd);
	CHECK(res);
}

std::vector<ListViewItem *> ListView::GetSelectedItems()
{
	std::vector<ListViewItem *> items;
	int index = -1;

	while ((index = ListView_GetNextItem(m_hwnd, index, LVNI_SELECTED)) != -1)
	{
		items.push_back(GetItemAtIndex(index));
	}

	return items;
}

bool ListView::IsItemSelected(const ListViewItem *item) const
{
	UINT state = ListView_GetItemState(m_hwnd, GetItemIndex(item), LVIS_SELECTED);
	return WI_IsFlagSet(state, LVIS_SELECTED);
}

void ListView::SelectItem(const ListViewItem *item)
{
	UpdateItemState(item, LVIS_SELECTED, ItemStateOp::Set);
}

void ListView::SelectAllItems()
{
	UpdateAllItemStates(LVIS_SELECTED, ItemStateOp::Set);
}

void ListView::DeselectAllItems()
{
	UpdateAllItemStates(LVIS_SELECTED, ItemStateOp::Clear);
}

void ListView::StartRenamingItem(const ListViewItem *item)
{
	// For ListView_EditLabel() to succeed, the control needs to have focus.
	SetFocus(m_hwnd);

	auto res = ListView_EditLabel(m_hwnd, GetItemIndex(item));
	CHECK(res);
}

RECT ListView::GetItemRect(const ListViewItem *item) const
{
	RECT itemRect;
	auto res = ListView_GetItemRect(m_hwnd, GetItemIndex(item), &itemRect, LVIR_BOUNDS);
	CHECK(res);
	return itemRect;
}

ListViewItem *ListView::MaybeGetItemAtPoint(const POINT &pt)
{
	LVHITTESTINFO hitTestInfo = {};
	hitTestInfo.pt = pt;
	int index = ListView_HitTest(m_hwnd, &hitTestInfo);

	if (index == -1)
	{
		return nullptr;
	}

	return GetItemAtIndex(index);
}

bool ListView::IsItemHighlighted(const ListViewItem *item) const
{
	UINT state = ListView_GetItemState(m_hwnd, GetItemIndex(item), LVIS_DROPHILITED);
	return WI_IsFlagSet(state, LVIS_DROPHILITED);
}

void ListView::HighlightItem(const ListViewItem *item)
{
	UpdateItemState(item, LVIS_DROPHILITED, ItemStateOp::Set);
}

void ListView::UnhighlightItem(const ListViewItem *item)
{
	UpdateItemState(item, LVIS_DROPHILITED, ItemStateOp::Clear);
}

void ListView::UpdateItemState(const ListViewItem *item, UINT state, ItemStateOp stateOp)
{
	ApplyItemStateUpdates(StateUpdateTarget::Item(GetItemIndex(item)), state, stateOp);
}

void ListView::UpdateAllItemStates(UINT state, ItemStateOp stateOp)
{
	ApplyItemStateUpdates(StateUpdateTarget::AllItems(), state, stateOp);
}

void ListView::ApplyItemStateUpdates(const StateUpdateTarget &target, UINT state,
	ItemStateOp stateOp)
{
	// An index of -1 will result in all items being updated.
	int index = target.IsItem() ? *target.GetItemIndex() : -1;

	LVITEM lvItem = {};
	lvItem.stateMask = state;
	lvItem.state = (stateOp == ItemStateOp::Set) ? state : 0;
	auto res = SendMessage(m_hwnd, LVM_SETITEMSTATE, index, reinterpret_cast<LPARAM>(&lvItem));
	CHECK(res);
}

void ListView::ShowInsertMark(const ListViewItem *targetItem, InsertMarkPosition position)
{
	LVINSERTMARK insertMark = {};
	insertMark.cbSize = sizeof(insertMark);
	insertMark.dwFlags = (position == InsertMarkPosition::After) ? LVIM_AFTER : 0;
	insertMark.iItem = GetItemIndex(targetItem);
	auto res = ListView_SetInsertMark(m_hwnd, &insertMark);
	CHECK(res);
}

void ListView::RemoveInsertMark()
{
	LVINSERTMARK insertMark = {};
	insertMark.cbSize = sizeof(insertMark);
	insertMark.dwFlags = 0;
	insertMark.iItem = -1;
	auto res = ListView_SetInsertMark(m_hwnd, &insertMark);
	CHECK(res);
}

int ListView::FindNextItemIndex(const POINT &pt) const
{
	CHECK(GetViewType() == ViewType::Details);

	int nextIndex = 0;

	for (const auto *item : m_model->GetItems())
	{
		auto rect = GetItemRect(item);

		if (pt.y < rect.top)
		{
			break;
		}

		nextIndex++;
	}

	return nextIndex;
}

void ListView::OnColumnVisibilityChanged(ListViewColumnId columnId, bool visible)
{
	if (visible)
	{
		AddColumn(columnId);
	}
	else
	{
		RemoveColumn(columnId);
	}
}

void ListView::OnColumnMoved(ListViewColumnId columnId, int newVisibleIndex)
{
	UNREFERENCED_PARAMETER(columnId);
	UNREFERENCED_PARAMETER(newVisibleIndex);

	UpdateColumnOrdering();
}

void ListView::RemoveColumn(ListViewColumnId columnId)
{
	auto res = ListView_DeleteColumn(m_hwnd, GetColumnIndex(columnId));
	CHECK(res);
}

void ListView::OnSortOrderChanged()
{
	SortItems();
	UpdateHeaderSortArrow();

	m_previousSortColumnId = m_model->GetSortColumnId();
}

void ListView::SortItems()
{
	ListView_SortItems(
		m_hwnd,
		[](LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
		{
			auto *listView = reinterpret_cast<ListView *>(lParamSort);
			return listView->SortItemsCallback(lParam1, lParam2);
		},
		reinterpret_cast<LPARAM>(this));
}

int ListView::SortItemsCallback(LPARAM lParam1, LPARAM lParam2)
{
	const auto *firstItem = reinterpret_cast<ListViewItem *>(lParam1);
	const auto *secondItem = reinterpret_cast<ListViewItem *>(lParam2);
	return m_model->GetItemIndex(firstItem) - m_model->GetItemIndex(secondItem);
}

void ListView::UpdateHeaderSortArrow()
{
	ClearHeaderSortArrow();
	SetHeaderSortArrow();
}

void ListView::ClearHeaderSortArrow()
{
	if (!m_previousSortColumnId)
	{
		return;
	}

	UpdateHeaderItemFormat(*m_previousSortColumnId, HDF_SORTUP | HDF_SORTDOWN, ItemStateOp::Clear);
}

void ListView::SetHeaderSortArrow()
{
	auto sortColumnId = m_model->GetSortColumnId();

	if (!sortColumnId)
	{
		return;
	}

	int sortOption;

	if (m_model->GetSortDirection() == +SortDirection::Ascending)
	{
		sortOption = HDF_SORTUP;
	}
	else
	{
		sortOption = HDF_SORTDOWN;
	}

	UpdateHeaderItemFormat(*sortColumnId, sortOption, ItemStateOp::Set);
}

void ListView::UpdateHeaderItemFormat(ListViewColumnId columnId, int format, ItemStateOp stateOp)
{
	auto index = MaybeGetColumnIndex(columnId);

	if (!index)
	{
		return;
	}

	HWND header = GetHeader();

	HDITEM headerItem = {};
	headerItem.mask = HDI_FORMAT;
	auto res = Header_GetItem(header, *index, &headerItem);
	CHECK(res);

	if (stateOp == ItemStateOp::Set)
	{
		WI_SetAllFlags(headerItem.fmt, format);
	}
	else
	{
		WI_ClearAllFlags(headerItem.fmt, format);
	}

	res = Header_SetItem(header, *index, &headerItem);
	CHECK(res);
}

LRESULT ListView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		// This key press is used to open the selected items.
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
		}
		break;

	case WM_CONTEXTMENU:
		if (reinterpret_cast<HWND>(wParam) == m_hwnd)
		{
			OnShowContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
			return 0;
		}
		else if (reinterpret_cast<HWND>(wParam) == GetHeader())
		{
			m_delegate->OnShowHeaderContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
			return 0;
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == GetHeader())
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case HDN_ITEMCHANGED:
				OnHeaderItemChanged(reinterpret_cast<NMHEADER *>(lParam));
				break;

			case HDN_ITEMCLICK:
				OnHeaderItemClick(reinterpret_cast<NMHEADER *>(lParam));
				break;

			case HDN_BEGINDRAG:
				return false;

			case HDN_ENDDRAG:
				OnHeaderEndDrag(reinterpret_cast<NMHEADER *>(lParam));
				return true;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ListView::OnShowContextMenu(const POINT &ptScreen)
{
	POINT finalPoint = ptScreen;

	bool keyboardGenerated = false;

	// If the context menu message was generated by the keyboard, there won't be any associated
	// position.
	if (ptScreen.x == -1 && ptScreen.y == -1)
	{
		keyboardGenerated = true;
	}

	auto selectedItems = GetSelectedItems();

	if (selectedItems.empty())
	{
		if (keyboardGenerated)
		{
			finalPoint = { 0, 0 };
			ClientToScreen(m_hwnd, &finalPoint);
		}

		m_delegate->OnShowBackgroundContextMenu(finalPoint);
	}
	else
	{
		if (keyboardGenerated)
		{
			RECT itemRect = GetItemRect(*selectedItems.rbegin());

			finalPoint = { itemRect.left, itemRect.bottom };
			ClientToScreen(m_hwnd, &finalPoint);
		}

		m_delegate->OnShowItemContextMenu(selectedItems, finalPoint);
	}
}

LRESULT ListView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case LVN_GETDISPINFO:
				OnGetDispInfo(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_ITEMACTIVATE:
				WithNonEmptySelection([this](const auto &selectedItems)
					{ m_delegate->OnItemsActivated(selectedItems); });
				break;

			case LVN_BEGINLABELEDIT:
				return OnBeginLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));

			case LVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));

			case LVN_KEYDOWN:
				OnKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
				break;

			case LVN_BEGINDRAG:
				m_delegate->OnBeginDrag(GetSelectedItems());
				break;

			case LVN_DELETEALLITEMS:
				// Suppress subsequent LVN_DELETEITEM notifications.
				return true;
			}
		}
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ListView::OnGetDispInfo(NMLVDISPINFO *dispInfo)
{
	const auto *item = GetItemAtIndex(dispInfo->item.iItem);

	if (WI_IsFlagSet(dispInfo->item.mask, LVIF_TEXT))
	{
		auto columnId = GetColumnIdAtIndex(dispInfo->item.iSubItem);
		const auto &text = item->GetColumnText(columnId);
		StringCchCopy(dispInfo->item.pszText, dispInfo->item.cchTextMax, text.c_str());
	}

	if (WI_IsFlagSet(dispInfo->item.mask, LVIF_IMAGE))
	{
		auto iconIndex = item->GetIconIndex();

		if (iconIndex)
		{
			dispInfo->item.iImage = *iconIndex;
		}
	}

	WI_SetFlag(dispInfo->item.mask, LVIF_DI_SETITEM);
}

bool ListView::OnBeginLabelEdit(const NMLVDISPINFO *dispInfo)
{
	const auto *item = GetItemAtIndex(dispInfo->item.iItem);

	if (!item->CanRename())
	{
		return true;
	}

	return false;
}

bool ListView::OnEndLabelEdit(const NMLVDISPINFO *dispInfo)
{
	if (!dispInfo->item.pszText)
	{
		return false;
	}

	return m_delegate->OnItemRenamed(GetItemAtIndex(dispInfo->item.iItem), dispInfo->item.pszText);
}

void ListView::OnKeyDown(const NMLVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case VK_F2:
		WithNonEmptySelection(
			[this](const auto &selectedItems) { StartRenamingItem(selectedItems[0]); });
		break;

	case 'A':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			SelectAllItems();
		}
		break;

	case 'C':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			WithNonEmptySelection(
				[this](const auto &selectedItems) { m_delegate->OnItemsCopied(selectedItems); });
		}
		break;

	case 'X':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			WithNonEmptySelection(
				[this](const auto &selectedItems) { m_delegate->OnItemsCut(selectedItems); });
		}
		break;

	case 'V':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			OnPaste();
		}
		break;

	case VK_INSERT:
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			WithNonEmptySelection(
				[this](const auto &selectedItems) { m_delegate->OnItemsCopied(selectedItems); });
		}
		if (!m_keyboardState->IsCtrlDown() && m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			OnPaste();
		}
		break;

	case VK_DELETE:
		WithNonEmptySelection(
			[this](const auto &selectedItems)
			{
				m_delegate->OnItemsRemoved(selectedItems,
					m_keyboardState->IsShiftDown() ? RemoveMode::Permanent : RemoveMode::Standard);
			});
		break;
	}
}

void ListView::WithNonEmptySelection(
	std::function<void(const std::vector<ListViewItem *> &selectedItems)> callback)
{
	auto selectedItems = GetSelectedItems();

	if (selectedItems.empty())
	{
		return;
	}

	callback(selectedItems);
}

void ListView::OnPaste()
{
	ListViewItem *lastSelectedItem = nullptr;

	if (!m_model->GetSortColumnId() && m_model->HasDefaultSortOrder())
	{
		// In this case, the items are sorted in the default order, so the pasted item should be
		// added after the last selected index.
		auto selectedItems = GetSelectedItems();

		if (!selectedItems.empty())
		{
			lastSelectedItem = *selectedItems.rbegin();
		}
	}

	m_delegate->OnPaste(lastSelectedItem);
}

void ListView::OnHeaderItemChanged(const NMHEADER *changeInfo)
{
	if (!changeInfo->pitem || WI_IsFlagClear(changeInfo->pitem->mask, HDI_WIDTH))
	{
		return;
	}

	auto columnId = GetColumnIdAtIndex(changeInfo->iItem);
	auto &column = m_model->GetColumnModel()->GetColumnById(columnId);
	column.width = changeInfo->pitem->cxy;
}

void ListView::OnHeaderItemClick(const NMHEADER *header)
{
	auto columnId = GetColumnIdAtIndex(header->iItem);
	auto sortColumnId = m_model->GetSortColumnId();

	if (columnId == sortColumnId)
	{
		if (m_model->GetSortDirection() == +SortDirection::Ascending)
		{
			m_model->SetSortDetails(columnId, SortDirection::Descending);
		}
		else
		{
			if (m_model->HasDefaultSortOrder())
			{
				m_model->SetSortDetails(std::nullopt, SortDirection::Ascending);
			}
			else
			{
				m_model->SetSortDetails(columnId, SortDirection::Ascending);
			}
		}
	}
	else
	{
		m_model->SetSortDetails(columnId, SortDirection::Ascending);
	}
}

void ListView::OnHeaderEndDrag(const NMHEADER *changeInfo)
{
	if (!changeInfo->pitem || WI_IsFlagClear(changeInfo->pitem->mask, HDI_ORDER))
	{
		DCHECK(false);
		return;
	}

	if (changeInfo->pitem->iOrder == -1)
	{
		// The drag was cancelled.
		return;
	}

	auto columnId = GetColumnIdAtIndex(changeInfo->iItem);

	if (GetColumnIdAtVisibleIndex(changeInfo->pitem->iOrder) == columnId)
	{
		return;
	}

	auto *columnModel = m_model->GetColumnModel();
	columnModel->MoveColumn(columnId, changeInfo->pitem->iOrder);
}

ListView::ViewType ListView::GetViewType() const
{
	ViewType viewType;
	auto nativeViewType = ListView_GetView(m_hwnd);

	switch (nativeViewType)
	{
	case LV_VIEW_ICON:
		viewType = ViewType::Icon;
		break;

	case LV_VIEW_SMALLICON:
		viewType = ViewType::SmallIcon;
		break;

	case LV_VIEW_LIST:
		viewType = ViewType::List;
		break;

	case LV_VIEW_DETAILS:
		viewType = ViewType::Details;
		break;

	case LV_VIEW_TILE:
		viewType = ViewType::Tiles;
		break;

	default:
		LOG(FATAL) << "Invalid view type";
	}

	return viewType;
}

ListViewColumnId ListView::GetColumnIdAtVisibleIndex(int index) const
{
	return GetColumnIdAtIndex(Header_OrderToIndex(GetHeader(), index));
}

ListViewColumnId ListView::GetColumnIdAtIndex(int index) const
{
	HDITEM hdItem = {};
	hdItem.mask = HDI_LPARAM;
	auto res = Header_GetItem(GetHeader(), index, &hdItem);
	CHECK(res);
	return ListViewColumnId(static_cast<int>(hdItem.lParam));
}

int ListView::GetColumnIndex(ListViewColumnId columnId) const
{
	auto index = MaybeGetColumnIndex(columnId);
	CHECK(index);
	return *index;
}

std::optional<int> ListView::MaybeGetColumnIndex(ListViewColumnId columnId) const
{
	int numColumns = Header_GetItemCount(GetHeader());
	CHECK_NE(numColumns, -1);

	for (int i = 0; i < numColumns; i++)
	{
		if (GetColumnIdAtIndex(i) == columnId)
		{
			return i;
		}
	}

	return std::nullopt;
}

ListViewItem *ListView::GetItemAtIndex(int index)
{
	return const_cast<ListViewItem *>(std::as_const(*this).GetItemAtIndex(index));
}

const ListViewItem *ListView::GetItemAtIndex(int index) const
{
	LVITEM lvItem = {};
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	auto res = ListView_GetItem(m_hwnd, &lvItem);
	CHECK(res);
	return reinterpret_cast<ListViewItem *>(lvItem.lParam);
}

int ListView::GetItemIndex(const ListViewItem *item) const
{
	LV_FINDINFO lvFind = {};
	lvFind.flags = LVFI_PARAM;
	lvFind.lParam = reinterpret_cast<LPARAM>(item);
	int index = ListView_FindItem(m_hwnd, -1, &lvFind);
	CHECK_NE(index, -1);
	return index;
}

HWND ListView::GetHeader() const
{
	HWND header = ListView_GetHeader(m_hwnd);
	CHECK(header);
	return header;
}

std::vector<ListViewColumnId> ListView::GetColumnsInVisibleOrderForTesting() const
{
	CHECK(IsInTest());

	int numColumns = Header_GetItemCount(GetHeader());
	CHECK_NE(numColumns, -1);

	std::vector<ListViewColumnId> columnIds;

	for (int i = 0; i < numColumns; i++)
	{
		columnIds.push_back(GetColumnIdAtVisibleIndex(i));
	}

	return columnIds;
}

std::optional<ListView::ColumnSortArrowDetails> ListView::GetColumnSortArrowDetailsForTesting()
	const
{
	CHECK(IsInTest());

	std::optional<ColumnSortArrowDetails> arrowDetails;
	HWND header = GetHeader();

	int numColumns = Header_GetItemCount(header);
	CHECK_NE(numColumns, -1);

	for (int i = 0; i < numColumns; i++)
	{
		HDITEM headerItem = {};
		headerItem.mask = HDI_FORMAT;
		auto res = Header_GetItem(header, i, &headerItem);
		CHECK(res);

		if (WI_IsFlagSet(headerItem.fmt, HDF_SORTUP))
		{
			// The arrow should only be shown on a single column at once.
			CHECK(!arrowDetails);
			arrowDetails = { GetColumnIdAtIndex(i), SortDirection::Ascending };
		}
		else if (WI_IsFlagSet(headerItem.fmt, HDF_SORTDOWN))
		{
			CHECK(!arrowDetails);
			arrowDetails = { GetColumnIdAtIndex(i), SortDirection::Descending };
		}
	}

	return arrowDetails;
}

const ListViewItem *ListView::GetItemAtIndexForTesting(int index) const
{
	CHECK(IsInTest());
	return GetItemAtIndex(index);
}

std::wstring ListView::GetItemTextForTesting(int item, int subItem) const
{
	CHECK(IsInTest());
	return ListViewHelper::GetItemText(m_hwnd, item, subItem);
}

int ListView::GetItemCountForTesting() const
{
	CHECK(IsInTest());
	return ListView_GetItemCount(m_hwnd);
}

bool ListView::NoOpDelegate::OnItemRenamed(ListViewItem *item, const std::wstring &name)
{
	UNREFERENCED_PARAMETER(item);
	UNREFERENCED_PARAMETER(name);
	return false;
}

void ListView::NoOpDelegate::OnItemsActivated(const std::vector<ListViewItem *> &items)
{
	UNREFERENCED_PARAMETER(items);
}

void ListView::NoOpDelegate::OnItemsRemoved(const std::vector<ListViewItem *> &items,
	RemoveMode removeMode)
{
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(removeMode);
}

void ListView::NoOpDelegate::OnItemsCopied(const std::vector<ListViewItem *> &items)
{
	UNREFERENCED_PARAMETER(items);
}

void ListView::NoOpDelegate::OnItemsCut(const std::vector<ListViewItem *> &items)
{
	UNREFERENCED_PARAMETER(items);
}

void ListView::NoOpDelegate::OnPaste(ListViewItem *lastSelectedItemOpt)
{
	UNREFERENCED_PARAMETER(lastSelectedItemOpt);
}

void ListView::NoOpDelegate::OnShowBackgroundContextMenu(const POINT &ptScreen)
{
	UNREFERENCED_PARAMETER(ptScreen);
}

void ListView::NoOpDelegate::OnShowItemContextMenu(const std::vector<ListViewItem *> &items,
	const POINT &ptScreen)
{
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(ptScreen);
}

void ListView::NoOpDelegate::OnShowHeaderContextMenu(const POINT &ptScreen)
{
	UNREFERENCED_PARAMETER(ptScreen);
}

void ListView::NoOpDelegate::OnBeginDrag(const std::vector<ListViewItem *> &items)
{
	UNREFERENCED_PARAMETER(items);
}

ListView::StateUpdateTarget ListView::StateUpdateTarget::AllItems()
{
	return StateUpdateTarget(AllItemsTag{});
}

ListView::StateUpdateTarget ListView::StateUpdateTarget::Item(int index)
{
	return StateUpdateTarget(index);
}

ListView::StateUpdateTarget::StateUpdateTarget(AllItemsTag) : m_target(AllItemsTag{})
{
}

ListView::StateUpdateTarget::StateUpdateTarget(int index) : m_target(index)
{
}

bool ListView::StateUpdateTarget::IsAllItems() const
{
	return std::holds_alternative<AllItemsTag>(m_target);
}

bool ListView::StateUpdateTarget::IsItem() const
{
	return std::holds_alternative<int>(m_target);
}

std::optional<int> ListView::StateUpdateTarget::GetItemIndex() const
{
	if (const auto *index = std::get_if<int>(&m_target))
	{
		return *index;
	}

	return std::nullopt;
}
