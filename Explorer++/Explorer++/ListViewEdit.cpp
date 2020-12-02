// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewEdit.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

ListViewEdit *ListViewEdit::CreateNew(HWND hwnd, int ItemIndex, IExplorerplusplus *pexpp)
{
	return new ListViewEdit(hwnd, ItemIndex, pexpp);
}

ListViewEdit::ListViewEdit(HWND hwnd, int ItemIndex, IExplorerplusplus *pexpp) :
	BaseWindow(hwnd),
	m_ItemIndex(ItemIndex),
	m_pexpp(pexpp),
	m_RenameStage(RenameStage::Filename),
	m_BeginRename(true)
{
}

void ListViewEdit::OnEMSetSel(WPARAM &wParam, LPARAM &lParam)
{
	/* When editing an item, the listview control
	will first deselect, then select all text. If
	an item has been put into edit mode, and the
	listview attempts to select all text, modify the
	message so that only text up to the extension
	(if any) is selected. */
	if (m_BeginRename && wParam == 0 && lParam == -1)
	{
		int index = GetExtensionIndex();

		if (index != -1)
		{
			wParam = 0;
			lParam = index;
		}

		m_BeginRename = false;
	}
}

INT_PTR ListViewEdit::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (uMsg)
	{
	case WM_APP_KEYDOWN:
		switch (wParam)
		{
		case VK_F2:
		{
			int index = GetExtensionIndex();

			if (index != -1)
			{
				switch (m_RenameStage)
				{
				case RenameStage::Filename:
					SendMessage(m_hwnd, EM_SETSEL, index + 1, -1);
					m_RenameStage = RenameStage::Extension;
					break;

				case RenameStage::Extension:
					SendMessage(m_hwnd, EM_SETSEL, 0, -1);
					m_RenameStage = RenameStage::Entire;
					break;

				case RenameStage::Entire:
					SendMessage(m_hwnd, EM_SETSEL, 0, index);
					m_RenameStage = RenameStage::Filename;
					break;

				default:
					assert(false);
					break;
				}
			}
		}
		break;

		case VK_TAB:
		{
			HWND hListView = GetParent(m_hwnd);

			int iSel = ListView_GetNextItem(hListView, -1, LVNI_ALL | LVNI_SELECTED);
			ListViewHelper::SelectItem(hListView, iSel, FALSE);

			int nItems = ListView_GetItemCount(hListView);

			int iNewSel;

			if (IsKeyDown(VK_SHIFT))
			{
				if (iSel == 0)
				{
					iNewSel = nItems - 1;
				}
				else
				{
					iNewSel = iSel - 1;
				}
			}
			else
			{
				if (iSel == (nItems - 1))
				{
					iNewSel = 0;
				}
				else
				{
					iNewSel = iSel + 1;
				}
			}

			ListView_EditLabel(hListView, iNewSel);
		}
		break;
		}
		break;
	}

	return 0;
}

int ListViewEdit::GetExtensionIndex()
{
	std::wstring fileName = GetWindowString(m_hwnd);

	DWORD dwAttributes =
		m_pexpp->GetActiveShellBrowser()->GetItemFileFindData(m_ItemIndex).dwFileAttributes;

	int index = -1;

	if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		for (size_t i = fileName.size() - 1; i >= 0; i--)
		{
			if (fileName[i] == '.')
			{
				index = static_cast<int>(i);
				break;
			}
		}
	}

	return index;
}