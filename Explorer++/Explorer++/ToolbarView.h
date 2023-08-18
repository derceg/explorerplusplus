// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/signals2.hpp>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct MouseEvent
{
	MouseEvent(POINT ptClient, bool shiftKey, bool ctrlKey) :
		ptClient(ptClient),
		shiftKey(shiftKey),
		ctrlKey(ctrlKey)
	{
	}

	POINT ptClient;
	bool shiftKey;
	bool ctrlKey;
};

struct Config;
class ToolbarView;

class ToolbarButton
{
public:
	using ClickedCallback = std::function<void(const MouseEvent &event)>;
	using RightClickedCallback = std::function<void(const MouseEvent &event)>;
	using MiddleClickedCallback = std::function<void(const MouseEvent &event)>;
	using DragStartedCallback = std::function<void()>;

	ToolbarButton(ClickedCallback clickedCallback);

	virtual ~ToolbarButton() = default;

	ToolbarButton(const ToolbarButton &) = delete;
	ToolbarButton(ToolbarButton &&) = delete;
	ToolbarButton &operator=(const ToolbarButton &) = delete;
	ToolbarButton &operator=(ToolbarButton &&) = delete;

	void SetParent(ToolbarView *parent);

	virtual std::wstring GetText() const = 0;
	virtual std::wstring GetTooltipText() const = 0;
	virtual std::optional<int> GetImageIndex() const = 0;

	virtual bool ShouldSuppressClick() const;

	void SetChecked(bool checked);
	bool GetChecked() const;

	void SetRightClickedCallback(RightClickedCallback rightClickedCallback);
	void SetMiddleClickedCallback(MiddleClickedCallback middleClickedCallback);
	void SetDragStartedCallback(DragStartedCallback dragStartedCallback);

	void OnClicked(const MouseEvent &event);
	void OnRightClicked(const MouseEvent &event);
	void OnMiddleClicked(const MouseEvent &event);
	void OnDragStarted();

protected:
	void NotifyParentOfUpdate() const;

private:
	ToolbarView *m_parent = nullptr;

	ClickedCallback m_clickedCallback;
	RightClickedCallback m_rightClickedCallback;
	MiddleClickedCallback m_middleClickedCallback;
	DragStartedCallback m_dragStartedCallback;

	bool m_checked = false;
};

// Menu buttons could use the BTNS_DROPDOWN style and show the menu in response to TBN_DROPDOWN.
// That would have the advantage that the highlight state of the button would be automatically
// maintained and the button would act like a toggle (i.e. clicking it while it was showing a menu
// would simply result in the menu being closed).
// However, TBN_DROPDOWN is sent as soon as the left mouse button goes down. That's a problem, since
// the menu shown would then block the message loop, which would make it impossible to drag the
// button at all. That is, to start a drag, you need to keep processing mouse messages, to determine
// whether the mouse has moved a sufficient distance while the mouse is down. You can't do that
// while a menu is being shown.
// That means the menu needs to be shown on the button up event, but TBN_DROPDOWN doesn't work that
// way, which is why the functionality is manually replicated here instead.
class ToolbarMenuButton : public ToolbarButton
{
public:
	ToolbarMenuButton(ClickedCallback clickedCallback);

	bool ShouldSuppressClick() const override;

private:
	static constexpr std::chrono::milliseconds MINIMUM_TIME_BETWEEN_CLICKS{ 100 };

	void OnMenuButtonClicked(const MouseEvent &event);

	ClickedCallback m_clickedCallback;
	std::chrono::steady_clock::time_point m_menuLastClosedTime;
};

// Designed to act as a base class for a toolbar control. Doesn't actually add any buttons on its
// own.
class ToolbarView
{
public:
	using ToolbarUpdatedSignal = boost::signals2::signal<void()>;
	using ToolbarSizeChangedSignal = boost::signals2::signal<void()>;
	using WindowDestroyedSignal = boost::signals2::signal<void()>;

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

	ToolbarView(const ToolbarView &) = delete;
	ToolbarView(ToolbarView &&) = delete;
	ToolbarView &operator=(const ToolbarView &) = delete;
	ToolbarView &operator=(ToolbarView &&) = delete;

	HWND GetHWND() const;

	void AddButton(std::unique_ptr<ToolbarButton> button, size_t index);
	void UpdateButton(const ToolbarButton *button);
	void UpdateButton(size_t index);
	void RemoveButton(size_t index);
	ToolbarButton *GetButton(size_t index) const;

	DropLocation GetDropLocation(const POINT &ptScreen);
	void ShowInsertMark(size_t index);
	void RemoveInsertMark();

	std::optional<size_t> MaybeGetIndexOfButtonAtPoint(const POINT &pt);
	bool IsButtonVisible(size_t index) const;
	RECT GetButtonRect(size_t index) const;
	size_t FindNextButtonIndex(const POINT &ptClient) const;
	void SetHotItem(size_t index);

	boost::signals2::connection AddToolbarSizeUpdatedObserver(
		const ToolbarSizeChangedSignal::slot_type &observer);
	boost::signals2::connection AddWindowDestroyedObserver(
		const WindowDestroyedSignal::slot_type &observer);

protected:
	ToolbarView(HWND parent, DWORD style, DWORD extendedStyle, const Config *config);
	virtual ~ToolbarView() = default;

	void SetupSmallShellImageList();

	const HWND m_hwnd;

private:
	BYTE GetToolbarButtonState(const ToolbarButton *button);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool OnLeftButtonDown(const POINT &pt);
	void OnMouseMove(int keys, const POINT &pt);
	void OnLeftButtonUp();
	bool OnClick(const NMMOUSE *mouseInfo);
	bool OnMiddleButtonUp(const POINT *pt, UINT keysDown);
	bool OnRightClick(const NMMOUSE *mouseInfo);
	bool OnGetInfoTip(NMTBGETINFOTIP *infoTip);

	ToolbarButton *MaybeGetButtonAtPoint(const POINT &pt);
	size_t TransformCommandToIndex(int command);
	ToolbarButton *GetButtonFromIndex(size_t index);

	void OnFontOrDpiUpdated();

	void OnNcDestroy();

	std::vector<std::unique_ptr<ToolbarButton>> m_buttons;

	// Items are largely referred to by their indexes, but there are certain cases where the command
	// ID has to be used (e.g. when processing a toolbar notification). Since command IDs and
	// indexes are numeric, it's easy to mix them up, especially if they tend to overlap. Starting
	// the command IDs at an offset means that any issues are likely to show up sooner (since a
	// command ID won't be a valid index and vice versa).
	int m_idCounter = 1000;

	MainFontSetter m_fontSetter;
	std::unique_ptr<MainFontSetter> m_tooltipFontSetter;

	// Drag and drop
	std::optional<POINT> m_leftButtonDownPoint;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;

	ToolbarSizeChangedSignal m_toolbarSizeUpdatedSignal;
	WindowDestroyedSignal m_windowDestroyedSignal;
};
