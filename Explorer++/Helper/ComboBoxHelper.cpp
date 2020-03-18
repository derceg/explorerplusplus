// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ComboBoxHelper.h"


std::list<std::wstring> NComboBox::ComboBox_GetStrings(HWND hComboBox)
{
	std::list<std::wstring> entries;

	int numItems = ComboBox_GetCount(hComboBox);

	for(int i = 0;i < numItems;i++)
	{
		int length = ComboBox_GetLBTextLen(hComboBox,i);

		auto entry = std::make_unique<TCHAR[]>(length + 1);
		ComboBox_GetLBText(hComboBox,i,entry.get());

		entries.emplace_back(entry.get());
	}

	return entries;
}