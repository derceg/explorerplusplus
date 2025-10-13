// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "InsertMarkPosition.h"
#include "ItemStateOp.h"
#include "ListViewColumn.h"
#include "ListViewDelegate.h"
#include "../Helper/SortDirection.h"
#include <boost/signals2.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class KeyboardState;
class LabelEditHandler;
class ListViewItem;
class ListViewModel;
class ResourceLoader;
class WindowSubclass;

class ListView
{
public:
	using LabelEditHandlerFactory = std::function<LabelEditHandler *(HWND hwnd, bool itemIsFile)>;

	enum class ImageListType
	{
		SmallIcons,
		NormalIcons
	};

	struct ColumnSortArrowDetails
	{
		ListViewColumnId columnId;
		SortDirection direction;

		bool operator==(const ColumnSortArrowDetails &) const = default;
	};

	ListView(HWND hwnd, const KeyboardState *keyboardState,
		LabelEditHandlerFactory labelEditHandlerFactory, const ResourceLoader *resourceLoader);
	~ListView();

	HWND GetHWND() const;

	void SetModel(ListViewModel *model);
	void SetDelegate(ListViewDelegate *delegate);

	void AddExtendedStyles(DWORD styles);
	void SetImageList(HIMAGELIST imageList, ImageListType imageListType);

	std::vector<ListViewItem *> GetSelectedItems();
	bool IsItemSelected(const ListViewItem *item) const;
	void SelectItem(const ListViewItem *item);
	void SelectAllItems();
	void DeselectAllItems();
	void StartRenamingItem(const ListViewItem *item);
	RECT GetItemRect(const ListViewItem *item) const;
	ListViewItem *MaybeGetItemAtPoint(const POINT &pt);
	bool IsItemHighlighted(const ListViewItem *item) const;
	void SetItemHighlighted(const ListViewItem *item, bool highlighted);
	void ShowInsertMark(const ListViewItem *targetItem, InsertMarkPosition position);
	void RemoveInsertMark();
	int FindNextItemIndex(const POINT &pt) const;

	std::vector<ListViewColumnId> GetColumnsInVisibleOrderForTesting() const;
	std::optional<ColumnSortArrowDetails> GetColumnSortArrowDetailsForTesting() const;
	const ListViewItem *GetItemAtIndexForTesting(int index) const;
	std::wstring GetItemTextForTesting(int item, int subItem) const;
	int GetItemCountForTesting() const;

private:
	enum class ViewType
	{
		Icon,
		SmallIcon,
		List,
		Details,
		Tiles
	};

	class NoOpDelegate : public ListViewDelegate
	{
	public:
		bool OnItemRenamed(ListViewItem *item, const std::wstring &name) override;
		void OnItemsActivated(const std::vector<ListViewItem *> &items) override;
		void OnItemsRemoved(const std::vector<ListViewItem *> &items,
			RemoveMode removeMode) override;
		void OnItemsCopied(const std::vector<ListViewItem *> &items) override;
		void OnItemsCut(const std::vector<ListViewItem *> &items) override;
		void OnPaste(ListViewItem *lastSelectedItemOpt) override;
		void OnShowBackgroundContextMenu(const POINT &ptScreen) override;
		void OnShowItemContextMenu(const std::vector<ListViewItem *> &items,
			const POINT &ptScreen) override;
		void OnShowHeaderContextMenu(const POINT &ptScreen) override;
		void OnBeginDrag(const std::vector<ListViewItem *> &items) override;
	};

	class StateUpdateTarget
	{
	public:
		static StateUpdateTarget AllItems();
		static StateUpdateTarget Item(int index);

		bool IsAllItems() const;
		bool IsItem() const;
		std::optional<int> GetItemIndex() const;

	private:
		struct AllItemsTag
		{
		};

		StateUpdateTarget(AllItemsTag);
		StateUpdateTarget(int index);

		const std::variant<AllItemsTag, int> m_target;
	};

	void AddColumns();
	void AddColumn(ListViewColumnId columnId);
	void UpdateColumnOrdering();
	void AddItems();
	void AddItem(ListViewItem *item, int index);

	void OnItemUpdated(const ListViewItem *item);
	void OnItemMoved(ListViewItem *item, int newIndex);
	void ResetItemImage(const ListViewItem *item);
	void ResetItemColumns(const ListViewItem *item);
	void RemoveItem(const ListViewItem *item);
	void RemoveAllItems();

	void OnColumnVisibilityChanged(ListViewColumnId columnId, bool visible);
	void OnColumnMoved(ListViewColumnId columnId, int newVisibleIndex);
	void RemoveColumn(ListViewColumnId columnId);

	void OnSortOrderChanged();
	void SortItems();
	int SortItemsCallback(LPARAM lParam1, LPARAM lParam2);

	void UpdateHeaderSortArrow();
	void ClearHeaderSortArrow();
	void SetHeaderSortArrow();
	void UpdateHeaderItemFormat(ListViewColumnId columnId, int format, ItemStateOp stateOp);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnShowContextMenu(const POINT &ptScreen);

	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnGetDispInfo(NMLVDISPINFO *dispInfo);
	bool OnBeginLabelEdit(const NMLVDISPINFO *dispInfo);
	bool OnEndLabelEdit(const NMLVDISPINFO *dispInfo);
	void OnKeyDown(const NMLVKEYDOWN *keyDown);
	void WithNonEmptySelection(
		std::function<void(const std::vector<ListViewItem *> &selectedItems)> callback);
	void OnPaste();
	void OnHeaderItemChanged(const NMHEADER *changeInfo);
	void OnHeaderItemClick(const NMHEADER *header);
	void OnHeaderEndDrag(const NMHEADER *changeInfo);

	void UpdateItemState(const ListViewItem *item, UINT state, ItemStateOp stateOp);
	void UpdateAllItemStates(UINT state, ItemStateOp stateOp);
	void ApplyItemStateUpdates(const StateUpdateTarget &target, UINT state, ItemStateOp stateOp);

	ViewType GetViewType() const;
	ListViewColumnId GetColumnIdAtVisibleIndex(int index) const;
	ListViewColumnId GetColumnIdAtIndex(int index) const;
	int GetColumnIndex(ListViewColumnId columnId) const;
	std::optional<int> MaybeGetColumnIndex(ListViewColumnId columnId) const;
	ListViewItem *GetItemAtIndex(int index);
	const ListViewItem *GetItemAtIndex(int index) const;
	int GetItemIndex(const ListViewItem *item) const;
	HWND GetHeader() const;

	const HWND m_hwnd;
	ListViewModel *m_model = nullptr;
	NoOpDelegate m_noOpDelegate;
	ListViewDelegate *m_delegate = &m_noOpDelegate;
	const KeyboardState *const m_keyboardState;
	LabelEditHandlerFactory m_labelEditHandlerFactory;
	const ResourceLoader *const m_resourceLoader;
	std::optional<ListViewColumnId> m_previousSortColumnId;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
