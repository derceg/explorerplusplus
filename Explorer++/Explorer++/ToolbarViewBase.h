// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "MouseEvent.h"
#include "RebarBandView.h"
#include "ViewBase.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <chrono>
#include <functional>
#include <optional>
#include <string>

struct Config;
class MenuBase;
class MenuHelpTextHost;
class MenuView;
class ToolbarViewBase;

class ToolbarButton
{
public:
	using MenuFactory = std::function<std::unique_ptr<MenuBase>(MenuView *menuView)>;
	using DragStartedCallback = std::function<void()>;

	ToolbarButton(MouseEventCallback clickedCallback);

	virtual ~ToolbarButton() = default;

	ToolbarButton(const ToolbarButton &) = delete;
	ToolbarButton(ToolbarButton &&) = delete;
	ToolbarButton &operator=(const ToolbarButton &) = delete;
	ToolbarButton &operator=(ToolbarButton &&) = delete;

	void SetParent(ToolbarViewBase *parent);

	virtual std::wstring GetText() const = 0;
	virtual bool ShouldShowText() const;
	virtual std::wstring GetTooltipText() const;
	virtual std::optional<int> GetImageIndex() const = 0;
	virtual bool IsDropDownButton() const;

	// If the button is a dropdown button, this method should return a factory that can be used to
	// create the dropdown menu. When the dropdown button is clicked, the menu created via this
	// factory will be shown.
	//
	// Note that it may not always be possible for a dropdown button to show a menu (if the menu
	// would be empty). This method can then return an empty result, though the button should be
	// disabled in that case.
	virtual MenuFactory MaybeGetMenuFactory() const;

	virtual bool ShouldSuppressClick() const;

	virtual bool IsEnabled() const;
	virtual bool IsChecked() const;

	void SetRightClickedCallback(MouseEventCallback rightClickedCallback);
	void SetMiddleClickedCallback(MouseEventCallback middleClickedCallback);
	void SetDragStartedCallback(DragStartedCallback dragStartedCallback);

	void OnClicked(const MouseEvent &event);
	void OnRightClicked(const MouseEvent &event);
	void OnMiddleClicked(const MouseEvent &event);
	void OnDragStarted();

	WeakPtr<ToolbarButton> GetWeakPtr();

protected:
	void NotifyParentOfUpdate() const;

private:
	ToolbarViewBase *m_parent = nullptr;

	MouseEventCallback m_clickedCallback;
	MouseEventCallback m_rightClickedCallback;
	MouseEventCallback m_middleClickedCallback;
	DragStartedCallback m_dragStartedCallback;

	WeakPtrFactory<ToolbarButton> m_weakPtrFactory{ this };
};

// Menu buttons could use the BTNS_DROPDOWN style and show the menu in response to TBN_DROPDOWN.
// That would have the advantage that the highlight state of the button would be automatically
// maintained and the button would act like a toggle (i.e. clicking it while it was showing a menu
// would simply result in the menu being closed).
//
// However, TBN_DROPDOWN is sent as soon as the left mouse button goes down. That's a problem, since
// the menu shown would then block the message loop, which would make it impossible to drag the
// button at all. That is, to start a drag, you need to keep processing mouse messages, to determine
// whether the mouse has moved a sufficient distance while the mouse is down. You can't do that
// while a menu is being shown.
//
// That means the menu needs to be shown on the button up event, but TBN_DROPDOWN doesn't work that
// way, which is why the functionality is manually replicated here instead.
class ToolbarMenuButton : public ToolbarButton
{
public:
	ToolbarMenuButton(MouseEventCallback clickedCallback);

	bool ShouldSuppressClick() const override;
	bool IsChecked() const override;

private:
	static constexpr std::chrono::milliseconds MINIMUM_TIME_BETWEEN_CLICKS{ 100 };

	void OnMenuButtonClicked(const MouseEvent &event);
	void SetMenuOpen(bool open);

	MouseEventCallback m_clickedCallback;
	std::chrono::steady_clock::time_point m_menuLastClosedTime;
	bool m_menuOpen = false;
};

// Represents either a toolbar button or a separator.
class ToolbarItem
{
public:
	static ToolbarItem Button(ToolbarButton *button);
	static ToolbarItem Separator();

	bool IsButton() const;
	bool IsSeparator() const;
	ToolbarButton *GetButton() const;

	bool operator==(const ToolbarItem &other) const = default;

private:
	ToolbarItem(ToolbarButton *button);

	ToolbarButton *m_button = nullptr;
};

// Designed to act as a base class for a toolbar control. Doesn't actually add any buttons on its
// own.
class ToolbarViewBase : public ViewBase, public RebarBandView
{
public:
	struct DropLocation
	{
		DropLocation(bool onItem, size_t index) : onItem(onItem), index(index)
		{
		}

		bool onItem;

		// If onItem is set, the drop location is over an item and this will be the index of the
		// item; otherwise it will be the index of the next item (which may be one past the last
		// item that's actually shown).
		size_t index;
	};

	ToolbarViewBase(const ToolbarViewBase &) = delete;
	ToolbarViewBase(ToolbarViewBase &&) = delete;
	ToolbarViewBase &operator=(const ToolbarViewBase &) = delete;
	ToolbarViewBase &operator=(ToolbarViewBase &&) = delete;

	// RebarBandView
	HWND GetHWND() const override;
	int GetHeight() const override;
	std::optional<int> MaybeGetIdealWidth() const override;

	void SetupSmallShellImageList();
	void SetImageList(HIMAGELIST imageList);
	HIMAGELIST MaybeGetImageList() const;

	void UpdateButton(const ToolbarButton *button);

	DropLocation GetDropLocation(const POINT &ptScreen) const;
	void ShowInsertMark(size_t index);
	void RemoveInsertMark();

	std::optional<size_t> MaybeGetIndexOfButtonAtPoint(const POINT &ptClient) const;
	std::vector<ToolbarItem> GetItems() const;
	std::vector<ToolbarItem> GetOverflowItems() const;
	bool IsButtonVisible(const ToolbarButton *button) const;
	RECT GetButtonRect(const ToolbarButton *button) const;
	size_t FindNextButtonIndex(const POINT &ptClient) const;
	void SetHotItem(const ToolbarButton *button);

protected:
	// Items are largely referred to by their indexes, but there are certain cases where the command
	// ID has to be used (e.g. when processing a toolbar notification). Since command IDs and
	// indexes are numeric, it's easy to mix them up, especially if they tend to overlap. Starting
	// the command IDs at an offset means that any issues are likely to show up sooner (since a
	// command ID won't be a valid index and vice versa).
	static constexpr int INITIAL_ID = 1000;

	struct ButtonEntry
	{
		int id;
		std::unique_ptr<ToolbarButton> button;
	};

	ToolbarViewBase(HWND parent, DWORD style, DWORD extendedStyle,
		MenuHelpTextHost *menuHelpTextHost, const Config *config);

	void AddButton(int id, const ToolbarButton *button, size_t index);
	TBBUTTON BuildNativeButtonStruct(int id, const ToolbarButton *button) const;
	void AddSeparator(size_t index);

	size_t GetNumButtons() const;

private:
	virtual ToolbarButton *GetButtonById(int id) const = 0;

	static BYTE GetToolbarButtonState(const ToolbarButton *button);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual std::optional<LRESULT> MaybeHandleNotification(NMHDR *notification);

	bool OnLeftButtonDown(const POINT &ptClient);
	void OnMouseMove(int keys, const POINT &ptClient);
	void OnLeftButtonUp();
	bool OnClick(const NMMOUSE *mouseInfo);
	bool OnMiddleButtonUp(int keys, const POINT &ptClient);
	bool OnRightClick(const NMMOUSE *mouseInfo);
	bool OnGetInfoTip(NMTBGETINFOTIP *infoTip);
	LRESULT OnDropDown(const NMTOOLBAR *data);

	ToolbarButton *MaybeGetButtonAtPoint(const POINT &ptClient) const;
	std::optional<size_t> MaybeGetButtonIndex(const ToolbarButton *button) const;
	ToolbarItem GetItemAtIndex(size_t index) const;
	bool IsItemVisible(size_t index) const;
	RECT GetItemRect(size_t index) const;

	void OnFontOrDpiUpdated();
	void RefreshAfterSizeChange();

	MenuHelpTextHost *const m_menuHelpTextHost;

	MainFontSetter m_fontSetter;
	std::unique_ptr<MainFontSetter> m_tooltipFontSetter;

	// Drag and drop
	std::optional<POINT> m_leftButtonDownPoint;
};
