// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Wraps a standard combo box control. Provides additional
 * autocomplete functionality.
 *
 * Notes:
 * The combobox control is not ideal when it comes to
 * implementing autocomplete. It is possible to show
 * a dropdown list of matched terms while the user
 * is typing, and allow them to select an entry
 * from that list, but the approach is fairly buggy.
 * The combobox will alter the edit text and selection
 * on list box selection changes, as well as when the
 * list is shown/hidden.
 * Another weakness is that the caret cannot be positioned
 * correctly (see below).
 *
 * A better solution might be to use a rich-edit control
 * in combination with a list box.
 */

#include "stdafx.h"
#include "Helper.h"
#include "WindowHelper.h"
#include "ComboBoxHelper.h"
#include "ComboBox.h"
#include "Macros.h"


UINT_PTR ComboBox::m_staticSubclassCounter = 0;

ComboBox *ComboBox::CreateNew(HWND hComboBox)
{
	return new ComboBox(hComboBox);
}

ComboBox::ComboBox(HWND hComboBox) :
BaseWindow(hComboBox),
m_SubclassCounter(m_staticSubclassCounter++),
m_SuppressAutocomplete(false)
{
	COMBOBOXINFO cbi;
	cbi.cbSize = sizeof(cbi);
	GetComboBoxInfo(m_hwnd,&cbi);
	SetWindowSubclass(cbi.hwndItem,ComboBoxEditProcStub,m_SubclassCounter,reinterpret_cast<DWORD_PTR>(this));

	/* Subclass the parent window so that the CBN_EDITCHANGE can be caught. */
	SetWindowSubclass(GetParent(m_hwnd),ComboBoxParentProcStub,m_SubclassCounter,reinterpret_cast<DWORD_PTR>(this));
}

INT_PTR ComboBox::OnDestroy()
{
	COMBOBOXINFO cbi;
	cbi.cbSize = sizeof(cbi);
	GetComboBoxInfo(m_hwnd,&cbi);
	RemoveWindowSubclass(cbi.hwndItem,ComboBoxEditProcStub,m_SubclassCounter);

	RemoveWindowSubclass(GetParent(m_hwnd),ComboBoxParentProcStub,m_SubclassCounter);

	return 0;
}

LRESULT CALLBACK ComboBox::ComboBoxEditProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pcb = reinterpret_cast<ComboBox *>(dwRefData);

	return pcb->ComboBoxEditProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK ComboBox::ComboBoxEditProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_KEYDOWN:
		if(wParam == VK_BACK ||
			wParam == VK_DELETE)
		{
			m_SuppressAutocomplete = true;
		}
		break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK ComboBox::ComboBoxParentProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pcb = reinterpret_cast<ComboBox *>(dwRefData);

	return pcb->ComboBoxParentProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK ComboBox::ComboBoxParentProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_COMMAND:
		if(HIWORD(wParam) != 0)
		{
			switch(HIWORD(wParam))
			{
			case CBN_EDITCHANGE:
				if(reinterpret_cast<HWND>(lParam) == m_hwnd)
				{
					return OnCBNEditChange();
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

INT_PTR ComboBox::OnCBNEditChange()
{
	if(m_SuppressAutocomplete)
	{
		m_SuppressAutocomplete = false;
		return 1;
	}

	std::wstring currentText = GetWindowString(m_hwnd);

	if(currentText.length() > 0)
	{
		std::list<std::wstring> stringEntries = NComboBox::ComboBox_GetStrings(m_hwnd);

		int index = 0;

		for(const auto &stringEntry : stringEntries)
		{
			if(stringEntry.compare(0,currentText.length(),currentText) == 0)
			{
				DWORD currentSelection = ComboBox_GetEditSel(m_hwnd);

				DWORD selectionStart = LOWORD(currentSelection);
				DWORD selectionEnd = HIWORD(currentSelection);

				/* Autocomplete will only be provided when typing at
				the end of the text. */
				if(selectionStart == currentText.length() &&
					selectionEnd == currentText.length())
				{
					/* The first matching entry will be used to autocomplete the text. */
					ComboBox_SetCurSel(m_hwnd,index);

					/* It would be better to select the text in reverse (i.e.
					from the end of the string to the last character the
					user typed), so that the caret position would be just
					after the last character typed.
					Single-line edit controls don't seem to support this
					however, and the caret will always be placed at the
					end of the selection (given by whichever selection
					position is greater).
					Multi-line edit controls and rich edit controls don't
					have this limitation. */
					ComboBox_SetEditSel(m_hwnd,currentText.length(),-1);
				}

				break;
			}

			index++;
		}
	}

	return 0;
}