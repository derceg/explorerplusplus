// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseWindow.h"

class ComboBox : public BaseWindow
{
public:
	static ComboBox *CreateNew(HWND hComboBox);

protected:
	INT_PTR OnDestroy() override;

private:
	ComboBox(HWND hComboBox);
	~ComboBox() = default;

	static LRESULT CALLBACK ComboBoxEditProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ComboBoxEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ComboBoxParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ComboBoxParentProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	INT_PTR OnCBNEditChange();

	static UINT_PTR m_staticSubclassCounter;

	UINT_PTR m_SubclassCounter;

	bool m_SuppressAutocomplete;
};
