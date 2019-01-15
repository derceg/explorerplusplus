/******************************************************************
 *
 * Project: Helper
 * File: ComboBoxHelper.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Combo box helper functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

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

		entries.push_back(entry.get());
	}

	return entries;
}