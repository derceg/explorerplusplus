// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ItemNameEditControl.h"
#include "Accelerator.h"
#include "AcceleratorManager.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowHelper.h"
#include <array>

ItemNameEditControl *ItemNameEditControl::CreateNew(HWND hwnd,
	AcceleratorManager *acceleratorManager, bool itemIsFile)
{
	return new ItemNameEditControl(hwnd, acceleratorManager, itemIsFile);
}

ItemNameEditControl::ItemNameEditControl(HWND hwnd, AcceleratorManager *acceleratorManager,
	bool itemIsFile) :
	m_hwnd(hwnd),
	m_acceleratorManager(acceleratorManager),
	m_itemIsFile(itemIsFile),
	m_renameStage(RenameStage::Filename),
	m_beginRename(true)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&ItemNameEditControl::WndProc, this)));

	UpdateAcceleratorTable();
}

void ItemNameEditControl::UpdateAcceleratorTable()
{
	if (!m_acceleratorManager)
	{
		return;
	}

	m_originalAccelerators = m_acceleratorManager->GetAccelerators();
	auto accelerators = m_acceleratorManager->GetAccelerators();

	// F2 will be handled below and used to cycle the selection between the filename and extension.
	// Tab/Shift + Tab will be used by the listview edit control. It has handling to move to the
	// next/previous item. However, for that functionality to be invoked, the control has to be able
	// to receive Tab/Shift + Tab key presses. Therefore, those items need to be removed from the
	// accelerator table as well.
	// Once the keys have been removed from the accelerator table, they can be received as normal.
	RemoveAcceleratorFromTable(accelerators,
		{ { FVIRTKEY, VK_F2 }, { FVIRTKEY, VK_TAB }, { FVIRTKEY | FSHIFT, VK_TAB } });

	m_acceleratorManager->SetAccelerators(accelerators);
}

void ItemNameEditControl::RemoveAcceleratorFromTable(std::vector<ACCEL> &accelerators,
	const std::vector<Accelerator> &itemsToRemove)
{
	for (auto &item : itemsToRemove)
	{
		auto itr = std::find_if(accelerators.begin(), accelerators.end(),
			[&item](const ACCEL &accel)
			{ return (accel.fVirt & ~FNOINVERT) == item.modifiers && accel.key == item.key; });

		if (itr != accelerators.end())
		{
			accelerators.erase(itr);
		}
	}
}

ItemNameEditControl::~ItemNameEditControl()
{
	if (m_acceleratorManager)
	{
		m_acceleratorManager->SetAccelerators(m_originalAccelerators);
	}
}

LRESULT ItemNameEditControl::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case EM_SETSEL:
		OnEMSetSel(wParam, lParam);
		break;

	case WM_KEYDOWN:
		if (OnKeyDown(hwnd, static_cast<UINT>(wParam)))
		{
			return 0;
		}
		break;

	case WM_CHAR:
		if (OnChar(static_cast<TCHAR>(wParam)))
		{
			return 0;
		}
		break;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ItemNameEditControl::OnEMSetSel(WPARAM &wParam, LPARAM &lParam)
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

bool ItemNameEditControl::OnKeyDown(HWND hwnd, UINT key)
{
	switch (key)
	{
	case VK_BACK:
	{
		if (!IsKeyDown(VK_CONTROL))
		{
			return false;
		}

		ErasePreviousWordOrSelectedText(hwnd);
	}
	break;

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

	default:
		return false;
	}

	return true;
}

int ItemNameEditControl::GetExtensionIndex(HWND hwnd)
{
	std::wstring fileName = GetWindowString(hwnd);

	int index = -1;

	if (m_itemIsFile)
	{
		auto position = fileName.find_last_of('.');

		if (position != std::wstring::npos)
		{
			index = static_cast<int>(position);
		}
	}

	return index;
}

void ItemNameEditControl::ErasePreviousWordOrSelectedText(HWND hwnd)
{
	DWORD selectionStart = 0;
	DWORD selectionEnd = 0;
	SendMessage(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&selectionStart),
		reinterpret_cast<LPARAM>(&selectionEnd));

	if (selectionStart == selectionEnd)
	{
		// In this case, there's no current selection, so the previous word will be selected.
		[[maybe_unused]] bool res = SelectPreviousWord(hwnd);
		assert(res);
	}

	SendMessage(hwnd, EM_REPLACESEL, true, reinterpret_cast<LPARAM>(L""));
}

bool ItemNameEditControl::SelectPreviousWord(HWND hwnd)
{
	std::array<BYTE, 256> keyStates;
	bool res = GetKeyboardState(keyStates.data());

	if (!res)
	{
		return res;
	}

	auto originalKeyStates = keyStates;

	// Mark the ctrl and shift keys as down. Note that this only alters the keyboard state for
	// the current thread, which is the state accessed by GetKeyboardState() and GetKeyState().
	keyStates[VK_CONTROL] = 0x80;
	keyStates[VK_SHIFT] = 0x80;
	res = SetKeyboardState(keyStates.data());

	if (!res)
	{
		return res;
	}

	// The edit control will interpret this as a Ctrl+Shift+Left keypress. It will then select
	// the previous word. The specifics of exactly how a word is selected are left to the edit
	// control.
	SendSimulatedKeyPress(hwnd, VK_LEFT);

	res = SetKeyboardState(originalKeyStates.data());

	if (!res)
	{
		return res;
	}

	return true;
}

bool ItemNameEditControl::OnChar(TCHAR character)
{
	if (character == 127)
	{
		// This is the delete control character, generated by pressing Ctrl+Backspace. Erasing the
		// last word will be handled in	WM_KEYDOWN. Meanwhile, this message should be marked as
		// handled to stop it from being forwarded to the original window procedure (which will
		// insert a non-printable character if it receives the message).
		return true;
	}

	return false;
}
