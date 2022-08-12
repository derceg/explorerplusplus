// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseWindow.h"
#include <wil/resource.h>

struct Accelerator;
__interface IExplorerplusplus;

// Some notes about the handling of Tab/Shift + Tab:
//
// When the items are removed from the accelerator table, they can be received as normal by
// controls. However, the edit control created by the listview has special handling for Tab/Shift +
// Tab. Although a WM_KEYDOWN event will be sent to the edit control for VK_TAB, the control will
// process it internally, meaning that the message won't be received by any subclasses. So it makes
// sense to simply rely on the control to handle Tab/Shift + Tab key presses.
class ListViewEdit : BaseWindow
{
public:
	static ListViewEdit *CreateNew(HWND hwnd, HACCEL *acceleratorTable, int ItemIndex,
		IExplorerplusplus *pexpp);

protected:
	void OnEMSetSel(WPARAM &wParam, LPARAM &lParam) override;

	void OnKeyDown(HWND hwnd, UINT key, BOOL down, int repeat, UINT flags) override;

private:
	enum class RenameStage
	{
		Filename,
		Extension,
		Entire
	};

	ListViewEdit(HWND hwnd, HACCEL *acceleratorTable, int ItemIndex, IExplorerplusplus *pexpp);
	~ListViewEdit();

	void UpdateAcceleratorTable();
	void RemoveAcceleratorFromTable(std::vector<ACCEL> &accelerators,
		const std::vector<Accelerator> &itemsToRemove);

	int GetExtensionIndex(HWND hwnd);

	HACCEL *m_acceleratorTable;
	wil::unique_haccel m_updatedAcceleratorTable;
	HACCEL m_originalAcceleratorTable;

	IExplorerplusplus *m_pexpp;

	int m_itemIndex;
	RenameStage m_renameStage;
	bool m_beginRename;
};