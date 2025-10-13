// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>
#include <optional>
#include <vector>

struct Accelerator;
class AcceleratorManager;
class WindowSubclass;

// Wraps an edit control used when editing a treeview/listview label. This class adds a few pieces
// of functionality:
//
// 1. The last word can be erased using Ctrl + Backspace.
// 2. It's possible to cycle through the various parts of a filename (name, extension, full name)
//    using F2. Only enabled if itemIsFile is set to true.
// 3. Tab/Shift+Tab accelerators are removed, allowing the control to handle those if appropriate.
//    For example, the listview has built-in handling to select the next/previous item when pressing
//    Tab/Shift+Tab.
//
// Some notes about the handling of Tab/Shift+Tab:
//
// When the items are removed from the accelerator table, they can be received as normal by
// controls. However, the edit control created by the listview has special handling for
// Tab/Shift+Tab. Although a WM_KEYDOWN event will be sent to the edit control for VK_TAB, the
// control will process it internally, meaning that the message won't be received by any subclasses.
// So it makes sense to simply rely on the control to handle Tab/Shift+Tab key presses.
class LabelEditHandler
{
public:
	// Indicates the class is going to be used for an edit control that's ultimately a child of the
	// main window. In this case, an AcceleratorManager instance should be provided, so that the
	// relevant accelerators can be temporarily disabled and the keys can be used by this class.
	static LabelEditHandler *CreateForMainWindow(HWND hwnd, AcceleratorManager *acceleratorManager,
		bool itemIsFile);

	// Indicates the class is going to be used for an edit control that's ultimately a child of a
	// dialog. In this case, no AcceleratorManager is needed, since accelerators aren't active in
	// dialogs.
	static LabelEditHandler *CreateForDialog(HWND hwnd, bool itemIsFile);

	// In tests, accelerators aren't relevant, so no AcceleratorManager instance is needed.
	static LabelEditHandler *CreateForTest(HWND hwnd, bool itemIsFile);

private:
	enum class RenameStage
	{
		Filename,
		Extension,
		Entire
	};

	LabelEditHandler(HWND hwnd, AcceleratorManager *acceleratorManagerOpt, bool itemIsFile);
	~LabelEditHandler();

	void UpdateAcceleratorTable();
	bool IsParentListView() const;
	void RemoveAcceleratorFromTable(std::vector<ACCEL> &accelerators,
		const std::vector<Accelerator> &itemsToRemove);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnEMSetSel(WPARAM &wParam, LPARAM &lParam);
	bool OnKeyDown(HWND hwnd, UINT key);
	std::optional<int> MaybeGetExtensionIndex(HWND hwnd);
	void ErasePreviousWordOrSelectedText(HWND hwnd);
	bool SelectPreviousWord(HWND hwnd);
	bool OnChar(TCHAR character);

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	AcceleratorManager *const m_acceleratorManagerOpt;
	std::vector<ACCEL> m_originalAccelerators;

	bool m_itemIsFile;
	RenameStage m_renameStage;
	bool m_initialSelectionSet = false;
};
