// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewEdit.h"
#include "Accelerator.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

ListViewEdit *ListViewEdit::CreateNew(HWND hwnd, HACCEL *acceleratorTable, int ItemIndex,
	IExplorerplusplus *pexpp)
{
	return new ListViewEdit(hwnd, acceleratorTable, ItemIndex, pexpp);
}

ListViewEdit::ListViewEdit(HWND hwnd, HACCEL *acceleratorTable, int ItemIndex,
	IExplorerplusplus *pexpp) :
	BaseWindow(hwnd),
	m_acceleratorTable(acceleratorTable),
	m_itemIndex(ItemIndex),
	m_pexpp(pexpp),
	m_renameStage(RenameStage::Filename),
	m_beginRename(true)
{
	UpdateAcceleratorTable();
}

void ListViewEdit::UpdateAcceleratorTable()
{
	m_originalAcceleratorTable = *m_acceleratorTable;

	int numAccelerators = CopyAcceleratorTable(*m_acceleratorTable, nullptr, 0);

	std::vector<ACCEL> accelerators(numAccelerators);
	CopyAcceleratorTable(*m_acceleratorTable, &accelerators[0],
		static_cast<int>(accelerators.size()));

	// F2 will be handled below and used to cycle the selection between the filename and extension.
	// Tab/Shift + Tab will be used by the listview edit control. It has handling to move to the
	// next/previous item. However, for that functionality to be invoked, the control has to be able
	// to receive Tab/Shift + Tab key presses. Therefore, those items need to be removed from the
	// accelerator table as well.
	// Once the keys have been removed from the accelerator table, they can be received as normal.
	RemoveAcceleratorFromTable(accelerators,
		{ { FVIRTKEY, VK_F2 }, { FVIRTKEY, VK_TAB }, { FVIRTKEY | FSHIFT, VK_TAB } });

	m_updatedAcceleratorTable.reset(
		CreateAcceleratorTable(&accelerators[0], static_cast<int>(accelerators.size())));

	if (!m_updatedAcceleratorTable)
	{
		return;
	}

	*m_acceleratorTable = m_updatedAcceleratorTable.get();
}

void ListViewEdit::RemoveAcceleratorFromTable(std::vector<ACCEL> &accelerators,
	const std::vector<Accelerator> &itemsToRemove)
{
	for (auto &item : itemsToRemove)
	{
		auto itr = std::find_if(accelerators.begin(), accelerators.end(),
			[&item](const ACCEL &accel)
			{
				return (accel.fVirt & ~FNOINVERT) == item.modifiers && accel.key == item.key;
			});

		if (itr != accelerators.end())
		{
			accelerators.erase(itr);
		}
	}
}

ListViewEdit::~ListViewEdit()
{
	*m_acceleratorTable = m_originalAcceleratorTable;
}

void ListViewEdit::OnEMSetSel(WPARAM &wParam, LPARAM &lParam)
{
	// When editing an item, the listview control will first deselect, then select all text. If an
	// item has been put into edit mode, and the listview attempts to select all text, modify the
	// message so that only text up to the extension (if any) is selected.
	if (m_beginRename && wParam == 0 && lParam == -1)
	{
		int index = GetExtensionIndex(m_hwnd);

		if (index != -1)
		{
			wParam = 0;
			lParam = index;
		}

		m_beginRename = false;
	}
}

void ListViewEdit::OnKeyDown(HWND hwnd, UINT key, BOOL down, int repeat, UINT flags)
{
	UNREFERENCED_PARAMETER(down);
	UNREFERENCED_PARAMETER(repeat);
	UNREFERENCED_PARAMETER(flags);

	switch (key)
	{
	case VK_F2:
	{
		int index = GetExtensionIndex(hwnd);

		if (index == -1)
		{
			break;
		}

		switch (m_renameStage)
		{
		case RenameStage::Filename:
			SendMessage(hwnd, EM_SETSEL, index + 1, -1);
			m_renameStage = RenameStage::Extension;
			break;

		case RenameStage::Extension:
			SendMessage(hwnd, EM_SETSEL, 0, -1);
			m_renameStage = RenameStage::Entire;
			break;

		case RenameStage::Entire:
			SendMessage(hwnd, EM_SETSEL, 0, index);
			m_renameStage = RenameStage::Filename;
			break;

		default:
			assert(false);
			break;
		}
	}
	break;
	}
}

int ListViewEdit::GetExtensionIndex(HWND hwnd)
{
	std::wstring fileName = GetWindowString(hwnd);

	DWORD dwAttributes =
		m_pexpp->GetActiveShellBrowser()->GetItemFileFindData(m_itemIndex).dwFileAttributes;

	int index = -1;

	if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		auto position = fileName.find_last_of('.');

		if (position != std::wstring::npos)
		{
			index = static_cast<int>(position);
		}
	}

	return index;
}