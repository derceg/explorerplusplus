#pragma once

#include "BaseWindow.h"

class CComboBox : CBaseWindow
{
	friend LRESULT CALLBACK ComboBoxEditProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
	friend LRESULT CALLBACK ComboBoxParentProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	static CComboBox *CreateNew(HWND hComboBox);

protected:

	INT_PTR	OnDestroy();

private:

	DISALLOW_COPY_AND_ASSIGN(CComboBox);

	CComboBox(HWND hComboBox);

	LRESULT CALLBACK ComboBoxEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK ComboBoxParentProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	INT_PTR	OnCBNEditChange();

	static UINT_PTR m_StaticSubclassCounter;

	HWND	m_hComboBox;
	UINT_PTR m_SubclassCounter;

	bool	m_SuppressAutocomplete;
};