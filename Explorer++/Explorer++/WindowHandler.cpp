// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "ApplicationToolbar.h"
#include "BookmarksToolbar.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ToolbarButtons.h"
#include "../Helper/Controls.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"

HWND Explorerplusplus::CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip)
{
	HWND TabToolbar = CreateToolbar(hParent,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|TBSTYLE_FLAT|CCS_NODIVIDER|
		CCS_NOPARENTALIGN|CCS_NORESIZE,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DOUBLEBUFFER);

	UINT dpi = m_dpiCompat.GetDpiForWindow(TabToolbar);
	int scaledIconSize = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
	
	SendMessage(TabToolbar,TB_SETBITMAPSIZE,0,MAKELONG(scaledIconSize,scaledIconSize));
	SendMessage(TabToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
	SendMessage(TabToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(scaledIconSize,scaledIconSize));

	/* TODO: The image list is been leaked. */
	HIMAGELIST himl = ImageList_Create(scaledIconSize,scaledIconSize,ILC_COLOR32|ILC_MASK,0,1);
	wil::unique_hbitmap bitmap = m_iconResourceLoader->LoadBitmapFromPNGForDpi(Icon::CloseButton, 16, 16, dpi);
	int iIndex = ImageList_Add(himl, bitmap.get(), nullptr);
	SendMessage(TabToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himl));

	/* Add the close button, used to close tabs. */
	TBBUTTON tbButton;
	tbButton.iBitmap	= iIndex;
	tbButton.idCommand	= idCommand;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE;
	tbButton.dwData		= 0;
	tbButton.iString	= reinterpret_cast<INT_PTR>(szTip);
	SendMessage(TabToolbar,TB_INSERTBUTTON,0,reinterpret_cast<LPARAM>(&tbButton));

	SendMessage(TabToolbar,TB_AUTOSIZE,0,0);

	return TabToolbar;
}

void Explorerplusplus::ResizeWindows(void)
{
	RECT rc;

	GetClientRect(m_hContainer,&rc);
	SendMessage(m_hContainer,WM_SIZE,
	SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
}

/* TODO: This should be linked to OnSize(). */
void Explorerplusplus::SetListViewInitialPosition(HWND hListView)
{
	RECT			rc;
	int				MainWindowWidth;
	int				MainWindowHeight;
	int				IndentBottom = 0;
	int				IndentTop = 0;
	int				IndentLeft = 0;
	int				iIndentRebar = 0;

	GetClientRect(m_hContainer,&rc);

	MainWindowWidth = GetRectWidth(&rc);
	MainWindowHeight = GetRectHeight(&rc);

	if(m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar,&rc);
		iIndentRebar += GetRectHeight(&rc);
	}

	if(m_config->showStatusBar)
	{
		GetWindowRect(m_hStatusBar,&rc);
		IndentBottom += GetRectHeight(&rc);
	}

	if(m_config->showDisplayWindow)
	{
		IndentBottom += m_config->displayWindowHeight;
	}

	if(m_config->showFolders)
	{
		GetClientRect(m_hHolder,&rc);
		IndentLeft = GetRectWidth(&rc);
	}

	IndentTop = iIndentRebar;

	RECT tabWindowRect;
	GetClientRect(m_tabContainer->GetHWND(), &tabWindowRect);

	int tabWindowHeight = GetRectHeight(&tabWindowRect);

	if(m_bShowTabBar)
	{
		if(!m_config->showTabBarAtBottom)
		{
			IndentTop += tabWindowHeight;
		}
	}

	if(!m_config->showTabBarAtBottom)
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop - tabWindowHeight,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
}

void Explorerplusplus::ToggleFolders(void)
{
	m_config->showFolders = !m_config->showFolders;

	if (m_config->showFolders)
	{
		UpdateTreeViewSelection();
	}

	lShowWindow(m_hHolder, m_config->showFolders);
	lShowWindow(m_hTreeView, m_config->showFolders);

	SendMessage(m_mainToolbar->GetHWND(),TB_CHECKBUTTON,(WPARAM)ToolbarButton::Folders,(LPARAM)m_config->showFolders);
	ResizeWindows();
}

void Explorerplusplus::UpdateLayout()
{
	RECT rc;
	GetClientRect(m_hContainer, &rc);
	SendMessage(m_hContainer, WM_SIZE, SIZE_RESTORED,
		MAKELPARAM(rc.right, rc.bottom));
}