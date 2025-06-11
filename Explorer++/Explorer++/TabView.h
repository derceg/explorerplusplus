// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "MouseEvent.h"
#include "../Helper/SignalWrapper.h"
#include <boost/core/noncopyable.hpp>
#include <CommCtrl.h>
#include <memory>
#include <optional>
#include <vector>

struct Config;
class TabView;
class TabViewDelegate;
class WindowSubclass;

// Represents an individual tab within the view.
class TabViewItem
{
public:
	virtual ~TabViewItem() = default;

	void SetParent(TabView *parent);

	virtual std::wstring GetText() const = 0;
	virtual std::optional<int> GetIconIndex() const = 0;

	void SetDoubleClickedCallback(MouseEventCallback doubleClickedCallback);
	void SetMiddleClickedCallback(MouseEventCallback middleClickedCallback);

	void OnDoubleClicked(const MouseEvent &event);
	void OnMiddleClicked(const MouseEvent &event);

protected:
	void NotifyParentOfUpdate() const;

private:
	TabView *m_parent = nullptr;

	MouseEventCallback m_doubleClickedCallback;
	MouseEventCallback m_middleClickedCallback;
};

// Designed to act as a base class for a tab control. Doesn't actually add any tabs on its own.
class TabView : private boost::noncopyable
{
public:
	enum class ScrollDirection
	{
		Left,
		Right
	};

	HWND GetHWND() const;
	void SetDelegate(TabViewDelegate *delegate);

	int AddTab(std::unique_ptr<TabViewItem> tabItem, int index);
	void UpdateTab(const TabViewItem *tabItem);
	void RemoveTab(int index);
	TabViewItem *GetTabAtIndex(int index) const;
	int GetNumTabs() const;

	void SetImageList(HIMAGELIST imageList);
	void Scroll(ScrollDirection direction);

	// Signals
	SignalWrapper<TabView, void()> sizeUpdatedSignal;
	SignalWrapper<TabView, void()> windowDestroyedSignal;

protected:
	TabView(HWND parent, DWORD style, const Config *config);
	virtual ~TabView();

private:
	// When dragging a tab, the tab will typically be moved if the cursor moves outside the
	// left/right edges of the tab.
	//
	// However, when the tab is swapped with an adjacent tab, the cursor can immediately end up
	// outside of the bounds of the tab. As the cursor is then moved, the two tabs could repeatedly
	// swap positions.
	//
	// To stop that, the left/right edge can be anchored to the previous tab position when dragging.
	// For example, if there are two tabs:
	//
	//   +-----------------------+-------+
	//   |         Tab A         | Tab B |
	//   +-----------------------+-------+
	//
	// and the second tab is dragged to the first position:
	//
	//   +-------+-----------------------+
	//   | Tab B |         Tab A         |
	//   +-------+-----------------------+
	//
	// the right edge of the tab (for drag purposes) will be considered anchored to the original tab
	// position:
	//
	//   +-------+-----------------------+
	//   | Tab B |         Tab A         |
	//   +-------+-----------------------+
	//   ^                       ^
	//   |                       |
	//   |                       +-- anchored "right" edge (original left edge of Tab B)
	//   +-- actual left edge of Tab B
	//
	// That will prevent the two tabs from repeatedly swapping positions as the cursor is moved
	// between them. That's because the original tab will only be moved back to the right once the
	// cursor hits the anchored right edge. When the cursor is in between the right edge of the tab
	// and the anchored right edge, the tab will remain in the same position.
	enum class TabDragAnchor
	{
		// Indicates that neither of the left/right edges are anchored (i.e. the bounds of the tab
		// correspond to the drag bounds).
		None,

		// Indicates that the left drag edge is anchored to the right edge of the previous position.
		// Used when dragging a tab to the right.
		Left,

		// Indicates that the right drag edge is anchored to the left edge of the previous position.
		// Used when dragging a tab to the left.
		Right
	};

	struct TabDragBounds
	{
		// As noted above, these typically correspond to the bounds of the tab, but one of the
		// bounds may be anchored to the previous position.
		int left;
		int right;
	};

	struct TabDragState
	{
		TabDragAnchor anchor;
	};

	int GetIndexOfTabItem(const TabViewItem *tabItem) const;
	std::optional<int> MaybeGetTabIconIndex(int index) const;
	void MaybeRemoveIcon(int iconIndex);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMouseWheel(HWND hwnd, int xPos, int yPos, int delta, UINT keys);
	void OnLeftButtonDown(const POINT &pt);
	void OnLeftButtonUp();
	void OnMouseMove(const POINT &pt);
	TabDragBounds GetTabBoundsForDrag(int tabIndex, TabDragAnchor anchor) const;
	void OnCaptureChanged(HWND target);
	void OnLeftButtonDoubleClick(const POINT &pt, UINT keysDown);
	void OnMiddleButtonDown(const POINT &pt, UINT keysDown);
	void OnMiddleButtonUp(const POINT &pt, UINT keysDown);
	int GetSelectedIndex() const;
	bool IsValidIndex(int index) const;
	RECT GetTabRect(int index) const;
	std::optional<int> MaybeGetIndexOfTabAtPoint(const POINT &pt) const;
	void OnFontUpdated();
	void OnNcDestroy();

	const HWND m_hwnd;
	std::vector<std::unique_ptr<TabViewItem>> m_tabs;
	TabViewDelegate *m_delegate = nullptr;
	MainFontSetter m_fontSetter;
	std::unique_ptr<MainFontSetter> m_tooltipFontSetter;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	// A value will only be assigned here whilst a tab is being dragged.
	std::optional<TabDragState> m_tabDragState;

	// This will only be set in between the point where the middle mouse button is pressed and the
	// point where the button is released.
	std::optional<int> m_middleClickItemIndex;
};
