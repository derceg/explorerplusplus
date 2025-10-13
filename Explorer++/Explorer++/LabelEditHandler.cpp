// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LabelEditHandler.h"
#include "Accelerator.h"
#include "AcceleratorManager.h"
#include "TestHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <array>

LabelEditHandler *LabelEditHandler::CreateForMainWindow(HWND hwnd,
	AcceleratorManager *acceleratorManager, bool itemIsFile)
{
	return new LabelEditHandler(hwnd, acceleratorManager, itemIsFile);
}

LabelEditHandler *LabelEditHandler::CreateForDialog(HWND hwnd, bool itemIsFile)
{
	return new LabelEditHandler(hwnd, nullptr, itemIsFile);
}

LabelEditHandler *LabelEditHandler::CreateForTest(HWND hwnd, bool itemIsFile)
{
	CHECK(IsInTest());
	return new LabelEditHandler(hwnd, nullptr, itemIsFile);
}

LabelEditHandler::LabelEditHandler(HWND hwnd, AcceleratorManager *acceleratorManagerOpt,
	bool itemIsFile) :
	m_hwnd(hwnd),
	m_acceleratorManagerOpt(acceleratorManagerOpt),
	m_itemIsFile(itemIsFile),
	m_renameStage(RenameStage::Filename)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&LabelEditHandler::WndProc, this)));

	UpdateAcceleratorTable();
}

void LabelEditHandler::UpdateAcceleratorTable()
{
	if (!m_acceleratorManagerOpt)
	{
		return;
	}

	m_originalAccelerators = m_acceleratorManagerOpt->GetAccelerators();
	auto accelerators = m_acceleratorManagerOpt->GetAccelerators();

	// F2 will be handled below and used to cycle the selection between the filename and extension.
	std::vector<Accelerator> itemsToRemove = { { FVIRTKEY, VK_F2 } };

	if (IsParentListView())
	{
		// Tab/Shift+Tab will be used by the listview edit control. It has handling to move to the
		// next/previous item. However, for that functionality to be invoked, the control has to be
		// able to receive Tab/Shift+Tab key presses. Therefore, those items need to be removed from
		// the accelerator table.
		//
		// Once the keys have been removed from the accelerator table, they can be received as
		// normal.
		itemsToRemove.insert(itemsToRemove.end(),
			{ { FVIRTKEY, VK_TAB }, { FVIRTKEY | FSHIFT, VK_TAB } });
	}

	RemoveAcceleratorFromTable(accelerators, itemsToRemove);

	m_acceleratorManagerOpt->SetAccelerators(accelerators);
}

bool LabelEditHandler::IsParentListView() const
{
	HWND parent = GetParent(m_hwnd);

	if (!parent)
	{
		return false;
	}

	wchar_t className[256];
	auto res = GetClassName(parent, className, std::size(className));

	if (res == 0)
	{
		DCHECK(false);
		return false;
	}

	return lstrcmp(className, WC_LISTVIEW) == 0;
}

void LabelEditHandler::RemoveAcceleratorFromTable(std::vector<ACCEL> &accelerators,
	const std::vector<Accelerator> &itemsToRemove)
{
	for (auto &item : itemsToRemove)
	{
		auto itr =
			std::find_if(accelerators.begin(), accelerators.end(), [&item](const ACCEL &accel)
				{ return (accel.fVirt & ~FNOINVERT) == item.modifiers && accel.key == item.key; });

		if (itr != accelerators.end())
		{
			accelerators.erase(itr);
		}
	}
}

LabelEditHandler::~LabelEditHandler()
{
	if (m_acceleratorManagerOpt)
	{
		m_acceleratorManagerOpt->SetAccelerators(m_originalAccelerators);
	}
}

LRESULT LabelEditHandler::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		// The control may be shown within a dialog. The keys here are used when editing and should
		// be handled by the edit control, not the parent dialog (if any).
		case VK_ESCAPE:
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
		}
		break;

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

void LabelEditHandler::OnEMSetSel(WPARAM &wParam, LPARAM &lParam)
{
	// When editing an item, the parent listview/treeview control will first deselect, then select
	// all text. If an item has been put into edit mode, and the parent attempts to select all text,
	// modify the message so that only text up to the extension (if any) is selected.
	if (!m_initialSelectionSet && wParam == 0 && lParam == -1)
	{
		auto index = MaybeGetExtensionIndex(m_hwnd);

		if (index)
		{
			wParam = 0;
			lParam = *index;
		}

		m_initialSelectionSet = true;
	}
}

bool LabelEditHandler::OnKeyDown(HWND hwnd, UINT key)
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
		auto index = MaybeGetExtensionIndex(hwnd);

		if (!index)
		{
			break;
		}

		switch (m_renameStage)
		{
		case RenameStage::Filename:
			SendMessage(hwnd, EM_SETSEL, *index + 1, -1);
			m_renameStage = RenameStage::Extension;
			break;

		case RenameStage::Extension:
			SendMessage(hwnd, EM_SETSEL, 0, -1);
			m_renameStage = RenameStage::Entire;
			break;

		case RenameStage::Entire:
			SendMessage(hwnd, EM_SETSEL, 0, *index);
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

std::optional<int> LabelEditHandler::MaybeGetExtensionIndex(HWND hwnd)
{
	if (!m_itemIsFile)
	{
		return std::nullopt;
	}

	std::wstring fileName = GetWindowString(hwnd);
	auto position = fileName.find_last_of('.');

	if (position == std::wstring::npos)
	{
		return std::nullopt;
	}

	return static_cast<int>(position);
}

void LabelEditHandler::ErasePreviousWordOrSelectedText(HWND hwnd)
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

bool LabelEditHandler::SelectPreviousWord(HWND hwnd)
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

bool LabelEditHandler::OnChar(TCHAR character)
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
