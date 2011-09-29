/******************************************************************
 *
 * Project: Helper
 * File: ComboBoxHelper.cpp
 * License: GPL - See COPYING in the top level directory
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
	std::list<std::wstring> StringEntries;

	int NumItems = ComboBox_GetCount(hComboBox);

	for(int i = 0;i < NumItems;i++)
	{
		int Length = ComboBox_GetLBTextLen(hComboBox,i);

		TCHAR *StringEntry = new TCHAR[Length + 1];
		ComboBox_GetLBText(hComboBox,i,StringEntry);
		StringEntries.push_back(StringEntry);
		delete[] StringEntry;
	}

	return StringEntries;
}