// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WindowSubclassWrapper.h"
#include <boost/signals2.hpp>
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

class ToolbarButton
{
public:
	using ClickedCallback = std::function<void(const MouseEvent &event)>;
	using RightClickedCallback = std::function<void(const MouseEvent &event)>;
	using MiddleClickedCallback = std::function<void(const MouseEvent &event)>;

	ToolbarButton(ClickedCallback clickedCallback);

	virtual ~ToolbarButton() = default;

	ToolbarButton(const ToolbarButton &) = delete;
	ToolbarButton(ToolbarButton &&) = delete;
	ToolbarButton &operator=(const ToolbarButton &) = delete;
	ToolbarButton &operator=(ToolbarButton &&) = delete;

	virtual std::wstring GetText() const = 0;
	virtual std::wstring GetTooltipText() const = 0;
	virtual std::optional<int> GetImageIndex() const = 0;

	void SetRightClickedCallback(RightClickedCallback rightClickedCallback);
	void SetMiddleClickedCallback(MiddleClickedCallback middleClickedCallback);

	void OnClicked(const MouseEvent &event);
	void OnRightClicked(const MouseEvent &event);
	void OnMiddleClicked(const MouseEvent &event);

private:
	ClickedCallback m_clickedCallback;
	RightClickedCallback m_rightClickedCallback;
	MiddleClickedCallback m_middleClickedCallback;
};

// Designed to act as a base class for a toolbar control. Doesn't actually add any buttons on its
// own.
class ToolbarView
{
public:
	using ToolbarUpdatedSignal = boost::signals2::signal<void()>;
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
	void UpdateButton(size_t index);
	void RemoveButton(size_t index);

	DropLocation GetDropLocation(const POINT &ptScreen);

	boost::signals2::connection AddToolbarUpdatedObserver(
		const ToolbarUpdatedSignal::slot_type &observer);
	boost::signals2::connection AddWindowDestroyedObserver(
		const WindowDestroyedSignal::slot_type &observer);

protected:
	ToolbarView(HWND parent, DWORD style, DWORD extendedStyle);
	virtual ~ToolbarView() = default;

	void SetupSmallShellImageList();

	const HWND m_hwnd;

private:
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool OnClick(const NMMOUSE *nmm);
	bool OnMiddleButtonUp(const POINT *pt, UINT keysDown);
	bool OnRightClick(const NMMOUSE *nmm);
	bool OnGetInfoTip(NMTBGETINFOTIP *infoTip);

	ToolbarButton *MaybeGetButtonFromIndex(size_t index);

	void OnNcDestroy();

	std::vector<std::unique_ptr<ToolbarButton>> m_buttons;
	int m_idCounter = 0;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;

	ToolbarUpdatedSignal m_toolbarUpdatedSignal;
	WindowDestroyedSignal m_windowDestroyedSignal;
};
