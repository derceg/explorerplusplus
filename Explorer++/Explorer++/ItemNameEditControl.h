// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>

struct Accelerator;
class AcceleratorManager;

// Wraps an edit control used to rename a shell item. This adds a few pieces of functionality:
//
// 1. The last word can be erased using Ctrl + Backspace.
// 2. It's possible to cycle through the various parts of a filename (name, extension, full name)
// using F2.
// 3. Tab/Shift + Tab accelerators are removed, allowing the control to handle those if appropriate.
// For example, the listview has built-in handling to select the next/previous item when pressing
// Tab/Shift + Tab.
//
// The latter two items are only enabled if the active accelerator table is passed in. If the
// accelerator table isn't passed in, the F2/Tab/Shift + Tab keys will continue to function as
// accelerators.
//
// Some notes about the handling of Tab/Shift + Tab:
//
// When the items are removed from the accelerator table, they can be received as normal by
// controls. However, the edit control created by the listview has special handling for Tab/Shift +
// Tab. Although a WM_KEYDOWN event will be sent to the edit control for VK_TAB, the control will
// process it internally, meaning that the message won't be received by any subclasses. So it makes
// sense to simply rely on the control to handle Tab/Shift + Tab key presses.
class ItemNameEditControl
{
public:
	static ItemNameEditControl *CreateNew(HWND hwnd, AcceleratorManager *acceleratorManager,
		bool itemIsFile);

private:
	enum class RenameStage
	{
		Filename,
		Extension,
		Entire
	};

	ItemNameEditControl(HWND hwnd, AcceleratorManager *acceleratorManager, bool itemIsFile);
	~ItemNameEditControl();

	void UpdateAcceleratorTable();
	void RemoveAcceleratorFromTable(std::vector<ACCEL> &accelerators,
		const std::vector<Accelerator> &itemsToRemove);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnEMSetSel(WPARAM &wParam, LPARAM &lParam);
	bool OnKeyDown(HWND hwnd, UINT key);
	int GetExtensionIndex(HWND hwnd);
	void ErasePreviousWordOrSelectedText(HWND hwnd);
	bool SelectPreviousWord(HWND hwnd);
	bool OnChar(TCHAR character);

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	AcceleratorManager *const m_acceleratorManager;
	std::vector<ACCEL> m_originalAccelerators;

	bool m_itemIsFile;
	RenameStage m_renameStage;
	bool m_beginRename;
};
