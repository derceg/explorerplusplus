// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include <string>
#include "StatusBar.h"


CStatusBar::CStatusBar(HWND hwnd) :
m_hwnd(hwnd)
{
	m_bAlteredStatusBarParts = false;
	m_nParts = 0;
}

void CStatusBar::SetPartText(int iPart, const TCHAR *szText)
{
	SendMessage(m_hwnd,SB_SETTEXT,MAKEWORD(iPart,0),
		reinterpret_cast<LPARAM>(szText));
}

void CStatusBar::HandleStatusBarMenuOpen(void)
{
	if(!m_bAlteredStatusBarParts)
	{
		/* Get the number of parts in the status bar window. */
		int nParts = static_cast<int>(SendMessage(m_hwnd,SB_GETPARTS,0,0));

		m_pPartWidths = new int[nParts];

		SendMessage(m_hwnd,SB_GETPARTS,nParts,
			reinterpret_cast<LPARAM>(m_pPartWidths));

		TCHAR szPartText[512];

		/* For each status bar part, retrieve the width and text
		of the part and store it. The text for that part will
		be restored when the menu is closed. */
		for(int i = 0;i < nParts;i++)
		{
			SendMessage(m_hwnd,SB_GETTEXT,i,reinterpret_cast<LPARAM>(szPartText));

			m_TextList.push_back(szPartText);
		}

		m_nParts = nParts;

		int aWidths = -1;

		/* Set the number of status bar parts to one. This single
		part will contain the help menu string for the selected
		menu. */
		SendMessage(m_hwnd,SB_SETPARTS,1,reinterpret_cast<LPARAM>(&aWidths));

		m_bAlteredStatusBarParts = true;
	}
}

void CStatusBar::HandleStatusBarMenuClose(void)
{
	int i = 0;

	if(m_nParts == 0)
	{
		return;
	}

	/* Restore the previous parts. */
	SendMessage(m_hwnd,SB_SETPARTS,m_nParts,reinterpret_cast<LPARAM>(m_pPartWidths));

	delete[] m_pPartWidths;

	for(const auto &strText : m_TextList)
	{
		/* Restore the text that was present before the menu was opened. */
		SendMessage(m_hwnd,SB_SETTEXT,(WPARAM)i|0,
			reinterpret_cast<LPARAM>(strText.c_str()));

		i++;
	}

	m_nParts = 0;
	m_TextList.clear();
	m_bAlteredStatusBarParts = false;
}